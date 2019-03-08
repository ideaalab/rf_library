#include "rf_rx.h"

// --- INT EXT ---
#INT_EXT
void EXT_isr(void) {
/* --- USAMOS TIMER0 PARA CONTAR --- */
#ifdef RF_RX_TIMER0
int Tmr0;
	
	disable_interrupts(INT_TIMER0);
	Tmr0 = get_timer0();
	
	//flanco ascendente -> comenzamos a contar
	if(INTEDG == RISING){
		TotalDuration = (Cycles * 256) + Tmr0;	//obtenemos duracion del ultimo pulso
		set_timer0(0);							//reset timer
		Cycles = 0;								//reset cycles
		flagPulse = TRUE;						//indica que hemos recibido un pulso completo
	}
	//flanco descendente -> termino la parte alta del pulso
	else{
		HighDuration = (Cycles * 256) + Tmr0;	//obtenemos la parte alta del pulso
	}
	
	enable_interrupts(INT_TIMER0);
	
/* --- USAMOS TIMER1 PARA CONTAR --- */
#else
	//flanco ascendente -> comenzamos a contar
	if(INTEDG == RISING){
#ifdef RF_RX_COUNT_TIME
		if(CountedBits == 0){
			TotalTime = 0;
		}
#endif
		TotalDuration = get_timer1();	//obtenemos duracion del ultimo pulso
		set_timer1(0);					//reset timer
		flagPulse = TRUE;				//indica que hemos recibido un pulso completo
	}
	//flanco descendente -> termino la parte alta del pulso
	else{
		HighDuration = get_timer1();	//obtenemos la parte alta del pulso
	}
#endif
	
	INTEDG = !INTEDG;					//invertimos el flanco de interrupcion
}

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
 * Va incluyendo cada bit que se va recibiento en rfBuffer y comprueba si se han
 * recibido todos los bits incluido el sync del final. Cuando se recibieron
 * todos los bits esperados y el sync, la funcion devuelve TRUE
 */
short DataFrameComplete(void){
	//comprobamos si el pulso es suficientemente largo y asi evitar analizar "ruido"
	if(TotalDuration > MIN_PULSE){
		Duty = ((int32)HighDuration * 100) / TotalDuration;
#ifdef RF_RX_COUNT_TIME
		TotalTime = TotalTime + TotalDuration;
#endif
		/* PULSO SYNC */
		if((MIN_SYNC <= Duty) && (Duty <= MAX_SYNC)){
			if(CountedBits == BUFFER_SIZE){		//la trama esta completa?
				CountedBits = 0;				//reinicio variable
				rfBuffer.Bytes.Nul = 0;
				return(TRUE);					//trama completa, devuelvo TRUE
			}

			CountedBits = 0;					//reinicio variable
#ifdef RF_RX_COUNT_TIME
			TotalTime = 0;
#endif
		}
		/* PULSO CERO */
		else if((MIN_ZERO <= Duty) && (Duty <= MAX_ZERO)){
			shift_right(&rfBuffer,3,0);			//"empujo" el bit recibido por la derecha
			
			if(CountedBits < BUFFER_SIZE)		//no puede ser mayor que BUFFER_SIZE
				++CountedBits;					//suma uno
		}
		/* PULSO UNO */
		else if((MIN_ONE <= Duty) && (Duty <= MAX_ONE)){
			shift_right(&rfBuffer,3,1);			//"empujo" el bit recibido por la derecha
			
			if(CountedBits < BUFFER_SIZE)		//no puede ser mayor que BUFFER_SIZE
				++CountedBits;					//suma uno
		}
		/* RUIDO */
		else{
			CountedBits = 0;	//reinicio variable
#ifdef RF_RX_COUNT_TIME
			TotalTime = 0;
#endif
		}
	}
	else{
		CountedBits = 0;		//reinicio variable
#ifdef RF_RX_COUNT_TIME
		TotalTime = 0;
#endif
	}
	
	return(FALSE);				//trama incompleta, devuelvo FALSE
}

/*
 * Llamar a esta funcion en el loop principal para que se detecten
 * los bits de RF. Devuelve TRUE cuando la trama esta completa
 */
short DataReady(void){
short Ready;

	if(flagPulse == TRUE){				//comprueba si se recibio pulso
		flagPulse = FALSE;				//limpia flag
		Ready = DataFrameComplete();
		
		return(Ready);
	}
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