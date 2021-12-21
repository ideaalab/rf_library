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
	CyclesSinceLastValidFrame++;
}
#else
#INT_TIMER1
void Timer1_isr(void){
	Cycles++;
	CyclesSinceLastValidFrame++;
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

/* ORIGINAL */
/*
 * Va incluyendo cada bit que se va recibiento en rfBuffer y comprueba si se han
 * recibido todos los bits incluido el sync del final. Cuando se recibieron
 * todos los bits esperados y el sync, la funcion devuelve TRUE
 * 
 * Ocupa unos 230 de ROM
 */
//short DataFrameComplete(void){
//	//comprobamos si el pulso es suficientemente largo y asi evitar analizar "ruido"
//	if(TotalPulseDuration > MIN_PULSE){
//		Duty = ((int32)HighPulseDuration * 100) / TotalPulseDuration;
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

/* OPTIMIZADA */
/*
 * Igual que original en funcionamiento, pero es mas rapida y menos restrictiva
 * a la hora de diferenciar los bits. Basicamente por encima del 50% del duty
 * es un 1, y por debajo es un 0.
 * Al ser menos restrictiva en teoria es mas facil que un ruido al azar pueda
 * activarla. Pero en la practica, necesita completar 24 bits y un sync todos
 * con una duracion minima que si fuese ruido seria dificil de saltarse. Ademas
 * si fuese el caso, luego se tiene que comparar con una direccion previamente
 * guardada, lo que hace practicamente imposible que un ruido al azar pueda 
 * activar nuestro dispositivo.
 * 
 * Ocupa unos 126 de ROM
 */
short DataFrameComplete(void){
int32 syncMin = TotalPulseDuration >> 6;	//duty tiene que ser mayor que el tiempo total / 64
int32 syncMax = TotalPulseDuration >> 4;	//duty tiene que ser menor que el tiempo total / 16
int32 dutyLowMax = TotalPulseDuration >> 1;	//duty tiene que ser menor que el tiempo total / 2
		
	if(TotalPulseDuration > MIN_PULSE){		//check if pulse is long enough, to avoid noise
		
		/* PULSO SYNC */
		if((HighPulseDuration > syncMin) && (HighPulseDuration < syncMax)){
			if(CountedBits == BUFFER_SIZE){		//data frame complete?
				CountedBits = 0;		//restart counted bits
				//rfBuffer.Bytes.Nul = 0;
				RestartRFmantenido();
				return(TRUE);			//data frame complete, returns TRUE
			}

			CountedBits = 0;		//restart counted bits
		}
		else{
			/* PULSO CERO */
			if(HighPulseDuration < dutyLowMax){
				shift_right(&rfBuffer,3,0);	//shift in received bit
			}
			/* PULSO UNO */
			else{
				shift_right(&rfBuffer,3,1);	//shift in received bit
			}
			
			if(CountedBits < BUFFER_SIZE)		//no more than BUFFER_SIZE
				++CountedBits;			//adds one
		}
	}
	else{
		CountedBits = 0;	//noise
	}
	
	return(FALSE);			//incomplete data frame, returns FALSE
}

/*
 * Calcula el tiempo transcurrido entre un flanco y el siguiente
 */
void CalcTimes(void){

	//flanco ascendente __↑̅̅|__
	if(RisingFlag == TRUE){
		RisingFlag = FALSE;
		
		//guardo tiempo total del frame
		if(CountedBits == 0){
			LastFrameDuration = TotalFrameDuration;
			TotalFrameDuration = 0;
		}

#ifdef RF_RX_TIMER0
		TotalPulseDuration = ((int32)CountedCycles * 256) + Tmr0;	//obtenemos duracion del ultimo pulso
#else
		TotalPulseDuration = ((int32)CountedCycles * 65536) + Tmr1;	//obtenemos duracion del ultimo pulso
#endif
		
		TotalFrameDuration = TotalFrameDuration + TotalPulseDuration;
		flagPulse = TRUE;						//indica que hemos recibido un pulso completo
	}
	//flanco descendente __|̅̅↓__
	else if(FallingFlag == TRUE){
		FallingFlag = FALSE;
		
#ifdef RF_RX_TIMER0
		HighPulseDuration = ((int32)CountedCycles * 256) + Tmr0;	//obtenemos la parte alta del pulso
#else
		HighPulseDuration = ((int32)CountedCycles * 65536) + Tmr1;	//obtenemos la parte alta del pulso
#endif
	}

#ifdef RF_RX_TIMER0
	TimeSinceLastValidFrame = ((int32)CyclesSinceLastValidFrame * 256) + get_timer0();		//obtenemos duracion del ultimo pulso
#else
	TimeSinceLastValidFrame = ((int32)CyclesSinceLastValidFrame * 65536) + get_timer1();	//obtenemos duracion del ultimo pulso
#endif
	
	if(TimeSinceLastValidFrame > TIME_OUT_RF_MANTENIDO){
		RestartRFmantenido();
		RFmantenido = FALSE;
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
	}
	
	return(Ready);
}

/*
 * Devuelve el tiempo que ha durado la ultima trama RF
 */
int32 GetRFTime(void){
	return(LastFrameDuration);
}

void RestartRFmantenido(void){
	TimeSinceLastValidFrame = 0;
	CyclesSinceLastValidFrame = 0;
}