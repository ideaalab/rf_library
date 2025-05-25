/* =============================================================================
 *                         LIBERIA DE RECECPCION RF
 * 
 * Autor: Martin Andersen
 * Compania: IDEAA Lab ( http://www.ideaalab.com )
 * 
 * ========================================================================== */

#ifndef RF_RX_H
#define	RF_RX_H

#include "rf_remotes.h"

/* COMPROBACIONES DE COMPATIBILIDAD */
#ifdef RF_TIMER0
	#error "Cambiar RF_TIMER0 por RF_RX_TIMER0"
#endif
#ifdef RX_TIMER0
	#error "Cambiar RX_TIMER0 por RF_RX_TIMER0"
#endif
#ifdef RF_COUNT_TIME
	#error "Cambiar RF_COUNT_TIME por RF_RX_COUNT_TIME"
#endif
#ifdef RF_RX_COUNT_TIME
	#warning "Ya no es necesario declarar esto. El tiempo siempre se cuenta"
#endif
#ifdef RF_MANTENIDO
	#warning "Ya no es necesario declarar esto. RFmantenido siempre esta activo"
#endif

#bit INTEDG = getenv("bit:INTEDG")

//definir prioridad de interrupciones
#ifdef RF_RX_TIMER0
	#priority timer0,ext
#else
	#priority timer1,ext
#endif

#ifndef RF_MANTENIDO_TIME_OUT
#define RF_MANTENIDO_TIME_OUT	200	//mS
#endif

/* CONSTANTS */
#define FALLING		0	//falling edge
#define RISING		1	//rising edge

#define MIN_PULSE	300	//minimum duration allowed for received pulse, in uS (theoreticaly is 16*ALFA)
#define BUFFER_SIZE	24	//length of the data stream received
#define RF_MANTENIDO_TIME_OUT_US	(RF_MANTENIDO_TIME_OUT * 1000)	//tiempo en uS para que se considere que se ha dejado de pulsar el boton

#ifdef RF_RX_TIMER0
#define TIMER_MAX_VAL		256
#define GET_TIMER_VAL		get_timer0()
#define SET_TIMER_VAL(x)	set_timer0(x)
#else
#define TIMER_MAX_VAL		65536
#define GET_TIMER_VAL		get_timer1();
#define SET_TIMER_VAL(x)	set_timer1(x)
#endif

/* VARIABLES GLOBALES */
int1 flagPulse = FALSE;				//indica si hay un pulso para contabilizar
int1 flagPulseSync = FALSE;			//indica si se recibio el pulso sync
int1 prevRFmantenido = FALSE;		//lo usamos para saber cuando inica o acaba una pulsacion
int1 RFmantenido = FALSE;			//indica si se esta manteniendo el pulsador de un mando a distancia

int8 CountedBits = 0;				//numero de bits contados

//asumimos que un pulso (H+L) no puede ser mayor a 65535
#ifdef RF_RX_TIMER0
int16 Cycles = 0;					//vueltas del timer0
int16 CountedCycles = 0;			//vueltas del timer0 almacenadas para que no cambien en una posible interrupcion
#else
int8 Cycles = 0;					//vueltas del timer1
int8 CountedCycles = 0;				//vueltas del timer1 almacenadas para que no cambien en una posible interrupcion
#endif
int16 HighPulseDuration = 0;		//duracion de la parte alta del pulso
int16 TotalPulseDuration = 0;		//duracion del pulso completo (alta + baja))

#ifdef RF_RX_TIMER0
int8 TmrVal = 0;
#else
int16 TmrVal = 0;
#endif

int32 LastFrameDuration = 0;		//duracion de la ultima trama recibida
int32 TotalFrameDuration = 0;		//duracion de todos los pulsos recibidos
int32 TimeSinceLastValidFrame = 0;	//tiempo transcurrido desde el ultimo dato valido

rfRemote rfBuffer;					//buffer de recepcion
rfRemote rfReceived;				//data received

/* PROTOTIPOS */
void EXT_isr(void);
void RF_timer_isr(void);
void EncenderRF(void);
void ApagarRF(void);
short DataFrameComplete(void);
short CalcTimes(void);
short DataReady(void);
int32 GetRFTime(void);
void RestartRFmantenido(void);
void LimpiarRF(void);

#endif	/* RF_RX_H */