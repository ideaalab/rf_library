//------------------------------------------------------------------------------
//                    Receiver RF
//
// Description: Emulates Princeton Technology decoders:
//				PT2270,	PT2272 & PT2294.
//				Decodes encoded signals of:
//				PT2240B, PT2260, PT2262 & PT2264.
//
//				On this encoders/decoders ZERO, ONE and SYNC are coded
//				this way:
//				ZERO: 25% duty pulse
//				ONE: 75% duty pulse
//				SYNC: 3.125% duty pulse
//				
//				--- Decoders ---
//				*PT2270, PT2272, PT2294
//				Period of cicle (ZERO & ONE) = 16 * ALFA
//				Period of cicle (SYNC) = 128 * ALFA
//
//				--- Encoders ---
//				*PT2262, PT2264
//				Period of cicle (ZERO & ONE) = 16 * ALFA
//				Period of cicle (SYNC) = 128 * ALFA
//				
//				*PT2240B
//				Period of cicle (ZERO & ONE) = 128 * ALFA
//				Period of cicle (SYNC) = 1024 * ALFA
//				
//				*PT2260
//				Period of cicle (ZERO & ONE) = 512 * ALFA
//				Period of cicle (SYNC) = 4096 * ALFA
//
//				ALFA = oscilator period of encoder/decoder.
//				
//				Encoder oscilation frequency should be 4 to 16 times
//				grater than the decoder
//
//		-->		To use Timer0 define constant RF_RX_TIMER0 before calling
//				library, otherwise Timer1 will be used
//
//		-->		To know total lenght of received pulses define constant RF_COUNT_TIME,
//				check TotalTime value once DataReady() == TRUE to know total time of pulse
//				
//		-->		Declare RF_RX as #bit on INT_EXT pin of MCU
//
//		-->		To initialize use: EncenderRF()
//
//		-->		Use this in main loop to check if data received:
//
//				if(DataReady() == TRUE){			//check if there is new data
//					//data received is on rfBuffer
//					... execute instructions
//				}
//				
//
//------------------------------------------------------------------------------

#include "rf_rx.h"

// --- INT EXT ---
#INT_EXT
void EXT_isr(void) {
//--------------------------------------------------------------------------------------
#ifdef RF_RX_TIMER0	//using TIMER0 to count
int Tmr0;
	
	disable_interrupts(INT_TIMER0);
	Tmr0 = get_timer0();
	
	if(INTEDG == RISING){			//rising edge, pulse starts
		TotalDuration = (Cycles * 256) + Tmr0;	//gets duration of last pulse
		set_timer0(0);				//reset timer
		Cycles = 0;					//reset cycles
		flagPulse = TRUE;			//indicates that a complete pulse was received
	}
	else{							//falling edge, ends HIGH part of the pulse
		HighDuration = (Cycles * 256) + Tmr0;	//gets duration of the HIGH part
	}
	
	enable_interrupts(INT_TIMER0);
	INTEDG = !INTEDG;				//invert edge
//--------------------------------------------------------------------------------------
#else	//using TIMER1 to count
	if(INTEDG == RISING){				//rising edge, pulse starts
		TotalDuration = get_timer1();	//gets duration of last pulse
		set_timer1(0);					//reset timer
		flagPulse = TRUE;				//indicates that a complete pulse was received
	}
	else{								//falling edge, ends HIGH part of the pulse
		HighDuration = get_timer1();	//gets duration of the HIGH part
	}

	INTEDG = !INTEDG;					//invert edge
#endif
//--------------------------------------------------------------------------------------
}
// ---------------
#ifdef RF_RX_TIMER0
#INT_TIMER0
void Timer0_isr(void){
	Cycles++;
}
#endif

/*
 * Enciende la recepcion RF y configura el Timer para que incremente
 * cada 1uS, asi facilita los calculos de tiempo
 */
void EncenderRF(void){
//set the timer that is being used to count 1uS per clock cycle
#ifdef RF_RX_TIMER0
	//timer0 config
	#if getenv("CLOCK") == 4000000
		setup_timer_0(T0_INTERNAL|T0_DIV_1);
	#elif getenv("CLOCK") == 8000000
		setup_timer_0(T0_INTERNAL|T0_DIV_2);
	#elif getenv("CLOCK") == 16000000
		setup_timer_0(T0_INTERNAL|T0_DIV_4);
	#elif getenv("CLOCK") == 32000000
		setup_timer_0(T0_INTERNAL|T0_DIV_8);
	#else
		#ERROR La velocidad del PIC debe ser de 4, 8, 16 o 32Mhz
	#endif

		enable_interrupts(INT_TIMER0);	//enable interrupt
#else
	//timer1 config
	#if getenv("CLOCK") == 4000000
		setup_timer_1(T1_INTERNAL|T1_DIV_BY_1);
	#elif getenv("CLOCK") == 8000000
		setup_timer_1(T1_INTERNAL|T1_DIV_BY_2);
	#elif getenv("CLOCK") == 16000000
		setup_timer_1(T1_INTERNAL|T1_DIV_BY_4);
	#elif getenv("CLOCK") == 32000000
		setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);
	#else
		#ERROR La velocidad del PIC debe ser de 4, 8, 16 o 32Mhz
	#endif
#endif

	ext_int_edge(L_TO_H);			//set int edge
	enable_interrupts(INT_EXT);		//enable interrupt (receiver)
	enable_interrupts(GLOBAL);		//enable global interrupt
}

/*
 * Apaga la recepcion RF y modulos asociados
 */
void ApagarRF(void){
#ifdef RF_RX_TIMER0
	disable_interrupts(INT_TIMER0);
#endif
	disable_interrupts(INT_EXT);
}

/* MODIFICADA 
short DataFrameComplete(void){			//check received pulse, return TRUE if data frame is complete
	if(TotalDuration > MIN_PULSE){		//check if pulse is long enough, to avoid noise
		Duty = ((int32)HighDuration * 100) / TotalDuration;
		#IFDEF RF_COUNT_TIME
		TotalTime = TotalTime + TotalDuration;
		#ENDIF
		
		if(Duty < MAX_SYNC){	//sync pulse
			if(CountedBits == BUFFER_SIZE){		//data frame complete?
				CountedBits = 0;		//restart counted bits
				return(TRUE);			//data frame complete, returns TRUE
			}

			CountedBits = 0;		//restart counted bits
			#IFDEF RF_COUNT_TIME
			TotalTime = 0;
			#ENDIF
			
		}
		else{
			if(Duty < MAX_ZERO){	//zero pulse
				shift_right(&rfBuffer,3,0);	//shift in received bit
				
				if(CountedBits < BUFFER_SIZE)		//no more than BUFFER_SIZE
					++CountedBits;			//adds one
			}
			else{
				if(Duty > MIN_ONE){	//one pulse
					shift_right(&rfBuffer,3,1);	//shift in received bit
					
					if(CountedBits < BUFFER_SIZE)		//no more than BUFFER_SIZE
						++CountedBits;			//adds one
				}
				else{							//received pulse is noise
					CountedBits = 0;			//restart counted bits
					#IFDEF RF_COUNT_TIME
					TotalTime = 0;
					#ENDIF
				}
			}
		}
	}
	else{
		CountedBits = 0;	//noise
		#IFDEF RF_COUNT_TIME
		TotalTime = 0;
		#ENDIF
	}
	
	return(FALSE);			//incomplete data frame, returns FALSE
}
*/

/* ORIGINAL */
/*
 * Incluye los bits recibitos en la trama y comprueba si se
 * han recibido todos los bits incluido el sync del final
 */
short DataFrameComplete(void){			//check received pulse, return TRUE if data frame is complete
	if(TotalDuration > MIN_PULSE){		//check if pulse is long enough, to avoid noise
		Duty = ((int32)HighDuration * 100) / TotalDuration;
#ifdef RF_RX_COUNT_TIME
		TotalTime = TotalTime + TotalDuration;
#endif
		
		if((MIN_SYNC <= Duty) && (Duty <= MAX_SYNC)){	//sync pulse
			if(CountedBits == BUFFER_SIZE){		//data frame complete?
				CountedBits = 0;		//restart counted bits
				return(TRUE);			//data frame complete, returns TRUE
			}

			CountedBits = 0;		//restart counted bits
#ifdef RF_RX_COUNT_TIME
			TotalTime = 0;
#endif
		}
		else if((MIN_ZERO <= Duty) && (Duty <= MAX_ZERO)){	//zero pulse
			shift_right(&rfBuffer,3,0);	//shift in received bit
			
			if(CountedBits < BUFFER_SIZE)		//no more than BUFFER_SIZE
				++CountedBits;			//adds one
		}
		else if((MIN_ONE <= Duty) && (Duty <= MAX_ONE)){	//one pulse
			shift_right(&rfBuffer,3,1);	//shift in received bit
			
			if(CountedBits < BUFFER_SIZE)		//no more than BUFFER_SIZE
				++CountedBits;			//adds one
		}
		else{							//received pulse is noise
			CountedBits = 0;			//restart counted bits
#ifdef RF_RX_COUNT_TIME
			TotalTime = 0;
#endif
		}
	}
	else{
		CountedBits = 0;	//noise
#ifdef RF_RX_COUNT_TIME
		TotalTime = 0;
#endif
	}
	
	return(FALSE);			//incomplete data frame, returns FALSE
}

/*
 * Llamar a esta funcion en el loop principal para que se detecten
 * los bits de RF. Devuelve TRUE cuando la trama esta completa
 */
short DataReady(void){
short Ready;

	if(flagPulse == TRUE){				//check if pulse received
		flagPulse = FALSE;				//clear flag
		Ready = DataFrameComplete();
		rfBuffer.Bytes.Nul = 0;
		
		return(Ready);
	}
}
