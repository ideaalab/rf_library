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
 * >La funcion DataFrameComplete() que se encarga de la decodificacion se va
 * optimizando cada vez mas, pero las versiones anteriores se mantienen por
 * compatibilidad, pruebas y referencia. En caso de querer usar una version
 * anterior a la mas nueva hay que declarar un define con la version deseada:
 * #define RF_DECODE_V0
 * #define RF_DECODE_V1
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
 * 
 * <Si no se usa la libreria rf_rx_aux.c entonces RFmantenido no se activa solo,
 * hay que ponerlo a TRUE manualmente cuando recibamos una señal
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

#endif	/* RF_RX_H */