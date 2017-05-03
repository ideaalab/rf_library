/*
 * #Para usar esta libreria hay que estar usando la libreria rf_rx.c
 * 
 * #Permite leer y grabar mandos en la memoria EEPROM
 * 
 * #Permite saber si se mantiene presionado el boton de un mando
 * -Si se quiere usar la funcion de "mantener" el boton del mando, hay que
 * declarar un define "RF_MANTENIDO".
 * -RF_MANTENIDO usa el timer2
 * 
 * #Hay que declarar un define "NUM_MANDOS_RF" que indique cuantos mandos
 * se pueden memorizar
 * 
 * #Hay que declarar un define "POS_MEM_MANDOS_START_RF" que indica donde
 * comienzan a escribirse los mandos
 * 
 * #Los mandos/canales se almacenan en la variable DirRF
 * 
 * #Permite almacenar mandos y canales de dos maneras:
 * -----------------------------------------------------------------------------
 * -Solo se almacena la direccion del mando, y los canales de deducen de la
 * trama RF recibida
 * DirRF[Mando] (2 bytes c/u, los canales dependen de como se interprete la trama)
 * 
 * Ej: DirRF[3] (3 mandos)
 * DirRF[0]: [M1]
 * DirRF[1]: [M2]
 * DirRF[2]: [M3]
 * -----------------------------------------------------------------------------
 * -Cada canal como un mando nuevo. Almacena la trama RF entera para cada canal
 * DirRF[Mando][Canal] (3 bytes c/u)
 * 
 * Ej: DirRF[3][4] (3 mandos de 4ch c/u)
 * DirRF	 [x][0]		 [x][1]		 [x][2]		 [x][3]
 * [0][y]:	[M1/Ch1]	[M1/Ch2]	[M1/Ch3]	[M1/Ch4]
 * [1][y]:	[M2/Ch1]	[M2/Ch2]	[M2/Ch3]	[M2/Ch4]
 * [2][y]:	[M3/Ch1]	[M3/Ch2]	[M3/Ch3]	[M3/Ch4]
 * 
 * Ej: DirRF[5][1] (5 mandos de 1ch cada uno)
 * DirRF	[x][0]
 * [0][0]: [M1/Ch1]
 * [1][0]: [M1/Ch2]
 * [2][0]: [M1/Ch3]
 * [3][0]: [M1/Ch4]
 * [4][0]: [M1/Ch5]
 * -----------------------------------------------------------------------------
 * -Si se declara un define "NUM_CANALES_RF" que indique cuantos canales tiene
 * cada mando, entonces cada canal se graba "entero" (3 bytes = direccion + canal),
 * lo que permite almacenar cada canal con independencia de los canales del
 * mando fisico. Es decir que cada canal puede pertenecer a un mando diferente
 * -Si no se declara "NUM_CANALES_RF" entonces solo se graba la direccion
*/

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
 */
#int_TIMER2
void Timer2_isr(void){
	//compruebo si sigue mantenido el boton del mando
	if(RFmantenido == TRUE){
		if(ContMantenidoTimeOut++ >= TIME_OUT_RF_MANTENIDO){	
			ContMantenidoTimeOut = 0;
			LED = FALSE;
			RFmantenido = FALSE;
			//lo de abajo se puede quitar?
			/*RecibAnterior.Completo = 0;
			Recibido.Completo = 0;
			rfBuffer.Completo = 0;
			CountedBits = 0;*/
		}
	}
}

/*
 * Configura el Timer 2 para interrumpir cada 10mS
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
 * Graba los 3 bytes del mando recibido en la memoria EEPROM
 * Utiliza los datos que haya en el buffer de recepcion
 * La foma de grabar es como una pila FIFO. Primero se desplazan las memorias
 * ya guardadas una posicion hacia atras, y luego se graba el mando recibido
 * en la primera posicion. Asi la ultima se pierde.
 * Suponiendo: NUM_MANDOS=3, POS_MEM_MANDOS_START=0
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
 * Es mas eficiente, ocupa 25 palabras menos que la funcion de abajo.
 */
void GrabarMando(void){
	int x = 0;

	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la grabacion
	
	for(x = FIRST_POS_TO_MOVE; (x >= LAST_POS_TO_MOVE)&&(x <= FIRST_POS_TO_MOVE); x--){
		write_eeprom(x+POS_TO_JUMP, read_eeprom(x));
	}
	
#ifdef GRABAR_DIRECCIONES
#warning "Comprobar que funciona correctamente"
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
 * Misma funcion que la de arriba, pero a esta hay que pasarle
 * los valores a grabar en la EEPROM.
 * Es menos eficiente, ocupa 25 palabras mas
 */
void GrabarMando(rfRemote* RemoteAddr){
	int x = 0;

	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la grabacion
	
	for(x = FIRST_POS_TO_MOVE; (x >= LAST_POS_TO_MOVE)&&(x <= FIRST_POS_TO_MOVE); x--){
		write_eeprom(x+POS_TO_JUMP, read_eeprom(x));
	}
	
#ifdef GRABAR_DIRECCIONES
#warning "Comprobar que funciona correctamente"
	//graba la direccion recibida en la primera posicion (2 bytes)
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_ADDR_LO, RemoteAddr->Bytes.Lo);	//Ch4.AddrLo
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_ADDR_HI, RemoteAddr->Bytes.Mi);	//Ch4.AddrHi
#else
	//graba el canal recibido en la primera posicion (3 bytes)
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_LO, RemoteAddr->Bytes.Lo);	//Ch4.AddrLo
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_MI, RemoteAddr->Bytes.Mi);	//Ch4.AddrHi
	write_eeprom(POS_MEM_MANDOS_START_RF + RF_BYTE_HI, RemoteAddr->Bytes.Hi);	//Ch4.Dat
#endif
	
	enable_interrupts(GLOBAL);
}

/*
 * Lee los mandos de la EEPROM y los almacena en DirRF[]
 * Devuelve TRUE si hay algun mando configurado
*/
short LeerMandos(void){
	short Mando = FALSE;
	int PosBase = 0;
	
	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la lectura
	
#warning "Comprobar"
	
//lee los mandos
#ifdef GRABAR_DIRECCIONES
	for(int x = 0; x<NUM_MANDOS_RF; x++){
		PosBase = POS_MEM_MANDOS_START + (x*POS_TO_JUMP);
		DirRF[x].Completo = 0;
		DirRF[x].Bytes.Lo = read_eeprom(PosBase + RF_ADDR_LO);
		DirRF[x].Bytes.Hi = read_eeprom(PosBase + RF_ADDR_HI);

		//si el mando leido es diferente a 0xFFFFFF quiere decir que hay algo grabado
		if((DirRF[x][y].Completo != 0) && (DirRF[x].Completo != 0xFFFFFF)){
			Mando = TRUE;
		}
	}
#else
	for(int x = 0; x<NUM_MANDOS_RF; x++){
		for(int y = 0; y<NUM_CANALES_RF; y++){
			PosBase = POS_MEM_MANDOS_START_RF + (x*POS_TO_JUMP) + (y*RF_SAVE_BYTES);
			
			DirRF[x][y].Completo = 0;
			DirRF[x][y].Bytes.Lo = read_eeprom(PosBase + RF_BYTE_LO);
			DirRF[x][y].Bytes.Mi = read_eeprom(PosBase + RF_BYTE_MI);
			DirRF[x][y].Bytes.Hi = read_eeprom(PosBase + RF_BYTE_HI);
			
			//si el mando leido es diferente a 0xFFFFFF quiere decir que hay algo grabado
			if((DirRF[x][y].Completo != 0) && (DirRF[x][y].Completo != 0xFFFFFF)){
				Mando = TRUE;
			}
		}
	}
#endif
	
	enable_interrupts(GLOBAL);
	
	return(Mando);
}

/*
 * Borra todos los mandos sincronizados
 */
void BorrarMandos(void){
	disable_interrupts(GLOBAL);
	
	for(int x = POS_MEM_MANDOS_START_RF; x <= POS_MEM_MANDOS_END_RF; x++){
		write_eeprom(x, 0xFF);
	}
	
	enable_interrupts(GLOBAL);
}