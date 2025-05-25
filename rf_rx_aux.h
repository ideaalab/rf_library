/* =============================================================================
 *                  LIBERIA AUXILIAR DE RECECPCION RF
 * 
 * Autor: Martin Andersen
 * Compania: IDEAA Lab ( http://www.ideaalab.com )
 *
 * ========================================================================== */

#ifndef RF_RX_AUX_H
#define	RF_RX_AUX_H

#ifndef NUM_MANDOS_RF
#error "Hay que declarar NUM_MANDOS_RF para que la libreria funcione"
#endif

#ifdef NUM_CANALES_RF
	#define GRABAR_CANALES
	#warning "Los canales de los mandos se graban de manera independiente (3 bytes cada uno)"
#else
	#define GRABAR_DIRECCIONES
	#warning "Los canales de los mandos no se graban, solo se graba la direccion del mando (2 bytes)"

	#ifndef RF_ADDR_BITS
		#warning "Hay que declarar el numero de bits de las direcciones (1 a 16)"
	#else
		#define RF_ADDR_MASK	((1<<RF_ADDR_BITS)-1)
	#endif
#endif

#ifndef POS_MEM_MANDOS_START_RF
#error "Hay que declarar POS_MEM_MANDOS_START_RF para que la libreria funcione"
#endif

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
#define FIRST_POS_TO_MOVE		(POS_MEM_MANDOS_START_RF + ((NUM_MANDOS_RF - 1) * RF_SAVE_BYTES) - 1)
#define POS_TO_JUMP				(RF_SAVE_BYTES)
#define	POS_MEM_MANDOS_END_RF	(POS_MEM_MANDOS_START_RF + (NUM_MANDOS_RF * RF_SAVE_BYTES) - 1)
#else						//se graban los canales
#define FIRST_POS_TO_MOVE		(POS_MEM_MANDOS_START_RF + ((NUM_MANDOS_RF - 1) * NUM_CANALES_RF * RF_SAVE_BYTES) - 1)
#define POS_TO_JUMP				(RF_SAVE_BYTES * NUM_CANALES_RF)
#define	POS_MEM_MANDOS_END_RF	(POS_MEM_MANDOS_START_RF + (NUM_MANDOS_RF * NUM_CANALES_RF * RF_SAVE_BYTES) - 1)
#endif

/* CONSTANTES */
#define MANTENIDO_RISING	1
#define MANTENIDO_FALLING	2

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
short AnalizarRF(rfRemote* DatosRF);
int8 FlancoMantenido(void);
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
#if definedinc(STDOUT)
void PrintMem(void);
#endif

/* PROTOTIPOS PRIVADOS */
void MoverBloque(int from, int to, int offset);

#endif	/* RF_RX_AUX_H */

