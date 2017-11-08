/*
 * QUE HACE ESTA LIBRERIA?
 * 
 * #Permite leer y grabar mandos en la memoria EEPROM
 * #Permite grabar solo la direccion del mando (2 bytes) y los canales se
 * deducen de la trama de datos recibida.
 * #Permite grabar toda la trama de datos como un canal (3 bytes)
 * #Permite saber si se mantiene presionado el boton de un mando
 * #Permite mover bloques de direcciones dentro de la EEPROM
 * =============================================================================
 * INTRODUCCION
 * 
 * #Se entiende por MANDO a un contenedor de CANALES. Cada mando puede tener uno
 * o mas canales.
 * -En el caso de almacenar solo la direccion, los canales siempre pertenecen al
 * mando fisico, ya que se deducen de la trama de datos.
 * -En el caso de almacenar toda la trama como si fuese un canal, cada mando se
 * podria entender como un "mando virtual", ya que creamos un mando donde los
 * canales pueden pertenecer fisicamente a varios mandos diferentes.
 * Es decir, si tenemos MemRF[1][2] los canales 1 y 2 del mando 1 pueden ser de
 * dos mandos fisicamente diferentes, pero estan unidos en este "mando virtual".
 * 
 * #Los mandos/canales se almacenan en la variable:
 * -MemRF[Mando]: ocupa 2 bytes * NUM_MANDOS_RF
 * -MemRF[Mando][Canal]: ocupa 3 bytes * NUM_CANALES_RF * NUM_MANDOS
 * =============================================================================
 * COMO SE ALMACENAN LOS MANDOS?
 * 
 * Se pueden almacenar mandos y canales de dos maneras:
 * 
 * --- DIRECCION DEL MANDO ---
 * -Solo se almacena la direccion del mando, y los canales de deducen de la
 * trama RF recibida
 * MemRF[Mando] (2 bytes c/u, los canales dependen de como se interprete la trama)
 * 
 * Ej: MemRF[3] (3 mandos)
 * MemRF[0]: [M1]
 * MemRF[1]: [M2]
 * MemRF[2]: [M3]
 * 
 * --- TRAMA COMPLETA ---
 * -Cada canal como un mando nuevo. Almacena la trama RF entera para cada canal
 * MemRF[Mando][Canal] (3 bytes c/u)
 * 
 * Ej: MemRF[3][4] (3 mandos de 4ch c/u)
 * MemRF	 [x][0]		 [x][1]		 [x][2]		 [x][3]
 * [0][y]:	[M1/Ch1]	[M1/Ch2]	[M1/Ch3]	[M1/Ch4]
 * [1][y]:	[M2/Ch1]	[M2/Ch2]	[M2/Ch3]	[M2/Ch4]
 * [2][y]:	[M3/Ch1]	[M3/Ch2]	[M3/Ch3]	[M3/Ch4]
 * 
 * Ej: MemRF[5][1] (5 mandos de 1ch cada uno)
 * MemRF	[x][0]
 * [0][0]: [M1/Ch1]
 * [1][0]: [M1/Ch2]
 * [2][0]: [M1/Ch3]
 * [3][0]: [M1/Ch4]
 * [4][0]: [M1/Ch5]
 * =============================================================================
 * COMO CONFIGURAR LA LIBRERIA?
 * 
 * #Esta libreria depende de rf_rx.c
 * 
 * #Hay que declarar un define POS_MEM_MANDOS_START_RF que indica donde
 * comienzan a escribirse los mandos en la EEPROM
 * #define POS_MEM_MANDOS_START_RF	10
 * 
 * #Hay que declarar un define NUM_MANDOS_RF que indique cuantos mandos
 * se pueden memorizar
 * #define NUM_MANDOS_RF	3
 * 
 * #Se puede declarar un define NUM_CANALES_RF que indique cuantos canales 
 * tiene cada mando. Si se declara NUM_CANALES_RF se asume que se graban los
 * canales de manera independiente.
 * #define NUM_CANALES_RF	4
 * 
 * #Si se quiere usar la funcion de "mantener" el boton del mando, hay que
 * declarar un define RF_MANTENIDO. RF_MANTENIDO usa el timer2
 * #define RF_MANTENIDO
 * 
 * #Desde que se recibe la señal RF_MANTENIDO espera otra señal como maximo 200mS.
 * Despues de ese tiempo se considera que el boton del mando se "solto". Si se
 * quiere usar otro tiempo hay que declarar TIME_OUT_RF_MANTENIDO con el valor
 * deseado en mS. Este valor mientras mayor es, mas tolerante se vuelve a cortes
 * e interferencias en la recepcion, pero tambien introduce mas retardo en la
 * recepcion.
 * #define TIME_OUT_RF_MANTENIDO	500
 * 
 * #Llamar a la funcion RF_mantenido_init() al inicio del programa para que se
 * detecte cuando se mantiene un canal.
 * 
 * =============================================================================
 * VARIABLES
 * 
 * -Recibido: almacena la ultima trama de datos recibida
 * -RecibAnterior: almacena la trama de datos anterior recibida. Sirve para
 * compararla con "Recibido" y ver si son iguales. Util para sincronizar.
 * -MemRF[x] / MemRF[x][y]: contiene los mandos que se han sincronizado y
 * guardado en la EEPROM. Es unidimensional (x: mando) cuando solo se guarda la
 * direccion, y es bidimensional (x: mando, y: canal) cuando se guarda toda la
 * trama como canal independiente.
 * -ButtonMatch[x]: contiene los canales activos que se han recibido. Los bits
 * independientes indican los canales activos y "x" los mandos
 * -flagSync: indica si estamos en modo sincronizacion
 * -SyncStep: cuando estamos grabando varios canales, esta variable nos indica
 * en que paso de sincronizacion estamos.
 * -MandoVirtual[x]: Sirve para sincronizar varios mandos a la vez. Es
 * equivalente a un "mando virtual" que consta de "x" canales. Se almacena cada
 * canal como una trama de datos completa. Una vez que se ha llenado con los
 * canales a sincronizar. Esta variable la usa la funcion GrabarBloqueMandos().
 * =============================================================================
 * FUNCIONES
 * 
 * - RF_mantenido_init(): inicializa los perifericos necesarios para poder
 * detectar cuando un canal RF se esta manteniendo.
 * -AnalizarRF(rfRemote): compara los datos recibidos con el contenido de MemRF.
 * Guarda las coincidencias en ButtonMatch[]. Devuelve TRUE si hubo alguna
 * coincidencia.
 * - GrabarMando(): mueve todos los mandos grabados una posicion (2 o 3 bytes
 * para atras y graba el mando que este en la variable "Recibido". El mando mas
 * antiguo se pierde.
 * - GrabarMando(rfRemote): igual que la anterior, pero se le pasa la direccion
 * de la variable que queremos que se grabe en vez de utilizar el valor que esta
 * en "Recibido".
 * EJ: GrabarMando(&Recibido);
 * - GrabarBloqueMandos(): graba todos los canales a la vez. Los valores
 * grabados son los que hay en MandoVirtual.
 * - GrabarBloqueMandos(rfRemote): graba todos los canales a la vez. Hay que
 * pasarle la direccion de una variable que contenga todos los canales a grabar.
 * EJ: GrabarBloqueMandos(&MandoVirtual);
 * - LeerMandos(): lee todos los mandos que hay en la EEPROM y los almacena en
 * MemRF[x] / MemRF[x][y]
 * -BorrarMandos(): borra la EEPROM de todos los mandos que se habian guardado
 * previamente. Tambien limpia la variable MemRF
 * ---------------------------------------------------------------------------*/

#include "rf_remotes.h"
#include "rf_rx_aux.h"

/* 
 * Esto permite saber si se mantiene presionado un boton de un mando a distancia
 * Cada vez que se reciba una trama correcta de RF y coincida con un mando hay
 * que poner a 0 la variable ContMantenidoTimeOut
 * El mantenido NO FUNCIONA BIEN CON CIRCUITOS DE SERVOS ya que introducen mucho
 * ruido y el receptor no consigue decodificar bien la seÃ±al, provocando "ausencia"
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
 * -1 Canal: x ROM
 * -4 Canales: 124 ROM
 */
short AnalizarRF(void){
short Match = FALSE;	//indica si hubo alguna coincidencia
	
	ApagarRF();			//apago RF para que no interfieran las interrupciones

#ifdef RF_MANTENIDO
	ContTimeOutRFmantenido = 0;
#endif
	
#ifdef GRABAR_DIRECCIONES
	#warning "Sin implementar"
#else
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
	short Respuesta = Match && !RFmantenido; //solo devuelvo true si Match = TRUE y RFmantenido = FALSE
	
	if(Match == TRUE){
		RFmantenido = TRUE;
	}
	set_timer2(0);
	ContTimeOutRFmantenido = 0;
	EncenderRF();	//vuelvo a enceder RF
	return(Respuesta);
	#else
	EncenderRF();	//vuelvo a enceder RF
	return(Match);
	#endif
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
 * -1 Canal: x ROM
 * -4 Canales: 141 ROM
 */
short AnalizarRF(rfRemote* DatosRF){
short Match = FALSE;	//indica si hubo alguna coincidencia
	
	ApagarRF();			//apago RF para que no interfieran las interrupciones

#ifdef RF_MANTENIDO
	ContTimeOutRFmantenido = 0;
#endif
	
#ifdef GRABAR_DIRECCIONES
	#warning "Sin implementar"
#else
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
	short Respuesta = Match && !RFmantenido; //solo devuelvo true si Match = TRUE y RFmantenido = FALSE
	
	if(Match == TRUE){
		RFmantenido = TRUE;
	}
	set_timer2(0);
	ContTimeOutRFmantenido = 0;
	EncenderRF();	//vuelvo a enceder RF
	return(Respuesta);
	#else
	EncenderRF();	//vuelvo a enceder RF
	return(Match);
	#endif
#endif
}

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
 * Es mas eficiente:
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
 * Misma funcion que la de arriba, pero a esta hay que pasarle
 * los valores a grabar en la EEPROM.
 * Se llama asi: GrabarMando(&Recibido);
 * Es menos eficiente:
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
#warning "El nombre de un array hace referencia a la primera direccion de ese array = &array[0], pero aqui no funciona"
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