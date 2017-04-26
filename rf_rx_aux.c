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

#ifndef NUM_MANDOS_RF
#ERROR "Hay que declarar NUM_MANDOS_RF para que la libreria funcione"
#endif

#ifdef NUM_CANALES_RF
#define GRABAR_CANALES
#warning "Los canales de los mandos se graban de manera independiente (3 bytes cada uno)"
#else
#define GRABAR_DIRECCIONES
#warning "Los canales de los mandos no se graban, solo se graba la direccion del mando"
#endif

#ifndef POS_MEM_MANDOS_START_RF
#ERROR "Hay que declarar POS_MEM_MANDOS_START_RF para que la libreria funcione"
#endif

#ifdef RF_MANTENIDO
#warning Se detectara cuando se mantiene presionado un boton del mando

#define T_T2					10	//interrupcion timer 2 en mS
#define TMP_SIN_RF				200	//cuanto tiempo tiene que pasar para que se interprete como que no esta mantenido
#define TIME_OUT_RF_MANTENIDO	(TMP_SIN_RF / T_T2)

#else
#warning No se detecta cuando se mantiene presionado un boton del mando
#endif

/* DEFINES */
#define RF_ADDR_LO	0
#define RF_ADDR_HI	1

#define RF_BYTE_LO	0
#define RF_BYTE_MI	1
#define RF_BYTE_HI	2

//cuantos bytes de guardan por cada mando/canal
#ifdef GRABAR_DIRECCIONES
#define RF_SAVE_BYTES	2
#else
#define RF_SAVE_BYTES	3
#endif

#warning "Hacer un header para esta libreria"

//usadas para cuando se graba un mando
#define LAST_POS_TO_MOVE	(POS_MEM_MANDOS_START_RF)

#ifdef GRABAR_DIRECCIONES	//se graban solo direcciones (2 bytes))
#warning "Comprobar si esta bien"
#define FIRST_POS_TO_MOVE	(POS_MEM_MANDOS_START_RF + ((NUM_MANDOS_RF - 1) * RF_SAVE_BYTES) - 1)
#define POS_TO_JUMP			(RF_SAVE_BYTES))
#define	RF_LAST_VALUE		(POS_MEM_MANDOS_START_RF + (NUM_MANDOS_RF * RF_SAVE_BYTES) - 1)
#else						//se graban los canales
#define FIRST_POS_TO_MOVE	(POS_MEM_MANDOS_START_RF + ((NUM_MANDOS_RF - 1) * NUM_CANALES_RF * RF_SAVE_BYTES) - 1)
#define POS_TO_JUMP			(RF_SAVE_BYTES * NUM_CANALES_RF)
#define	RF_LAST_VALUE		(POS_MEM_MANDOS_START_RF + (NUM_MANDOS_RF * NUM_CANALES_RF * RF_SAVE_BYTES) - 1)
#endif

/* MACROS */
//devuelve la posicion de memoria donde se encuentra el mando

/* VARIABLES */
short flagSync = false;		//indica si estamos grabando un mando

int ButtonMatch[NUM_MANDOS_RF];//indica que botones se presionaron de cada mando (max 8 botones por mando, 1bit cada boton)

#ifdef GRABAR_DIRECCIONES
rfAddr DirRF[NUM_MANDOS_RF];	//direcciones de los mandos almacenados
#else
rfRemote DirRF[NUM_MANDOS_RF][NUM_CANALES_RF];	//direcciones de los mandos/botones almacenados
rfRemote RecibAnterior;							//anterior direccion recibida
rfRemote Recibido;								//ultima direccion recibida
#endif



/* 
 * Esto permite saber si se mantiene presionado un boton de un mando a distancia
 * Cada vez que se reciba una trama correcta de RF y coincida con un mando hay
 * que poner a 0 la variable ContMantenidoTimeOut
 * El mantenido NO FUNCIONA BIEN CON CIRCUITOS DE SERVOS ya que introducen mucho
 * ruido y el receptor no consigue decodificar bien la señal, provocando "ausencia"
 * de tramas correctas.
 */
#IFDEF RF_MANTENIDO
short RFmantenido = false;
int ContMantenidoTimeOut = 0;

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
#ENDIF

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
void GrabarMando(void){
byte x = 0;

#warning "En vez de grabar el valor de -Recibido- se puede pasar el valor a grabar a la funcion?"
#warning "Comprobar que funciona correctamente"

	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la grabacion
	
	for(x = FIRST_POS_TO_MOVE; (x >= LAST_POS_TO_MOVE)&&(x<=FIRST_POS_TO_MOVE); x--){
		write_eeprom(x+POS_TO_JUMP, read_eeprom(x));
	}
	
#ifdef GRABAR_DIRECCIONES
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
 * Lee los mandos de la EEPROM y los almacena en DirRF[]
 * Devuelve TRUE si hay algun mando configurado
*/
short LeerMandos(void){
	short Mando = FALSE;
	
	disable_interrupts(GLOBAL);	//no quiero que nada interrumpa la lectura
	
#warning "Comprobar"
	
//lee los mandos
#ifdef GRABAR_DIRECCIONES
	for(int x = 0; x<NUM_MANDOS_RF; x++){
		DirRF[x].Completo = 0;
		DirRF[x].Bytes.Lo = read_eeprom(POS_MEM_MANDOS_START + (x*POS_TO_JUMP) + RF_ADDR_LO);
		DirRF[x].Bytes.Hi = read_eeprom(POS_MEM_MANDOS_START + (x*POS_TO_JUMP) + RF_ADDR_HI);

		//si el mando leido es diferente a 0xFFFFFF quiere decir que hay algo grabado
		if((DirRF[x][y].Completo != 0) && (DirRF[x].Completo != 0xFFFFFF)){
			Mando = TRUE;
		}
	}
#else
	for(int x = 0; x<NUM_MANDOS_RF; x++){
		for(int y = 0; y<NUM_CANALES_RF; y++){
			DirRF[x][y].Completo = 0;
			DirRF[x][y].Bytes.Lo = read_eeprom(POS_MEM_MANDOS_START_RF + (x*POS_TO_JUMP) + (y*RF_SAVE_BYTES) + RF_BYTE_LO);
			DirRF[x][y].Bytes.Mi = read_eeprom(POS_MEM_MANDOS_START_RF + (x*POS_TO_JUMP) + (y*RF_SAVE_BYTES) + RF_BYTE_MI);
			DirRF[x][y].Bytes.Hi = read_eeprom(POS_MEM_MANDOS_START_RF + (x*POS_TO_JUMP) + (y*RF_SAVE_BYTES) + RF_BYTE_HI);
			
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
	for(int x = POS_MEM_MANDOS_START_RF; x <= RF_LAST_VALUE; x++){
		write_eeprom(x, 0xFF);
	}
}