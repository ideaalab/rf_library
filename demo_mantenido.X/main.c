#include <16F1825.h>
#device adc=8
#use delay(clock=4000000)

#define DEBUG

#FUSES INTRC_IO, NOWDT, PUT, NOMCLR, NOPROTECT, NOCPD, BROWNOUT, NOCLKOUT, NOIESO, NOFCMEN
#FUSES NOWRT, PLL_SW, NOSTVREN, BORV19, NODEBUG, NOLVP

#use fast_io(a)               //se accede al puerto a como memoria
#byte PORTA	= getenv("SFR:PORTA")

#define	PRESIONADO	0

//pins
#bit SERIAL_TX	= PORTA.0			//serial out
#bit A1			= PORTA.1			//NC
#bit RF			= PORTA.2			//RF in
#bit BTN		= PORTA.3			//btn
#bit LED1		= PORTA.4			//led 1
#bit LED2		= PORTA.5			//led 2

//defines
#define P_SERIAL_TX	PIN_A0			//O
#define P_A1		PIN_A1			//I
#define P_RF		PIN_A2			//I
#define P_BTN		PIN_A3			//I
#define P_LED1		PIN_A4			//O
#define P_LED2		PIN_A5			//O

//Bits			    543210
#define TRIS_A	0b00001110			//define cuales son entradas y cuales salidas
#define WPU_A	0b00001000			//define los weak pull up

/* VARIABLES */
short HayMandos;					//indica si hay algun mando grabado en memoria

/* CONSTANTES PARA RF */
#define RF_RX_TIMER0				//usamos el timer0 para RF
#define NUM_MANDOS_RF			3	//permite memorizar 3 mandos
#define NUM_CANALES_RF			1	//vamos a usar el btn mantenido por RF de RF_RX_AUX
#define RF_MANTENIDO_TIME_OUT	150	//cuanto tiempo tiene que pasar para que se interprete como que no esta mantenido (en mS)
#define POS_MEM_MANDOS_START_RF	0	//a partir de aqui se graban los mandos

#ifdef DEBUG
#use rs232(baud=250000, xmit=P_SERIAL_TX, ERRORS)
#endif

#include "..\rf_rx.c"
#include "..\rf_rx_aux.c"

void ComprobarRF(void);
/* -------------------------------- */

void main(void){
	setup_oscillator(OSC_4MHZ|OSC_PLL_OFF);	//configura oscilador interno
	setup_wdt(WDT_OFF);						//configuracion wdt
	//setup_timer_0(T0_INTERNAL|T0_DIV_1);		//lo configura rf_in_init()
	//setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);	//
	//setup_timer_2(T2_DIV_BY_64,249,5);		//
	setup_dac(DAC_OFF);						//configura DAC
	setup_adc(ADC_CLOCK_INTERNAL);			//configura ADC
	setup_adc_ports(NO_ANALOGS);			//configura ADC
	//setup_ccp1(CCP_COMPARE_INT);				//lo configura rf_in_init()
	setup_spi(SPI_DISABLED);				//configura SPI
	//setup_uart(FALSE);					//configura UART
	setup_vref(VREF_OFF);					//no se usa voltaje de referencia
	setup_comparator(NC_NC_NC_NC);			//comparador apagado
	
	//enable_interrupts(GLOBAL);				//lo habilitan otras funciones
	
	set_tris_a(TRIS_A);						//configura pines
	port_a_pullups(WPU_A);					//configura weak pull ups
	
	output_low(P_LED1);
	output_low(P_LED2);
	
	HayMandos = LeerMandos();	//lee los mandos grabados
	EncenderRF();

	do{
		if(BTN == PRESIONADO){
			flagSync = TRUE;
			output_high(P_LED2);
			
			while(BTN == PRESIONADO){delay_ms(20);}
		}
		
		ComprobarRF();
	}while(true);
}

/*
 * Comprueba si llego alguna trama RF
 */
void ComprobarRF(void){
	if(DataReady() == TRUE){   //data frame is on Buffer
		RecibAnterior.Completo = Recibido.Completo;
		Recibido.Completo = rfBuffer.Completo;
		
#ifdef DEBUG
		printf("0x %02X %02X %02X\r", Recibido.Bytes.Hi, Recibido.Bytes.Mi, Recibido.Bytes.Lo);
#endif
		
		if(flagSync == TRUE){
			if(RecibAnterior.Completo == Recibido.Completo){
				output_low(P_LED2);			//apago led
				flagSync = FALSE;			//apago sync
				GrabarMando();				//graba el mando recibido
				HayMandos = LeerMandos();	//vuelve a leer los mandos almacenados
				delay_ms(1000);
			}
		}
		else{
			if(AnalizarRF() == TRUE){
				output_high(P_LED1);
				
				output_high(P_LED2);
				delay_us(10);
				output_low(P_LED2);
			}
		}
	}
	
	#warning "que pasa si esta mantenido y se presiona el boton de sync?"
	if((RFMantenido == FALSE) && (flagSync == FALSE)){
		output_low(P_LED1);
	}
}