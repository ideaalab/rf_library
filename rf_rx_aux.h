/* =============================================================================
 *                  LIBERIA AUXILIAR DE RECECPCION RF
 * 
 * Autor: Martin Andersen
 * Compania: IDEAA Lab ( http://www.ideaalab.com )
 *
 * =============================================================================
 * QUE HACE ESTA LIBRERIA?
 * 
 * >Permite leer y grabar mandos en la memoria EEPROM
 * >Permite grabar solo la direccion del mando (2 bytes) y los canales se
 * deducen de la trama de datos recibida.
 * >Permite grabar toda la trama de datos como un canal (3 bytes)
 * >Permite saber si se mantiene presionado el boton de un mando
 * >Permite mover bloques de direcciones dentro de la EEPROM
 * =============================================================================
 * INTRODUCCION
 * 
 * >Se entiende por MANDO a un contenedor de CANALES. Cada mando puede tener uno
 * o mas canales.
 * -En el caso de almacenar solo la direccion, los canales siempre pertenecen al
 * mando fisico, ya que se deducen de la trama de datos.
 * -En el caso de almacenar toda la trama como si fuese un canal, cada mando se
 * podria entender como un "mando virtual", ya que creamos un mando donde los
 * canales pueden pertenecer fisicamente a varios mandos diferentes.
 * Es decir, si tenemos MemRF[1][2] los canales 1 y 2 del mando 1 pueden ser de
 * dos mandos fisicamente diferentes, pero estan unidos en este "mando virtual".
 * 
 * >Los mandos/canales se almacenan en la variable:
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
 * [1][0]: [M2/Ch1]
 * [2][0]: [M3/Ch1]
 * [3][0]: [M4/Ch1]
 * [4][0]: [M5/Ch1]
 * =============================================================================
 * COMO CONFIGURAR LA LIBRERIA?
 * 
 * >Esta libreria depende de rf_rx.c
 * 
 * >Hay que declarar un define POS_MEM_MANDOS_START_RF que indica donde
 * comienzan a escribirse los mandos en la EEPROM
 * #define POS_MEM_MANDOS_START_RF	10
 * 
 * >Hay que declarar un define NUM_MANDOS_RF que indique cuantos mandos
 * se pueden memorizar
 * #define NUM_MANDOS_RF	3
 * 
 * >Se puede declarar un define NUM_CANALES_RF que indique cuantos canales 
 * tiene cada mando. Si se declara NUM_CANALES_RF se asume que se graban los
 * canales de manera independiente.
 * #define NUM_CANALES_RF	4
 * 
 * >Si se quiere usar la funcion de "mantener" el boton del mando, hay que
 * declarar un define RF_MANTENIDO. RF_MANTENIDO usa el timer2
 * #define RF_MANTENIDO
 * 
 * >Desde que se recibe la se�al RF_MANTENIDO espera otra se�al como maximo 200mS.
 * Despues de ese tiempo se considera que el boton del mando se "solto". Si se
 * quiere usar otro tiempo hay que declarar TIME_OUT_RF_MANTENIDO con el valor
 * deseado en mS. Este valor mientras mayor es, mas tolerante se vuelve a cortes
 * e interferencias en la recepcion, pero tambien introduce mas retardo en la
 * recepcion.
 * #define TIME_OUT_RF_MANTENIDO	500
 * 
 * >Llamar a la funcion RF_mantenido_init() al inicio del programa para que se
 * detecte cuando se mantiene un canal.
 * =============================================================================
 * VARIABLES
 * 
 * >Recibido: almacena la ultima trama de datos recibida.
 * 
 * >RecibAnterior: almacena la trama de datos anterior recibida. Sirve para
 * compararla con "Recibido" y ver si son iguales. Util para sincronizar.
 * 
 * >MemRF[x] / MemRF[x][y]: contiene los mandos que se han sincronizado y
 * guardado en la EEPROM. Es unidimensional (x: mando) cuando solo se guarda la
 * direccion, y es bidimensional (x: mando, y: canal) cuando se guarda toda la
 * trama como canal independiente.
 * 
 * >ButtonMatch[x]: contiene los canales activos que se han recibido. Los bits
 * independientes indican los canales activos y "x" los mandos
 * 
 * >flagSync: indica si estamos en modo sincronizacion.
 * 
 * >SyncStep: cuando estamos grabando varios canales, esta variable nos indica
 * en que paso de sincronizacion estamos.
 * 
 * >MandoVirtual[x]: Sirve para sincronizar varios mandos a la vez. Es
 * equivalente a un "mando virtual" que consta de "x" canales. Se almacena cada
 * canal como una trama de datos completa. Una vez que se ha llenado con los
 * canales a sincronizar. Esta variable la usa la funcion GrabarBloqueMandos().
 * =============================================================================
 * FUNCIONES
 * 
 * > RF_mantenido_init(): inicializa los perifericos necesarios para poder
 * detectar cuando un canal RF se esta manteniendo.
 * >AnalizarRF(rfRemote): compara los datos recibidos con el contenido de MemRF.
 * Guarda las coincidencias en ButtonMatch[]. Devuelve TRUE si hubo alguna
 * coincidencia.
 * > GrabarMando(): mueve todos los mandos grabados una posicion (2 o 3 bytes
 * para atras y graba el mando que este en la variable "Recibido". El mando mas
 * antiguo se pierde.
 * > GrabarMando(rfRemote): igual que la anterior, pero se le pasa la direccion
 * de la variable que queremos que se grabe en vez de utilizar el valor que esta
 * en "Recibido".
 * EJ: GrabarMando(&Recibido);
 * > GrabarBloqueMandos(): graba todos los canales a la vez. Los valores
 * grabados son los que hay en MandoVirtual.
 * > GrabarBloqueMandos(rfRemote): graba todos los canales a la vez. Hay que
 * pasarle la direccion de una variable que contenga todos los canales a grabar.
 * EJ: GrabarBloqueMandos(&MandoVirtual);
 * > LeerMandos(): lee todos los mandos que hay en la EEPROM y los almacena en
 * MemRF[x] / MemRF[x][y]
 * >BorrarMandos(): borra la EEPROM de todos los mandos que se habian guardado
 * previamente. Tambien limpia la variable MemRF
 * >RestartRFmantenido(): reinicia contadores de RF mantenido. Si se quiere
 * simular una pulsacion RF (RF mantenido se encarga de apagar el led de se�al),
 *  antes de llamar a esta funcion, tambien hay que activar su variable
 * RFmantenido = TRUE;
 * =============================================================================
 * RECURSOS USADOS
 * 
 * >Si se usa RF_MANTENIDO requiere el Timer2 para funcionar
 * ========================================================================== */

#ifndef RF_RX_AUX_H
#define	RF_RX_AUX_H

#ifndef NUM_MANDOS_RF
#ERROR "Hay que declarar NUM_MANDOS_RF para que la libreria funcione"
#endif

#ifdef NUM_CANALES_RF
	#define GRABAR_CANALES
	#warning "Los canales de los mandos se graban de manera independiente (3 bytes cada uno)"
#else
	#define GRABAR_DIRECCIONES
	#warning "Los canales de los mandos no se graban, solo se graba la direccion del mando (2 bytes)"
#endif

#ifndef POS_MEM_MANDOS_START_RF
#ERROR "Hay que declarar POS_MEM_MANDOS_START_RF para que la libreria funcione"
#endif

#ifdef RF_MANTENIDO
#warning "Se detecta cuando se mantiene presionado un boton del mando"

/*#define T_T2							10	//interrupcion timer 2 en mS
#ifndef TIME_OUT_RF_MANTENIDO
#define TIME_OUT_RF_MANTENIDO			200	//cuanto tiempo tiene que pasar para que se interprete como que no esta mantenido
#endif
#define VUELTAS_TIME_OUT_RF_MANTENIDO	(TIME_OUT_RF_MANTENIDO / T_T2)

#else
#warning "No se detecta cuando se mantiene presionado un boton del mando"
#endif*/

/* DEFINES */
#define RF_ADDR_LO	0
#define RF_ADDR_HI	1

#define RF_BYTE_LO	0
#define RF_BYTE_MI	1
#define RF_BYTE_HI	2

//cuantos bytes se guardan por cada mando/canal
#ifdef GRABAR_DIRECCIONES
#define RF_SAVE_BYTES	2
#else
#define RF_SAVE_BYTES	3
#endif

//usadas para cuando se graba un mando
#define LAST_POS_TO_MOVE	(POS_MEM_MANDOS_START_RF)

#ifdef GRABAR_DIRECCIONES	//se graban solo direcciones (2 bytes))
#warning "Comprobar si esta bien"
#define FIRST_POS_TO_MOVE		(POS_MEM_MANDOS_START_RF + ((NUM_MANDOS_RF - 1) * RF_SAVE_BYTES) - 1)
#define POS_TO_JUMP				(RF_SAVE_BYTES)
#define	POS_MEM_MANDOS_END_RF	(POS_MEM_MANDOS_START_RF + (NUM_MANDOS_RF * RF_SAVE_BYTES) - 1)
#else						//se graban los canales
#define FIRST_POS_TO_MOVE		(POS_MEM_MANDOS_START_RF + ((NUM_MANDOS_RF - 1) * NUM_CANALES_RF * RF_SAVE_BYTES) - 1)
#define POS_TO_JUMP				(RF_SAVE_BYTES * NUM_CANALES_RF)
#define	POS_MEM_MANDOS_END_RF	(POS_MEM_MANDOS_START_RF + (NUM_MANDOS_RF * NUM_CANALES_RF * RF_SAVE_BYTES) - 1)
#endif

/* VARIABLES */
short flagSync = false;		//indica si estamos grabando un mando

int ButtonMatch[NUM_MANDOS_RF];//indica que botones se presionaron de cada mando (max 8 botones por mando, 1bit cada boton)

#ifdef GRABAR_DIRECCIONES
rfAddr MemRF[NUM_MANDOS_RF];	//direcciones de los mandos almacenados
#else
rfRemote MemRF[NUM_MANDOS_RF][NUM_CANALES_RF];	//direcciones de los mandos/botones almacenados
#endif

rfRemote RecibAnterior;							//anterior direccion recibida
rfRemote Recibido;								//ultima direccion recibida

#ifdef GRABAR_CANALES
#if NUM_CANALES_RF > 1
int SyncStep = 0;						//en que paso de sincronizacion estamos
rfRemote MandoVirtual[NUM_CANALES_RF];	//variable para retener en memoria varias direcciones RF y poder sincronizar todos los canales al mismo tiempo
#endif
#endif

/* PROTOTIPOS PUBLICOS */
short AnalizarRF(void);
short AnalizarRF(rfRemote* c);
void GrabarMando(void);
void GrabarMando(rfRemote* DatosRF);
#ifdef GRABAR_CANALES
#if NUM_CANALES_RF > 1
void GrabarBloqueMandos(void);
void GrabarBloqueMandos(rfRemote* DatosRF);
#endif
#endif
short LeerMandos(void);
void BorrarMandos(void);

/* PROTOTIPOS PRIVADOS */
void Timer2_isr(void);
void MoverBloque(int from, int to, int offset);

#endif	/* RF_RX_AUX_H */

