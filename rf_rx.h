/* =============================================================================
 *                         LIBERIA DE RECECPCION RF
 * 
 * Autor: Martin Andersen
 * Compania: IDEAA Lab ( http://www.ideaalab.com )
 * 
 * =============================================================================
 * QUE HACE ESTA LIBRERIA?
 * 
 * >Permite decodificar se�ales RF codificadas con encoders como PT2240B, PT2260,
 * PT2262, PT2264, etc.
 * =============================================================================
 * INTRODUCCION
 * 
 * Esta libreria utiliza un timer interno para contar pulsos recibidos. Estos
 * pulsos se reciben por el pin de interrupcion externa.
 * La trama de datos consiste en 24 bits + 1 sync. Cada bit es un pulso.
 * La suma de 24 bits + 1 sync componen una trama completa.
 * CERO: 25% duty del pulso
 * UNO: 75% duty del pulso
 * SYNC: 3.125% duty del pulso
 * =============================================================================
 * COMO CONFIGURAR LA LIBRERIA
 * 
 * >Por defecto la libreria funciona con el Timer 1. Si queremos usar el Timer 0
 * hay que declarar un define RF_RX_TIMER0 antes de llamar a la libreria.
 * #define RF_RX_TIMER0
 * 
 * >Desde que se recibe la señal se espera otra señal como maximo 200mS.
 * Despues de ese tiempo se considera que el boton del mando se "solto". Si se
 * quiere usar otro tiempo hay que declarar RF_MANTENIDO_TIME_OUT con el valor
 * deseado en mS. Este valor mientras mayor es, mas tolerante se vuelve a cortes
 * e interferencias en la recepcion, pero tambien introduce mas retardo en la
 * recepcion.
 * #define RF_MANTENIDO_TIME_OUT	500
 * 
 * =============================================================================
 * VARIABLES
 * 
 * >rfBuffer: es el buffer de recepcion. Contiene los valores que se van
 * recibiendo. Comprobarlo cuando DataReady == TRUE.
 * =============================================================================
 * FUNCIONES
 * 
 * > EncenderRF(): enciende la recepcion de RF (activa las interrupciones).
 * 
 * > ApagarRF(): apaga la recepcion de RF (desactiva las interrupciones).
 * 
 * > DataReady(): devuelve TRUE cuando hemos recibido una trama completa.
 * 
 * > GetRFTime(): devuelve (int32) el tiempo en mS de la ultima trama completa.
 * Comprobar este valor cuando DataReady == TRUE.
 * =============================================================================
 * COMO USAR LA LIBRERIA
 * 
 * >Primero hay que inicializar la libreria llamando a EncenderRF().
 * >Luego hay que comprobar DataReady constantemente en el main. Cuando devuelva
 * TRUE quiere decir que tenemos la trama lista en rfBuffer.
 * 
 *		if(DataReady() == TRUE){			//check if there is new data
 *			//data received is on rfBuffer
 *			... execute instructions
 *		}
 * =============================================================================
 * RECURSOS USADOS
 * 
 * >Utiliza el Timer1 por defecto, o el Timer0 si se declara RF_RX_TIMER0
 * 
 * >Utiliza la interrupcion externa
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

/* VARIABLES GLOBALES */
short flagPulseSync = FALSE;
short flagPulse = FALSE;			//indica si hay un pulso para contabilizar
short FallingFlag = FALSE;			//indica si hubo un flanco de bajada
short RisingFlag = FALSE;			//inidca si hubo un flanco de subida
short RFmantenido = FALSE;			//indica si se esta manteniendo el pulsador de un mando a distancia
rfRemote rfBuffer;					//buffer de recepcion
int CountedBits = 0;				//numero de bits contados
//int Duty = 0;						//duty cycle del pulso
long Cycles = 0;					//vueltas del timer0/1
long CyclesSinceLastValidFrame = 0;	//vueltas del timer0/1 desde el ultimo dato valido
long CountedCycles = 0;				//vueltas del timer0/1 almacenadas para que no cambien en una posible interrupcion
long HighPulseDuration = 0;			//duracion de la parte alta del pulso
long TotalPulseDuration = 0;		//duracion del pulso completo (alta + baja))

#ifdef RF_RX_TIMER0
int Tmr0 = 0;
#else
long Tmr1 = 0;
#endif

int32 LastFrameDuration = 0;		//duracion de la ultima trama recibida
int32 TotalFrameDuration = 0;		//duracion de todos los pulsos recibidos
int32 TimeSinceLastValidFrame = 0;	//tiempo transcurrido desde el ultimo dato valido
	
/* PROTOTIPOS */
void EncenderRF(void);
void ApagarRF(void);
short DataFrameComplete(void);
void CalcTimes(void);
short DataReady(void);
int32 GetRFTime(void);
void RestartRFmantenido(void);

#endif	/* RF_RX_H */