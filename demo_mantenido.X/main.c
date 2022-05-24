#include <16F1825.h>
#device adc=8
#use delay(clock=32000000)

#FUSES INTRC_IO, NOWDT, PUT, NOMCLR, NOPROTECT, NOCPD, BROWNOUT, NOCLKOUT, NOIESO, NOFCMEN
#FUSES NOWRT, PLL_SW, NOSTVREN, BORV19, NODEBUG, NOLVP

#use fast_io(a)               //se accede al puerto a como memoria
#byte PORTA	= getenv("SFR:PORTA")
#byte PORTC	= getenv("SFR:PORTC")

/* PORT A */
//pins
#bit SERIAL_TX	= PORTA.0			//serial out
#bit A1			= PORTA.1			//
#bit RF			= PORTA.2			//RF in
#bit BTN1		= PORTA.3			//btn 1
#bit LED1		= PORTA.4			//led 1
#bit LED2		= PORTA.5			//led 2

//defines
#define P_SERIAL_TX	PIN_A0			//O
#define P_A1		PIN_A1			//I
#define P_RF		PIN_A2			//I
#define P_BTN1		PIN_A3			//I
#define P_LED1		PIN_A4			//O
#define P_LED2		PIN_A5			//O

//Bits			    543210
#define TRIS_A	0b00001110			//define cuales son entradas y cuales salidas
#define WPU_A	0b00001000			//define los weak pull up

/* PORT C */
//pins
#bit BTN2		= PORTC.0			//
#bit C1			= PORTC.1			//
#bit C2			= PORTC.2			//
#bit C3			= PORTC.3			//
#bit C4			= PORTC.4			//
#bit C5			= PORTC.5			//

//defines
#define P_BTN2		PIN_C0			//I
#define P_C1		PIN_C1			//I
#define P_C2		PIN_C2			//I
#define P_C3		PIN_C3			//I
#define P_C4		PIN_C4			//I
#define P_C5		PIN_C5			//I
#define P_C6		PIN_C6			//I
#define P_C7		PIN_C7			//I

//Bits			    543210
#define TRIS_C	0b11110011			//define cuales son entradas y cuales salidas
#define WPU_C	0b00000001			//define los weak pull up

/* VARIABLES */
short HayMandos;					//indica si hay algun mando grabado en memoria

/* CONSTANTES GENERALES */
//#define DEBUG
#define	PRESIONADO	0

/* CONSTANTES PARA RF */
#define RF_RX_TIMER0				//usamos el timer0 para RF
#define NUM_MANDOS_RF			10	//permite memorizar 3 mandos
#define NUM_CANALES_RF			1	//un canal por cada mando
#define RF_MANTENIDO_TIME_OUT	300	//cuanto tiempo tiene que pasar para que se interprete como que no esta mantenido (en mS)
#define POS_MEM_MANDOS_START_RF	0	//a partir de aqui se graban los mandos

#ifdef DEBUG
#use rs232(baud=250000, xmit=P_SERIAL_TX, ERRORS)
#endif

#include "..\rf_rx.c"
#include "..\rf_rx_aux.c"

void ComprobarRF(void);
/* -------------------------------- */

void main(void){
	setup_oscillator(OSC_8MHZ|OSC_PLL_ON);	//configura oscilador interno
	setup_wdt(WDT_OFF);						//configuracion wdt
	//setup_timer_0(T0_INTERNAL|T0_DIV_1);	//lo configura EncenderRF()
	//setup_timer_1(T1_INTERNAL|T1_DIV_BY_1);	//
	//setup_timer_2(T2_DIV_BY_1,255,1);		//
	setup_dac(DAC_OFF);						//configura DAC
	setup_adc(ADC_CLOCK_INTERNAL);			//configura ADC
	setup_adc_ports(NO_ANALOGS);			//configura ADC
	//setup_ccp1(CCP_COMPARE_INT);			//lo configura EncenderRF()
	setup_spi(SPI_DISABLED);				//configura SPI
	//setup_uart(FALSE);					//configura UART
	setup_vref(VREF_OFF);					//no se usa voltaje de referencia
	setup_comparator(NC_NC_NC_NC);			//comparador apagado
	
	//enable_interrupts(GLOBAL);			//lo habilita EncenderRF()
	
	set_tris_a(TRIS_A);						//configura pines
	port_a_pullups(WPU_A);					//configura weak pull ups
	
	set_tris_c(TRIS_C);						//configura pines
	port_c_pullups(WPU_C);					//configura weak pull ups
	
	output_low(P_LED1);
	output_low(P_LED2);
	
	HayMandos = LeerMandos();	//lee los mandos grabados
	EncenderRF();
	
	//printf("\r\n%Lu", RF_MANTENIDO_TIME_OUT_US);

	do{
		if(BTN1 == PRESIONADO){
			flagSync = TRUE;
			output_high(P_LED2);
			
			while(BTN1 == PRESIONADO){delay_ms(20);}
		}
		
#ifdef DEBUG
		if(BTN2 == PRESIONADO){
			printf("\r\n - MEMORIA - \r\n");
			
			for(int x = 0; x<NUM_MANDOS_RF; x++){
				int PosBase = POS_MEM_MANDOS_START_RF + (x*POS_TO_JUMP);
				
				printf("%02u: 0x %02X %02X %02X\r\n", x, MemRF[x][0].Bytes.Hi, MemRF[x][0].Bytes.Mi, MemRF[x][0].Bytes.Lo);
			}
			printf("\r\n");
			
			while(BTN2 == PRESIONADO){delay_ms(20);}
		}
#endif
		
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
		printf("0x %02X %02X %02X", Recibido.Bytes.Hi, Recibido.Bytes.Mi, Recibido.Bytes.Lo);
		
		if(RecibAnterior.Completo != Recibido.Completo)
			printf(" < ");	//mando diferente
		else
			printf("   ");
#endif
		
		if(flagSync == TRUE){
#ifdef DEBUG
			printf(" S ");	//sync
#endif
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
#ifdef DEBUG
				printf(" M ");	//match con un mando en la memoria
#endif
				//output_high(P_LED1);	//no hace falta. El LED se enciende/apaga con la variable RFmantenido.
			}
			else{
#ifdef DEBUG
			printf("   ");
#endif
			}
		}
#ifdef DEBUG
		printf("\r\n");
#endif
	}
	
	//#warning "que pasa si esta mantenido y se presiona el boton de sync?" -> el proceso de sync tiene un delay de 1000mS que "bloquea" el programa y la señal se deja de analizar por lo que RFmantenido vuelve a FALSE
	/*if((RFMantenido == FALSE) && (flagSync == FALSE)){
		output_low(P_LED1);
	}*/
	//el led se enciende/apaga segun la variable RFmantenido
	if(flagSync == FALSE){
		LED1 = RFmantenido;
	}
}