#include <12F1840.h>
#device adc=8
#use delay(clock=32000000)

/* CONFIGURATION WORD 1 */

/* Oscillator Selection (FOSC) */
//#FUSES LP			//Low-power crystal connected between OSC1 and OSC2 pins
//#FUSES XT			//Crystal/resonator connected between OSC1 and OSC2 pins
//#FUSES HS			//High-speed crystal/resonator connected between OSC1 and OSC2 pins
//#FUSES RC			//External RC circuit connected to CLKIN pin
#FUSES INTRC_IO		//I/O function on CLKIN pin
//#FUSES ECL		//External Clock, Low-Power mode (0-0.5 MHz): device clock supplied to CLKIN pin
//#FUSES ECM		//External Clock, Medium-Power mode (0.5-4 MHz): device clock supplied to CLKIN pin
//#FUSES ECH		//External Clock, High-Power mode (4-20 MHz): device clock supplied to CLKIN pin

/* Watchdog Timer Enable (WDTE) */
#FUSES NOWDT		//WDT disabled
//#FUSES WDT_SW		//WDT controlled by software
//#FUSES WDT_NOSL	//WDT enabled while running and disabled in Sleep
//#FUSES WDT		//WDT enabled

/* Power-up Timer Enable (PWRTE) */
#FUSES PUT			//PWRT enabled
//#FUSES NOPUT		//PWRT disabled

/* MCLR/VPPPin Function (MCLRE) */
#FUSES NOMCLR		//MCLR/VPP pin function is digital input.
//#FUSES MCLR			//MCLR/VPP pin function is MCLR; Weak pull-up enabled.

/* Code Protection (CP)*/
//#FUSES PROTECT		//Program memory code protection is enabled
#FUSES NOPROTECT	//Program memory code protection is disabled
		
/* Data Code Protection (CPD) */
//#FUSES CPD			//Data memory code protection is enabled
#FUSES NOCPD		//Data memory code protection is disabled

/* Brown-out Reset Enable (BOREN) */
//#FUSES NOBROWNOUT	//BOR disabled
//#FUSES BROWNOUT_SW	//BOR controlled by software
//#FUSES BROWNOUT_NOSL	//BOR enabled during operation and disabled in Sleep
#FUSES BROWNOUT		//BOR enabled

/* Clock Out Enable (CLKOUTEN) */
//#FUSES CLKOUT		//CLKOUT function is enabled on the CLKOUT pin
#FUSES NOCLKOUT		//CLKOUT function is disabled. I/O function on the CLKOUT pin.

/* Internal External Switchover (IESO) */
#FUSES NOIESO		//Internal/External Switchover mode is disabled
//#FUSES IESO			//Internal/External Switchover mode is enabled

/* Fail-Safe Clock Monitor Enable (FCMEN) */
#FUSES NOFCMEN		//Fail-Safe Clock Monitor is disabled
//#FUSES FCMEN		//Fail-Safe Clock Monitor is enabled

/* CONFIGURATION WORD 2 */

/* Flash Memory Self-Write Protection (WRT) */
//#FUSES WRT			//000h to FFFh write-protected, no addresses may be modified
//#FUSES WRT_800		//000h to 7FFh write-protected, 800h to FFFh may be modified
//#FUSES WRT_200		//000h to 1FFh write-protected, 200h to FFFh may be modified
#FUSES NOWRT		//Write protection off

/* PLL Enable (PLLEN) */
#FUSES PLL_SW		//4xPLL disabled (can be enabled by software)
//#FUSES PLL			//4xPLL enabled

/* Stack Overflow/Underflow Reset Enable (STVREN) */
#FUSES NOSTVREN		//Stack Overflow or Underflow will not cause a Reset
//#FUSES STVREN		//Stack Overflow or Underflow will cause a Reset

/* Brown-out Reset Voltage Selection (BORV) */
#FUSES BORV25		//Brown-out Reset voltage (Vbor), high trip point selected
//#FUSES BORV19		//Brown-out Reset voltage (Vbor), low trip point selected

/* In-Circuit Debugger Mode (DEBUG) */
//#FUSES DEBUG		//In-Circuit Debugger enabled, ICSPCLK and ICSPDAT are dedicated to the debugger
#FUSES NODEBUG		//In-Circuit Debugger disabled, ICSPCLK and ICSPDAT are general purpose I/O pins

/* Low-Voltage Programming Enable (LVP) */
#FUSES NOLVP		//High-voltage on MCLRmust be used for programming
//#FUSES LVP			//Low-voltage programming enabled 

/* -------------------------------- */

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
#define TIME_OUT_RF_MANTENIDO	100		//cuanto tiempo tiene que pasar para que se interprete como que no esta mantenido
#define POS_MEM_MANDOS_START_RF	0	//a partir de aqui se graban los mandos

#include "..\rf_rx.c"
#include "..\rf_rx_aux.c"

#ifdef DEBUG
#use rs232(baud=115200, xmit=P_OUT, DISABLE_INTS, ERRORS)
#endif

void ComprobarRF(void);
/* -------------------------------- */

void main(void){
	setup_oscillator(OSC_8MHZ|OSC_PLL_ON);	//configura oscilador interno
	setup_wdt(WDT_OFF);						//configuracion wdt
	//setup_timer_0(T0_INTERNAL|T0_DIV_1);		//lo configura rf_in_init()
	//setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);	//lo configura servo_init()
	//setup_timer_2(T2_DIV_BY_64,249,5);		//lo configura RFmantenido_init()
	setup_dac(DAC_OFF);						//configura DAC
	setup_adc(ADC_CLOCK_INTERNAL);			//configura ADC
	setup_adc_ports(sAN3|VSS_VDD);			//configura ADC
	//setup_ccp1(CCP_COMPARE_INT);				//lo configura rf_in_init()
	setup_spi(SPI_DISABLED);				//configura SPI
	//setup_uart(FALSE);					//configura UART
	setup_vref(VREF_OFF);					//no se usa voltaje de referencia

	//enable_interrupts(GLOBAL);				//lo habilitan otras funciones

	setup_comparator(NC_NC);				//comparador apagado
	
	set_tris_a(TRIS_A);						//configura pines
	port_a_pullups(WPU_A);					//configura weak pull ups
	
	LED = FALSE;
	OUT = FALSE;
	
	HayMandos = LeerMandos();	//lee los mandos grabados
	EncenderRF();
	RF_mantenido_init();
	
	do{
		if(BTN == PRESIONADO){
			flagSync = TRUE;
			LED = TRUE;
			
			while(BTN == PRESIONADO){delay_ms(20);}
		}
		
		ComprobarRF();
		
#warning "que pasa si esta mantenido y se presiona el boton de sync?"
		if((RFMantenido == FALSE) && (flagSync == FALSE)){
			LED = FALSE;
		}
	}while(true);
}

/*
 * Comprueba si llego alguna trama RF
 */
void ComprobarRF(void){
	if(DataReady() == TRUE){   //data frame is on Buffer
#ifdef DEBUG
		printf("BL: 0x%02X BM: 0x%02X BH: 0x%02X\r\n", rfBuffer.Bytes.Lo, rfBuffer.Bytes.Mi, rfBuffer.Bytes.Hi);
#endif
		
		if(flagSync == TRUE){
			RecibAnterior.Completo = Recibido.Completo;
			Recibido.Completo = rfBuffer.Completo;
		
			if(RecibAnterior.Completo == Recibido.Completo){
				LED = FALSE;		//apago led
				flagSync = FALSE;	//apago sync
				GrabarMando();		//graba el mando recibido
				HayMandos = LeerMandos();	//vuelve a leer los mandos almacenados
				delay_ms(1000);
			}
		}
		else{
			if(AnalizarRF() == TRUE){
				output_high(P_LED);
				
				output_high(P_OUT);
				delay_us(10);
				output_low(P_OUT);
			}
			//rfBuffer.Completo = 0;
		}
	}
}