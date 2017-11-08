/* 
 * File:   rf_rx_aux.h
 * Author: Martin
 *
 * Created on 27 de abril de 2017, 12:47
 */

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
#warning "Los canales de los mandos no se graban, solo se graba la direccion del mando"
#endif

#ifndef POS_MEM_MANDOS_START_RF
#ERROR "Hay que declarar POS_MEM_MANDOS_START_RF para que la libreria funcione"
#endif

#ifdef RF_MANTENIDO
#warning "Se detecta cuando se mantiene presionado un boton del mando"

#define T_T2							10	//interrupcion timer 2 en mS
#ifndef TIME_OUT_RF_MANTENIDO
#define TIME_OUT_RF_MANTENIDO			200	//cuanto tiempo tiene que pasar para que se interprete como que no esta mantenido
#endif
#define VUELTAS_TIME_OUT_RF_MANTENIDO	(TIME_OUT_RF_MANTENIDO / T_T2)

#else
#warning "No se detecta cuando se mantiene presionado un boton del mando"
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

#if NUM_CANALES_RF > 1
int SyncStep = 0;						//en que paso de sincronizacion estamos
rfRemote MandoVirtual[NUM_CANALES_RF];	//variable para retener en memoria varias direcciones RF y poder sincronizar todos los canales al mismo tiempo
#endif

#ifdef RF_MANTENIDO
short RFmantenido = false;
int ContTimeOutRFmantenido = 0;
#endif

/* PROTOTIPOS PUBLICOS */
void RF_mantenido_init(void);
short AnalizarRF(void);
short AnalizarRF(rfRemote* c);
void GrabarMando(void);
void GrabarMando(rfRemote* DatosRF);
#if NUM_CANALES_RF > 1
void GrabarBloqueMandos(void);
void GrabarBloqueMandos(rfRemote* DatosRF);
#endif
short LeerMandos(void);
void BorrarMandos(void);
/* PROTOTIPOS PRIVADOS */
void Timer2_isr(void);
void MoverBloque(int from, int to, int offset);

#endif	/* RF_RX_AUX_H */

