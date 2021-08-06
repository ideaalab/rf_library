#include <12F1840.h>
#device adc=8
#use delay(clock=32000000)

//#define DEBUG

#FUSES INTRC_IO, NOWDT, PUT, NOMCLR, NOPROTECT, NOCPD, BROWNOUT, NOCLKOUT, NOIESO, NOFCMEN
#FUSES NOWRT, PLL_SW, NOSTVREN, BORV19, NODEBUG, NOLVP

#use fast_io(a)               //se accede al puerto a como memoria
#byte PORTA	= getenv("SFR:PORTA")

#define		PRESIONADO	0

//pins
#bit LED	= PORTA.0	//led
#bit A1		= PORTA.1	//
#bit RF		= PORTA.2	//RF in
#bit BTN	= PORTA.3	//btn
#bit A4		= PORTA.4	//
#bit OUT	= PORTA.5	//

//defines
#define P_LED		PIN_A0		//O
#define P_A1		PIN_A1		//I
#define P_RF		PIN_A2		//I
#define P_BTN		PIN_A3		//I
#define P_A4		PIN_A4		//I
#define P_OUT		PIN_A5		//O

//Bits			    543210
#define TRIS_A	0b00011110	//define cuales son entradas y cuales salidas
#define WPU_A	0b00001000	//define los weak pull up

/* VARIABLES */
short HayMandos;			//indica si hay algun mando grabado en memoria

/* CONSTANTES PARA RF */
#define RF_RX_TIMER0					//usamos el timer0 para RF
#define NUM_MANDOS_RF			3		//permite memorizar 3 mandos
#define NUM_CANALES_RF			1
#define RF_MANTENIDO					//vamos a usar el btn mantenido por RF de RF_RX_AUX
#define TIME_OUT_RF_MANTENIDO	150		//cuanto tiempo tiene que pasar para que se interprete como que no esta mantenido
#define POS_MEM_MANDOS_START_RF	0		//a partir de aqui se graban los mandos

#ifdef DEBUG
#use rs232(baud=250000, xmit=P_OUT, ERRORS)
#endif

#include "..\rf_rx.c"
#include "..\rf_rx_aux.c"

void ComprobarRF(void);
/* -------------------------------- */

void main(void){
	setup_oscillator(OSC_8MHZ|OSC_PLL_ON);	//configura oscilador interno
	setup_wdt(WDT_OFF);						//configuracion wdt
	//setup_timer_0(T0_INTERNAL|T0_DIV_1);		//lo configura rf_in_init()
	//setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);	//
	//setup_timer_2(T2_DIV_BY_64,249,5);		//lo configura RFmantenido_init()
	setup_dac(DAC_OFF);						//configura DAC
	setup_adc(ADC_CLOCK_INTERNAL);			//configura ADC
	setup_adc_ports(NO_ANALOGS);			//configura ADC
	//setup_ccp1(CCP_COMPARE_INT);				//lo configura rf_in_init()
	setup_spi(SPI_DISABLED);				//configura SPI
	//setup_uart(FALSE);					//configura UART
	setup_vref(VREF_OFF);					//no se usa voltaje de referencia
	setup_comparator(NC_NC);				//comparador apagado
	
	//enable_interrupts(GLOBAL);				//lo habilitan otras funciones
	
	set_tris_a(TRIS_A);						//configura pines
	port_a_pullups(WPU_A);					//configura weak pull ups
	
	output_low(P_LED);
	output_low(P_OUT);
	
	HayMandos = LeerMandos();	//lee los mandos grabados
	EncenderRF();
	RF_mantenido_init();
	
	do{
		if(BTN == PRESIONADO){
			flagSync = TRUE;
			output_high(P_LED);
			
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
		/*putc(Recibido.Bytes.Hi);
		putc(Recibido.Bytes.Mi);
		putc(Recibido.Bytes.Lo);
		printf("\r\n");*/
#endif
		
		if(flagSync == TRUE){
			if(RecibAnterior.Completo == Recibido.Completo){
				output_low(P_LED);			//apago led
				flagSync = FALSE;			//apago sync
				GrabarMando();				//graba el mando recibido
				HayMandos = LeerMandos();	//vuelve a leer los mandos almacenados
				delay_ms(1000);
			}
		}
		else{
			if(AnalizarRF() == TRUE){
				output_high(P_LED);
#ifndef DEBUG
				output_high(P_OUT);
				delay_us(10);
				output_low(P_OUT);
#endif
			}
			//rfBuffer.Completo = 0;
		}
	}
	
	#warning "que pasa si esta mantenido y se presiona el boton de sync?"
	if((RFMantenido == FALSE) && (flagSync == FALSE)){
		output_low(P_LED);
	}
}