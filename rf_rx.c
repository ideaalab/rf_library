#include "rf_rx.h"

// --- INT EXT ---
#INT_EXT
void EXT_isr(void) {
#ifdef RF_RX_TIMER0
	Tmr0 = get_timer0();
#else
	Tmr1 = get_timer1();
#endif

	CountedCycles = Cycles;
	
	//flanco ascendente -> comenzamos a contar
	// __↑̅ ̅ |__
	if(INTEDG == RISING){
#ifdef RF_RX_TIMER0
		set_timer0(0);		//reset timer
#else
		set_timer1(0);		//reset timer
#endif
		Cycles = 0;			//reset cycles
		RisingFlag = TRUE;	//indicamos que hubo un flanco de subida
	}
	//flanco descendente -> termino la parte alta del pulso
	// __|̅ ̅ ↓__
	else{
		FallingFlag = TRUE;
	}
	
	INTEDG = !INTEDG;					//invertimos el flanco de interrupcion
}


#ifdef RF_RX_TIMER0
#INT_TIMER0
void Timer0_isr(void){
	Cycles++;
}
#else
#INT_TIMER1
void Timer1_isr(void){
	Cycles++;
}
#endif

/*
 * Enciende la recepcion RF y configura el Timer para que incremente
 * cada 1uS, asi facilita los calculos de tiempo
 */
void EncenderRF(void){
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
		#ERROR "La velocidad del PIC debe ser de 4, 8, 16 o 32Mhz"
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
		#ERROR "La velocidad del PIC debe ser de 4, 8, 16 o 32Mhz"
	#endif

	enable_interrupts(INT_TIMER1);	//enable interrupt
#endif

	ext_int_edge(L_TO_H);			//configuramos flanco de interrupcion
	enable_interrupts(INT_EXT);		//habilitamos interrupcion por flanco
	enable_interrupts(GLOBAL);		//habilitamos interrupciones globales
}

/*
 * Apaga la recepcion RF y modulos asociados
 */
void ApagarRF(void){
#ifdef RF_RX_TIMER0
	disable_interrupts(INT_TIMER0);
#else
	disable_interrupts(INT_TIMER1);
#endif
	
	disable_interrupts(INT_EXT);
}

// MODIFICADA 
short DataFrameComplete(void){			//check received pulse, return TRUE if data frame is complete
int32 syncMax = TotalDuration >> 4;		//duty tiene que ser menor que el tiempo total / 16
int32 dutyLowMax = TotalDuration >> 1;	//duty tiene que ser menor que el tiempo total / 2
		
	if(TotalDuration > MIN_PULSE){		//check if pulse is long enough, to avoid noise
		
		/* PULSO SYNC */
		if(HighDuration < syncMax){
			if(CountedBits == BUFFER_SIZE){		//data frame complete?
				CountedBits = 0;		//restart counted bits
				rfBuffer.Bytes.Nul = 0;
				return(TRUE);			//data frame complete, returns TRUE
			}

			CountedBits = 0;		//restart counted bits
		}
		else{
		/* PULSO CERO */
			if(HighDuration < dutyLowMax){
				shift_right(&rfBuffer,3,0);	//shift in received bit
				
				if(CountedBits < BUFFER_SIZE)		//no more than BUFFER_SIZE
					++CountedBits;			//adds one
			}
		/* PULSO UNO */
			else{
				shift_right(&rfBuffer,3,1);	//shift in received bit

				if(CountedBits < BUFFER_SIZE)		//no more than BUFFER_SIZE
					++CountedBits;			//adds one
			}
		}
	}
	else{
		CountedBits = 0;	//noise
	}
	
	return(FALSE);			//incomplete data frame, returns FALSE
}


/* ORIGINAL */
/*
 * Va incluyendo cada bit que se va recibiento en rfBuffer y comprueba si se han
 * recibido todos los bits incluido el sync del final. Cuando se recibieron
 * todos los bits esperados y el sync, la funcion devuelve TRUE
 */
//short DataFrameComplete(void){
//	//comprobamos si el pulso es suficientemente largo y asi evitar analizar "ruido"
//	if(TotalDuration > MIN_PULSE){
//		Duty = ((int32)HighDuration * 100) / TotalDuration;
//		
//		/* PULSO SYNC */
//		if((MIN_SYNC <= Duty) && (Duty <= MAX_SYNC)){
//			if(CountedBits == BUFFER_SIZE){		//la trama esta completa?
//				CountedBits = 0;				//reinicio variable
//				rfBuffer.Bytes.Nul = 0;
//				return(TRUE);					//trama completa, devuelvo TRUE
//			}
//
//			CountedBits = 0;					//reinicio variable
//		}
//		/* PULSO CERO */
//		else if((MIN_ZERO <= Duty) && (Duty <= MAX_ZERO)){
//			shift_right(&rfBuffer,3,0);			//"empujo" el bit recibido por la derecha
//			
//			if(CountedBits < BUFFER_SIZE)		//no puede ser mayor que BUFFER_SIZE
//				++CountedBits;					//suma uno
//		}
//		/* PULSO UNO */
//		else if((MIN_ONE <= Duty) && (Duty <= MAX_ONE)){
//			shift_right(&rfBuffer,3,1);			//"empujo" el bit recibido por la derecha
//			
//			if(CountedBits < BUFFER_SIZE)		//no puede ser mayor que BUFFER_SIZE
//				++CountedBits;					//suma uno
//		}
//		/* RUIDO */
//		else{
//			CountedBits = 0;	//reinicio variable
//		}
//	}
//	//esto es ruido, el pulso es menor a lo que podemos esperar
//	else{
//		CountedBits = 0;		//reinicio variable
//	}
//	
//	return(FALSE);				//trama incompleta, devuelvo FALSE
//}

/*
 * Calcula el tiempo transcurrido entre un flanco y el siguiente
 */
void CalcTimes(void){
	//flanco ascendente __↑̅̅|__
	if(RisingFlag == TRUE){
		RisingFlag = FALSE;
		
#ifdef RF_RX_COUNT_TIME
		if(CountedBits == 0){
			TotalTime = 0;
		}
#endif

#ifdef RF_RX_TIMER0
		TotalDuration = (CountedCycles * 256) + Tmr0;	//obtenemos duracion del ultimo pulso
#else
		TotalDuration = (CountedCycles * 65536) + Tmr1;	//obtenemos duracion del ultimo pulso
#endif
		
#ifdef RF_RX_COUNT_TIME
		TotalTime = TotalTime + TotalDuration;
#endif
		
		flagPulse = TRUE;						//indica que hemos recibido un pulso completo
	}
	//flanco descendente __|̅̅↓__
	else if(FallingFlag == TRUE){
		FallingFlag = FALSE;
		
#ifdef RF_RX_TIMER0
		HighDuration = (CountedCycles * 256) + Tmr0;	//obtenemos la parte alta del pulso
#else
		HighDuration = (CountedCycles * 65536) + Tmr1;	//obtenemos la parte alta del pulso
#endif
	}
}

/*
 * Llamar a esta funcion en el loop principal para que se detecten
 * los bits de RF. Devuelve TRUE cuando la trama esta completa
 */
short DataReady(void){
short Ready = FALSE;

	CalcTimes();
	
	if(flagPulse == TRUE){				//comprueba si se recibio pulso
		flagPulse = FALSE;				//limpia flag
		Ready = DataFrameComplete();
		//return(Ready);
	}
	
	return(Ready);
}

#ifdef RF_RX_COUNT_TIME
/*
 * Devuelve el tiempo que ha durado la ultima trama RF
 * Utilizar solo despues de que DataReady == TRUE
 */
int32 GetRFTime(void){
	return(TotalTime);
}
#endif