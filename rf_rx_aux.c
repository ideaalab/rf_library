#include "rf_remotes.h"
#include "rf_rx_aux.h"

/* 
 * Esto permite saber si se mantiene presionado un boton de un mando a distancia
 * Cada vez que se reciba una trama correcta de RF y coincida con un mando hay
 * que poner a 0 la variable ContMantenidoTimeOut
 * El mantenido NO FUNCIONA BIEN CON CIRCUITOS DE SERVOS ya que introducen mucho
 * ruido y el receptor no consigue decodificar bien la señal, provocando "ausencia"
 * de tramas correctas.
 */
#ifdef RF_MANTENIDO
/*
 * Interrupcion del timer 2
 * Se usa para simular pulsacion mantenida por RF
 * Ocupa 13 de ROM
 */
#int_TIMER2
void Timer2_isr(void){
	//compruebo si sigue mantenido el boton del mando
	if(RFmantenido == TRUE){
		if(ContTimeOutRFmantenido++ >= VUELTAS_TIME_OUT_RF_MANTENIDO){	
			ContTimeOutRFmantenido = 0;
			LED = FALSE;
			RFmantenido = FALSE;
		}
	}
}

/*
 * Configura el Timer 2 para interrumpir cada 10mS
 * Ocupa 13 de ROM
 */
void RF_mantenido_init(void){
#IF getenv("CLOCK") == 4000000
	setup_timer_2(T2_DIV_BY_4,249,10);
#ELIF getenv("CLOCK") == 8000000
	setup_timer_2(T2_DIV_BY_16,249,5);
#ELIF getenv("CLOCK") == 16000000
	setup_timer_2(T2_DIV_BY_16,249,10);
#ELIF getenv("CLOCK") == 32000000
	setup_timer_2(T2_DIV_BY_64,249,5);
#ELSE
	#ERROR La velocidad del PIC debe ser de 4, 8, 16 o 32Mhz
#ENDIF

	enable_interrupts(INT_TIMER2);
	enable_interrupts(GLOBAL);		//enable global interrupt
}
#endif

/*
 * Mira si los datos recibidos por RF se corresponden con algun mando almacenado
 * Si hay alguna coincidencia se guarda en la variable ButtonMatch[] y la
 * funcion devuelve TRUE
 * Utiliza los valores de la variable "Recibido" para comparar
 * Ocupa:
 * -Direcciones: x ROM
 * -1 Canal: 80 ROM
 * -1 Canal + Mantenido: 93 ROM
 * -4 Canales: 106 ROM
 * -4 Canales + Mantenido: 121 ROM
 */
short AnalizarRF(void){
short Match = FALSE;	//indica si hubo alguna coincidencia
	
	ApagarRF();			//apago RF para que no interfieran las interrupciones
	
#ifdef GRABAR_DIRECCIONES
	#warning "Sin implementar"
#else

	#ifdef RF_MANTENIDO
	ContTimeOutRFmantenido = 0;
	
	//si se esta manteniendo un canal salgo de aqui
	if(RFmantenido == TRUE){
		set_timer2(0);
		ContTimeOutRFmantenido = 0;
		EncenderRF();	//vuelvo a enceder RF
		return(FALSE);
	}
	#endif

	/* comprueba si la direccion recibida coincide con algun mando */
	//recorre los mandos
	for(int m = 0; m < NUM_MANDOS_RF; m++){
		ButtonMatch[m] = 0;	//borro previas recepciones
		
	//cuando solo hay un canal podemos optimizar el codigo
	#if NUM_CANALES_RF == 1
		if(Recibido.Completo == MemRF[m][0].Completo){
			bit_set(ButtonMatch[m], 0);	//marco el canal 1 de cada mando que se haya presionado
			Match = TRUE;
		}
	#else
		//recorre los canales
		for(int c = 0; c < NUM_CANALES_RF; c++){
			if(Recibido.Completo == MemRF[m][c].Completo){
				bit_set(ButtonMatch[m], c);	//marco cada canal de cada mando que se haya presionado
				Match = TRUE;
			}
		}
	#endif
	}
	
	#ifdef RF_MANTENIDO
	RFmantenido = Match;
	set_timer2(0);
	ContTimeOutRFmantenido = 0;
	#endif

	EncenderRF();	//vuelvo a enceder RF
	return(Match);
#endif
}

/*
 * Mira si los datos recibidos por RF se corresponden con algun mando almacenado
 * Si hay alguna coincidencia se guarda en la variable ButtonMatch[] y la
 * funcion devuelve TRUE
 * Hay que pasarle una trama de datos para analizar
 * Ej: AnalizarRF(&Recibido);
 * Ocupa:
 * -Direcciones: x ROM
 * -1 Canal: 97 ROM
 * -1 Canal + Mantenido: 110 ROM
 * -4 Canales: 131 ROM
 * -4 Canales + Mantenido: 146 ROM
 */
short AnalizarRF(rfRemote* DatosRF){
short Match = FALSE;	//indica si hubo alguna coincidencia
	
	ApagarRF();			//apago RF para que no interfieran las interrupciones
	
#ifdef GRABAR_DIRECCIONES
	#warning "Sin implementar"
#else

	#ifdef RF_MANTENIDO
	ContTimeOutRFmantenido = 0;
	
	//si se esta manteniendo un canal salgo de aqui
	if(RFmantenido == TRUE){
		set_timer2(0);
		ContTimeOutRFmantenido = 0;
		EncenderRF();	//vuelvo a enceder RF
		return(FALSE);
	}
	#endif
	
	/* comprueba si la direccion recibida coincide con algun mando */
	//recorre los mandos
	for(int m = 0; m < NUM_MANDOS_RF; m++){
		ButtonMatch[m] = 0;	//borro previas recepciones
		
	//cuando solo hay un canal podemos optimizar el codigo
	#if NUM_CANALES_RF == 1
		if(DatosRF->Completo == MemRF[m][0].Completo){
			bit_set(ButtonMatch[m], 0);	//marco el canal 1 de cada mando que se haya presionado
			Match = TRUE;
		}
	#else
		//recorre los canales
		for(int c = 0; c < NUM_CANALES_RF; c++){
			if(DatosRF->Completo == MemRF[m][c].Completo){
				bit_set(ButtonMatch[m], c);	//marco cada canal de cada mando que se haya presionado
				Match = TRUE;
			}
		}
	#endif
	}
	
	#ifdef RF_MANTENIDO
	RFmantenido = Match;
	set_timer2(0);
	ContTimeOutRFmantenido = 0;
	#endif

	EncenderRF();	//vuelvo a enceder RF
	return(Match);
#endif
}

/*
 * Graba los 3 bytes del mando recibido en la memoria EEPROM.
 * La foma de grabar es como una pila FIFO. Primero se desplazan las memorias
 * ya guardadas una posicion hacia atras, y luego se graba el mando recibido
 * en la primera posicion. Asi la ultima se pierde.
 * Suponiendo: NUM_MANDOS_RF=3, NUM_CANALES_RF=1, POS_MEM_MANDOS_START=0
 * 5>8, 4>7, 3>6, 2>5, 1>4, 0>3
 *      _____
 *    _|___  v_____
 *  _|_|_  v_|___  v
 * | | | v_|_|_  v  
 * | | | | | | v    
 * 0 1 2 3 4 5 6 7 8
*/

/*
 * Esta funcion toma los datos a grabar de la variable Recibido.
 * Graba direcciones: x de ROM
 * Graba mandos: 131 de ROM
 */
void GrabarMando(void){
	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la grabacion
	
	MoverBloque(LAST_POS_TO_MOVE, FIRST_POS_TO_MOVE, POS_TO_JUMP);
	
#ifdef GRABAR_DIRECCIONES
#warning "Comprobar"
	//graba la direccion recibida en la primera posicion (2 bytes)
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_ADDR_LO, Recibido.Bytes.Lo);	//Ch4.AddrLo
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_ADDR_HI, Recibido.Bytes.Mi);	//Ch4.AddrHi
#else
	//graba el canal recibido en la primera posicion (3 bytes)
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_LO, Recibido.Bytes.Lo);	//Ch4.AddrLo
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_MI, Recibido.Bytes.Mi);	//Ch4.AddrHi
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_HI, Recibido.Bytes.Hi);	//Ch4.Dat
#endif
	
	enable_interrupts(GLOBAL);
}

/*
 * Misma funcion que la de arriba, pero a esta hay que pasarle los valores a
 * grabar en la EEPROM. Es menos eficiente.
 * Se llama asi: GrabarMando(&Recibido);
 * Graba direcciones: x de ROM
 * Graba mandos: 156 de ROM
 */
void GrabarMando(rfRemote* DatosRF){

	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la grabacion
	
	MoverBloque(LAST_POS_TO_MOVE, FIRST_POS_TO_MOVE, POS_TO_JUMP);
	
#ifdef GRABAR_DIRECCIONES
#warning "Comprobar"
	//graba la direccion recibida en la primera posicion (2 bytes)
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_ADDR_LO, DatosRF->Bytes.Lo);	//Ch4.AddrLo
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_ADDR_HI, DatosRF->Bytes.Mi);	//Ch4.AddrHi
#else
	//graba el canal recibido en la primera posicion (3 bytes)
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_LO, DatosRF->Bytes.Lo);	//Ch4.AddrLo
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_MI, DatosRF->Bytes.Mi);	//Ch4.AddrHi
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_HI, DatosRF->Bytes.Hi);	//Ch4.Dat
#endif
	
	enable_interrupts(GLOBAL);
}

/*
 * Graba al mismo tiempo un mando en bloque (NUM_CANALES_RF * canales a la vez)
 * Los valores almacenados son los que hay en MandoVirtual[]
 * Ocupa: 215 ROM
 */
#ifdef NUM_CANALES_RF
#if NUM_CANALES_RF > 1
void GrabarBloqueMandos(void){
	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la grabacion
	
	MoverBloque(LAST_POS_TO_MOVE, FIRST_POS_TO_MOVE, POS_TO_JUMP);
	
	int x, start;
	
	for(x = 0; x < NUM_CANALES_RF; x++){
		//graba el mando recibido (3 bytes)
		start = POS_MEM_MANDOS_START_RF + (x * RF_SAVE_BYTES);
		write_eeprom(start + RF_BYTE_LO, MandoVirtual[x].Bytes.Lo);	//Ch4.AddrLo
		write_eeprom(start + RF_BYTE_MI, MandoVirtual[x].Bytes.Mi);	//Ch4.AddrHi
		write_eeprom(start + RF_BYTE_HI, MandoVirtual[x].Bytes.Hi);	//Ch4.Dat
	}
	
	enable_interrupts(GLOBAL);
}

/*
 * Graba al mismo tiempo un mando en bloque (NUM_CANALES_RF * canales a la vez)
 * Se llama asi: GrabarBloqueMandos(&MandoVirtual[0]);
 * Ocupa: 240 ROM
 */
void GrabarBloqueMandos(rfRemote* DatosRF){
// https://stackoverflow.com/questions/13730786/c-pass-int-array-pointer-as-parameter-into-a-function
// El nombre de un array hace referencia a la primera direccion de ese array = &array[0], pero aqui no funciona
	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la grabacion
	
	MoverBloque(LAST_POS_TO_MOVE, FIRST_POS_TO_MOVE, POS_TO_JUMP);
	
	int x, start;
	
	for(x = 0; x < NUM_CANALES_RF; x++){
		//graba el mando recibido (3 bytes)
		start = POS_MEM_MANDOS_START_RF + (x * RF_SAVE_BYTES);
		write_eeprom(start + RF_BYTE_LO, DatosRF[x].Bytes.Lo);	//Ch4.AddrLo
		write_eeprom(start + RF_BYTE_MI, DatosRF[x].Bytes.Mi);	//Ch4.AddrHi
		write_eeprom(start + RF_BYTE_HI, DatosRF[x].Bytes.Hi);	//Ch4.Dat
	}
	
	enable_interrupts(GLOBAL);
}
#endif
#endif

/*
 * Lee los mandos de la EEPROM y los almacena en MemRF[]
 * Devuelve TRUE si hay algun mando configurado
 * Uso de ROM:
 * Lee direcciones: x de ROM
 * Lee mandos de 1CH: 192 de ROM
 * Lee mandos de > 1CH: 254 de ROM
 */
short LeerMandos(void){
	short Mando = FALSE;
	int PosBase = 0;
	int x;
	
	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la lectura
	
//lee los mandos
#ifdef GRABAR_DIRECCIONES
#warning "Comprobar"
	for(x = 0; x<NUM_MANDOS_RF; x++){
		PosBase = POS_MEM_MANDOS_START_RF + (x*POS_TO_JUMP);
		MemRF[x].AddrLo = read_eeprom(PosBase + RF_ADDR_LO);
		MemRF[x].AddrHi = read_eeprom(PosBase + RF_ADDR_HI);

		//si la direccion leida es diferente a 0xFFFF quiere decir que hay algo grabado
		if(MemRF[x].Addr != 0){
			Mando = TRUE;
		}
	}
#else
	//optimizado para mandos de 1CH
	#if NUM_CANALES_RF == 1
	for(x = 0; x<NUM_MANDOS_RF; x++){
		PosBase = POS_MEM_MANDOS_START_RF + (x*POS_TO_JUMP);

		MemRF[x][0].Completo = 0;
		MemRF[x][0].Bytes.Lo = read_eeprom(PosBase + RF_BYTE_LO);
		MemRF[x][0].Bytes.Mi = read_eeprom(PosBase + RF_BYTE_MI);
		MemRF[x][0].Bytes.Hi = read_eeprom(PosBase + RF_BYTE_HI);

		//si el mando leido es diferente a 0xFFFFFF quiere decir que hay algo grabado
		if((MemRF[x][0].Completo != 0) && (MemRF[x][0].Completo != 0xFFFFFF)){
			Mando = TRUE;
		}
	}
	//para mandos de mas de 1CH
	#else
	for(x = 0; x<NUM_MANDOS_RF; x++){
		for(int y = 0; y<NUM_CANALES_RF; y++){
			PosBase = POS_MEM_MANDOS_START_RF + (x*POS_TO_JUMP) + (y*RF_SAVE_BYTES);
			
			MemRF[x][y].Completo = 0;
			MemRF[x][y].Bytes.Lo = read_eeprom(PosBase + RF_BYTE_LO);
			MemRF[x][y].Bytes.Mi = read_eeprom(PosBase + RF_BYTE_MI);
			MemRF[x][y].Bytes.Hi = read_eeprom(PosBase + RF_BYTE_HI);
			
			//si el mando leido es diferente a 0xFFFFFF quiere decir que hay algo grabado
			if((MemRF[x][y].Completo != 0) && (MemRF[x][y].Completo != 0xFFFFFF)){
				Mando = TRUE;
			}
		}
	}
	#endif
#endif
	
	enable_interrupts(GLOBAL);
	
	return(Mando);
}

/*
 * Borra todos los mandos sincronizados
 * Ocupa 37 de ROM
 */
void BorrarMandos(void){
	disable_interrupts(GLOBAL);
	
	for(int x = POS_MEM_MANDOS_START_RF; x <= POS_MEM_MANDOS_END_RF; x++){
		write_eeprom(x, 0xFF);
	}
	
	enable_interrupts(GLOBAL);
}

/*
 * Mueve un rango de valores de la EEPROM x posiciones hacia adelante
 * from: desde que posicion mover (inclusive)
 * to: hasta que posicion mover (inclusive)
 * offset: cuantas posiciones la mueve
 */
void MoverBloque(int from, int to, int offset){
	disable_interrupts(GLOBAL);
	
	for(int x = to; (x >= from) && (x <= to); x--){
		write_eeprom(x+offset, read_eeprom(x));
	}
	
	enable_interrupts(GLOBAL);
}