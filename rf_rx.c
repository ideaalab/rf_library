#include "rf_rx.h"

// --- INT EXT ---
#INT_EXT
void EXT_isr(void){
	TmrVal = GET_TIMER_VAL;	//obtenemos el tiempo desde la ultima interrupcion
	 
	CountedCycles = Cycles;	//guardamos las vueltas que dio el timer

	//flanco ascendente -> comenzamos a contar
	// __↑̅ ̅ |__
	if(INTEDG == RISING){
		SET_TIMER_VAL(0);	//reset timer
		Cycles = 0;			//reset cycles
	}
	
	flagPulse = TRUE;
	INTEDG = !INTEDG;	//invertimos el flanco de interrupcion
}


#ifdef RF_RX_TIMER0
#INT_TIMER0	//Interrupts every 256uS
#else
#INT_TIMER1	//Interrupts every 65536uS
#endif
void RF_timer_isr(void){
	Cycles++;
}

/*
 * Enciende la recepcion RF y configura el Timer para que incremente
 * cada 1uS, asi facilita los calculos de tiempo
 * 
 * Usa 13 de ROM (con timer 0)
 * Usa 15 de ROM (con timer 1)
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

#if defined(RF_DECODE_V0)
/* ORIGINAL v0 */
/*
 * Va incluyendo cada bit que se va recibiento en rfBuffer y comprueba si se han
 * recibido todos los bits incluido el sync del final. Cuando se recibieron
 * todos los bits esperados y el sync, la funcion devuelve TRUE
 * 
 * Ocupa unos 230 de ROM
 */
short DataFrameComplete(void){
	//comprobamos si el pulso es suficientemente largo y asi evitar analizar "ruido"
	if(TotalPulseDuration > MIN_PULSE){
		Duty = ((int32)HighPulseDuration * 100) / TotalPulseDuration;
		
		/* PULSO SYNC */
		if((MIN_SYNC <= Duty) && (Duty <= MAX_SYNC)){
			if(CountedBits == BUFFER_SIZE){		//la trama esta completa?
				CountedBits = 0;				//reinicio variable
				rfBuffer.Bytes.Nul = 0;
				return(TRUE);					//trama completa, devuelvo TRUE
			}

			CountedBits = 0;					//reinicio variable
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
		}
	}
	//esto es ruido, el pulso es menor a lo que podemos esperar
	else{
		CountedBits = 0;		//reinicio variable
	}
	
	return(FALSE);				//trama incompleta, devuelvo FALSE
}
#elif defined(RF_DECODE_V1)
/* OPTIMIZADA v1 */
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
 * Utiliza el SYNC como final de la trama de pulsos. En los mandos learning code
 * el SYNC se envia al principio, lo que hace que se pierda la primer trama de datos
 * 
 * Ocupa unos 143 de ROM
 */
short DataFrameComplete(void){
int16 syncMin = TotalPulseDuration >> 6;	//duty tiene que ser mayor que el tiempo total / 64
int16 syncMax = TotalPulseDuration >> 4;	//duty tiene que ser menor que el tiempo total / 16
int16 dutyLowMax = TotalPulseDuration >> 1;	//duty tiene que ser menor que el tiempo total / 2
		
	if(TotalPulseDuration > MIN_PULSE){		//check if pulse is long enough, to avoid noise
		
		/* PULSO SYNC */
		if((HighPulseDuration > syncMin) && (HighPulseDuration < syncMax)){
			if(CountedBits == BUFFER_SIZE){	//data frame complete?
				CountedBits = 0;			//restart counted bits
				rfBuffer.Bytes.Nul = 0;
				RestartRFmantenido();
				
				LastFrameDuration = TotalFrameDuration;
				TotalFrameDuration = 0;
				return(TRUE);				//data frame complete, returns TRUE
			}

			CountedBits = 0;				//restart counted bits
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
			
			if(CountedBits < BUFFER_SIZE)	//no more than BUFFER_SIZE
				++CountedBits;				//adds one
		}
	}
	else{
		CountedBits = 0;	//noise
		TotalFrameDuration = 0;
	}
	
	return(FALSE);			//incomplete data frame, returns FALSE
}
#else
/* OPTIMIZADA CON SYNC AL PRINCIPIO V2 */
/* 
 * Parecida a v1, pero utiliza el SYNC como inicio de la trama
 * 
 * Ocupa 161 de ROM
 */
short DataFrameComplete(void){
int16 syncMin = TotalPulseDuration >> 6;	//duty tiene que ser mayor que el tiempo total / 64 (>1.56%))
int16 syncMax = TotalPulseDuration >> 4;	//duty tiene que ser menor que el tiempo total / 16 (<6.25%)
int16 dutyLowMax = TotalPulseDuration >> 1;	//duty tiene que ser menor que el tiempo total / 2 (>50%)
		
	if(TotalPulseDuration > MIN_PULSE){		//check if pulse is long enough, to avoid noise
		
		/* PULSO SYNC */
		if((HighPulseDuration > syncMin) && (HighPulseDuration < syncMax)){
			CountedBits = 0;				//restart counted bits
			flagPulseSync = TRUE;
		}
		else if(flagPulseSync == TRUE){
			/* PULSO CERO */
			if(HighPulseDuration < dutyLowMax){
				shift_right(&rfBuffer,3,0);	//shift in received bit 0
			}
			/* PULSO UNO */
			else{
				shift_right(&rfBuffer,3,1);	//shift in received bit 1
			}
			
			//if(CountedBits < BUFFER_SIZE){	//no more than BUFFER_SIZE
				++CountedBits;				//adds one
			//}
			
			//else{							//assume frame is complete
			if(CountedBits == BUFFER_SIZE){	//data frame complete?
				//CountedBits = 0;			//restart counted bits (not needed? as its already cleared on sync pulse)
				flagPulseSync = FALSE;
				
				rfBuffer.Bytes.Nul = 0;		//clear the null byte, as it may have spureus data and will not match with the expected value
				rfReceived.Completo = rfBuffer.completo;	//copy the value in buffer to rfReceived
				RestartRFmantenido();
				
				/*LED = false;
				delay_us(1);
				LED = true;*/
				
				LastFrameDuration = TotalFrameDuration;
				TotalFrameDuration = 0;
				
				return(TRUE);				//data frame complete, returns TRUE
			}
		}
		else{
			CountedBits = 0;	//noise
			TotalFrameDuration = 0;
		}
	}
	else{
		//noise
		LimpiarRF();
	}
	
	return(FALSE);			//incomplete data frame, returns FALSE
}
#endif

/*
 * Calcula el tiempo transcurrido entre un flanco y el siguiente
 * En el flanco descendente (↓) cuenta el la parte alta del pulso
 * En el flanco ascendente (↑) cuenta la duracion total del pulso
 * Se entiende un pulso completo en el flanco ascendente
 * Devuelve TRUE si hay un pulso completo
 * 
 * Hay que hacer calculos de tiempo con la duracion de los pulsos
 * HighPulseDuration = duracion de la parte alta del pulso
 * TotalPulseDuration = duracion del pulso completo (H+L)
 * TotalFrameDuratio = duracion de todos los pulsos que componen una trama
 * TimeSinceLastValidFrame = tiempo transcurrido desde la ultima trama recibida
 * 
 * Ocupa 155 de ROM (usando timer 0)
 * Ocupa 158 de ROM (usando timer 1)
 */
short CalcTimes(void){
int1 PulseReady = FALSE;
int32 time = 0;	//variable temporal para almacenar tiempos

	//si hubo pulso cuenta duracion del pulso, duracion de la trama y tiempo desde ultima trama
	if(flagPulse == TRUE){
		flagPulse = FALSE;
		
		//asumimos que un pulso no puede durar mas de 65535 uS
		//si usamos el mismo calculo para el timer 1, la funcion ocuparia +20 de ROM
#ifdef RF_RX_TIMER0
		time = (CountedCycles * TIMER_MAX_VAL) + TmrVal;	//obtenemos duracion del ultimo pulso
#else
		time = TmrVal;										//obtenemos duracion del ultimo pulso
#endif
		
		//hubo flanco ascendente __↑̅̅|__
		if(INTEDG == FALLING){
			TotalPulseDuration = time;												//guardamos tiempo desde el ultimo pulso
			TotalFrameDuration = TotalFrameDuration + TotalPulseDuration;			//incrementamos duracion de la trama
			TimeSinceLastValidFrame = TimeSinceLastValidFrame + TotalPulseDuration;	//incrementamos tiempo desde ultima trama
			
			time = TimeSinceLastValidFrame;
			PulseReady = TRUE;
		}
		//hubo flanco descendente __|̅̅↓__
		else{
			HighPulseDuration = time;
		}
	}
	//si no hubo pulso cuenta el tiempo desde la ultima trama para RFmantenido
	else{
		time = TimeSinceLastValidFrame + ((int32)Cycles * TIMER_MAX_VAL) + GET_TIMER_VAL;
	}
	
	//si el tiempo es mayor al establecido para RFmantenido, entonces lo apaga
	if(time > RF_MANTENIDO_TIME_OUT_US){
		RestartRFmantenido();
		RFmantenido = FALSE;
	}
	
	return(PulseReady);
}

/*
 * Llamar a esta funcion en el loop principal para que se detecten
 * los bits de RF. Devuelve TRUE cuando la trama esta completa
 * 
 * Usa 9 de ROM
 */
short DataReady(void){
	if(CalcTimes() == TRUE){				//comprueba si se recibio pulso
		return(DataFrameComplete());
	}
	
	return(FALSE);	//default behaviour
}

/*
 * Devuelve el tiempo que ha durado la ultima trama RF
 * 
 * Usa 10 de ROM
 */
int32 GetRFTime(void){
	return(LastFrameDuration);
}

/*
 * Llamar a esta funcion cuando queremos que RFmantenido no llegue al tiempo
 * Por ejemplo, cuando llega una nueva trama de datos, o si estamos haciendo
 * algo que bloquea el RF
 * 
 * Usa 5 de ROM
 */
void RestartRFmantenido(void){
	TimeSinceLastValidFrame = 0;
}

/*
 * Limpia los pulsos recibidos y reinicia las variables
 */
void LimpiarRF(void){
	CountedBits = 0;	//noise
	TotalFrameDuration = 0;
	flagPulseSync = FALSE;
}