/* 
 * File:   rf_rx.h
 * Author: Martin
 *
 * Created on 03 de mayo de 2016, 19:50
 * 
 * Modificaciones:
 * 01/12/2016:
 * -Se cambia constante RF_TIMER0 por RF_RX_TIMER0
 * -Se cambia constante RF_COUNT_TIMER por RF_RX_COUNT_TIME
 * -Se cambian formato de text (mayusculas/minusculas) para hacerlo estandar
 * 
 * 30/11/2016:
 * -Se elimina funcion rf_in_init() y se sustituye por EncenderRF()
 */

#ifndef RF_RX_H
#define	RF_RX_H

#include "rf_remotes.h"

/* COMPROBACIONES DE COMPATIBILIDAD */
#if getenv("CLOCK") == 4000000
#warning "Las pruebas sugieren que el receptor no funciona a 4Mhz"
#endif

#ifdef RF_TIMER0
	#error Cambiar RF_TIMER0 por RF_RX_TIMER0
#endif
#ifdef RX_TIMER0
	#error Cambiar RX_TIMER0 por RF_RX_TIMER0
#endif
#ifdef RF_COUNT_TIME
	#error Cambiar RF_COUNT_TIME por RF_RX_COUNT_TIME
#endif

#bit INTEDG = getenv("bit:INTEDG")

//define interrupt priority
#ifdef RF_RX_TIMER0
	#priority ext,timer0
#else
	#priority ext,timer1
#endif

/* CONSTANTS */
#define FALLING		0	//falling edge
#define RISING		1	//rising edge

#define MIN_PULSE	300	//minimum duration allowed for received pulse, in uS (theoreticaly is 16*ALFA)
#define BUFFER_SIZE	24	//length of the data stream received

//--- Duty cicle
#define	MIN_ZERO	15		
#define ZERO		25		//theoretical
#define MAX_ZERO	35

#define	MIN_ONE		65
#define ONE			75		//theoretical
#define MAX_ONE		85

#define MIN_SYNC	1
#define SYNC		3.125	//theoretical
#define MAX_SYNC	5

/* VARIABLES GLOBALES */
short flagPulse = FALSE;	//indicates a pulse edge
rfRemote rfBuffer;			//reception buffer
int CountedBits = 0;		//number of counted bits
int Duty = 0;				//pulse duty cycle
long HighDuration = 0;		//duration of the high part of the pulse
long TotalDuration = 0;		//duration of the pulse (high & low)
#ifdef RF_RX_COUNT_TIME
	int32 TotalTime = 0;	//duration of all received pulses, from first to last
#endif

#ifdef RF_RX_TIMER0
	int Cycles = 0;			//cicles through timer0
#endif
	
/* PROTOTIPOS */
void EncenderRF(void);
void ApagarRF(void);
short DataFrameComplete(void);
short DataReady(void);

#endif	/* RF_RX_H */