//------------------------------------------------------------------------------
//                    Emisor RF
//
// Descripcion: Puede emular los encoders PT2240B, PT2260, PT2262 y PT2264.
//				En estos encoders/decoders los CEROS, UNOS y SYNC se codifican
//				de la sigiente forma:
//				CERO: Pulso High corto - Pulso Low largo (Duty 25%)
//				UNO: Pulso High largo - Pulso Low corto	(Duty 75%)
//				SYNC: Pulso High corto - Pulso Low MUY largo (Duty 3.125%)
//				
//				PT2262, PT2264
//				Pulso Corto = 4 * ALFA
//				Pulso Largo = 12 * ALFA
//				Pulso Sync	= 124 * ALFA
//				
//				PT2240B
//				Pulso Corto = 32 * ALFA
//				Pulso Largo = 96 * ALFA
//				Pulso Sync	= 992 * ALFA
//				
//				PT2260
//				Pulso Corto = 128 * ALFA
//				Pulso Largo = 348 * ALFA
//				Pulso Sync	= 3968 * ALFA
//				
//				Siempre despues de un pulso corto viene uno largo (o muy largo si
//				es un SYNC), y despues de un High viene un Low
//				
//		-->		Declarar RF_TX como #bit para que la libreria funcione
//
//		-->		Se puede acceder a los datos de estas formas:
//				txBuffer.
//						Ch4.Address/A/B/C/D 	(16/2/2/2/2) TRIN
//						Ch6.Address/A/B/C/D/E/F (12/2/2/2/2/2/2) TRIN
//						Esp.Address/Type/Val	(8/8/8) BIN
//						Sen.Address/Val			(16/8) BIN
//						bits[x]					(24)
//
//				Los datos trinarios utilizan 2 bits para representar 0, 1 y floating.
//				0 = 00
//				1 = 11
//				f = 01
//
//------------------------------------------------------------------------------

//Constantes
/*#define PT2262	1	//factor de multiplicacion de ALFA
#define PT2264	1
#define PT2240B	8
#define PT2260	32

#define FREQ	200000
#define ALFA	(1000000/FREQ)	//en uS

#define CERO_H	(ALFA*4)
#define CERO_L	(ALFA*12)

#define UNO_H	(ALFA*12)
#define UNO_L	(ALFA*4)

#define SYNC_H	(ALFA*4)
#define SYNC_L	(ALFA*124)
*/
/*
#define CORTO	57
#define LARGO	182
#define SYNC	992
*/
/*
#define CORTO	31
#define LARGO	94
#define SYNC	992
*/

/* en uso
#define CERO_CORTO	255
#define CERO_LARGO	766

#define UNO_LARGO	767
#define UNO_CORTO	255

#define SYNC_CORTO	255
#define SYNC_LARGO	7936
*/

#define CERO_CORTO	382
#define CERO_LARGO	1149

#define UNO_LARGO	1149
#define UNO_CORTO	382

#define SYNC_CORTO	382
#define SYNC_LARGO	11904

//------------------------

void Send_Cero(){	//envia un CERO
	RF_TX = 1;
	delay_us(CERO_CORTO);
	//delay_cycles(7);
	delay_cycles(10);
	RF_TX = 0;
	delay_us(CERO_LARGO);
	//delay_cycles(6);
	delay_cycles(14);
}

void Send_Uno(){	//Envia un UNO
	RF_TX = 1;
	delay_us(UNO_LARGO);
	//delay_cycles(15);
	RF_TX = 0;
	delay_us(UNO_CORTO);
	//delay_cycles(1);
}

void Send_Sync(){	//Envia un SYNC
	RF_TX = 1;
	delay_us(SYNC_CORTO);
	//delay_cycles(7);
	RF_TX = 0;
	delay_us(SYNC_LARGO);
	//delay_cycles(3);
}

void Send_TX(rfRemote Data){
int x, y;

	for(x=0; x<4;x++){		//repite la emision 4 veces
		for(y=0;y<24;y++){
			if(Data.Bits[y] == 0)
				Send_Cero();
			else
				Send_Uno();	
		}
		Send_Sync();
		
		/*
		//23---
		if(bit_test(Data.Ints.Hi, 7) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//22---
		if(bit_test(Data.Ints.Hi, 6) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//21---
		if(bit_test(Data.Ints.Hi, 5) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//20---
		if(bit_test(Data.Ints.Hi, 4) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//19---
		if(bit_test(Data.Ints.Hi, 3) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//18---
		if(bit_test(Data.Ints.Hi, 2) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//17---
		if(bit_test(Data.Ints.Hi, 1) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//16---
		if(bit_test(Data.Ints.Hi, 0) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//15---
		if(bit_test(Data.Ints.Mid, 7) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//14---
		if(bit_test(Data.Ints.Mid, 6) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//13---
		if(bit_test(Data.Ints.Mid, 5) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//12---
		if(bit_test(Data.Ints.Mid, 4) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//11---
		if(bit_test(Data.Ints.Mid, 3) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//10--
		if(bit_test(Data.Ints.Mid, 2) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//9
		if(bit_test(Data.Ints.Mid, 1) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//8---
		if(bit_test(Data.Ints.Mid, 0) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//7---
		if(bit_test(Data.Ints.Lo, 7) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//6---
		if(bit_test(Data.Ints.Lo, 6) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//5---
		if(bit_test(Data.Ints.Lo, 5) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//4---
		if(bit_test(Data.Ints.Lo, 4) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//3---
		if(bit_test(Data.Ints.Lo, 3) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//2---
		if(bit_test(Data.Ints.Lo, 2) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//1---
		if(bit_test(Data.Ints.Lo, 1) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//0---
		if(bit_test(Data.Ints.Lo, 0) == 0){
			Send_Cero();
		}
		else{
			Send_Uno();
		}
		//---
		
		Send_Sync();
		 */
	}
}

/* -------------------------------------------------------------------------- */
void Zero_16mS(void){	//envia un CERO
	RF_TX = 1;
	delay_us(125-5);
	RF_TX = 0;
	delay_us(375-13);
}

void One_16mS(void){	//Envia un UNO
	RF_TX = 1;
	delay_us(375-11);
	RF_TX = 0;
	delay_us(125-7);
}

void Sync_16mS(void){	//Envia un SYNC
	RF_TX = 1;
	delay_us(125-5);
	RF_TX = 0;
	delay_us(3875-93);
}

void TX_16mS(rfRemote Data, int num){
int x, y;
	
	for(x=0; x<num;x++){		//repite la emision
		for(y=0;y<24;y++){
			if(Data.Bits[y] == 0)
				Zero_16mS();
			else
				One_16mS();	
		}
		Sync_16mS();
	}
}
/* -------------------------------------------------------------------------- */
void Zero_32mS(void){	//envia un CERO
	RF_TX = 1;
	delay_us(250-5);
	RF_TX = 0;
	delay_us(750-24);
}

void One_32mS(void){	//Envia un UNO
	RF_TX = 1;
	delay_us(750-17);
	RF_TX = 0;
	delay_us(250-11);
}

void Sync_32mS(void){	//Envia un SYNC
	RF_TX = 1;
	delay_us(250-5);
	RF_TX = 0;
	delay_us(7750-183);
}

void TX_32mS(rfRemote Data, int num){
int x, y;
	
	for(x=0; x<num;x++){		//repite la emision
		for(y=0;y<24;y++){
			if(Data.Bits[y] == 0)
				Zero_32mS();
			else
				One_32mS();	
		}
		Sync_32mS();
	}
}
/* -------------------------------------------------------------------------- */
void Zero_48mS(void){	//envia un CERO
	RF_TX = 1;
	delay_us(375-11);
	RF_TX = 0;
	delay_us(1125-31);
}

void One_48mS(void){	//Envia un UNO
	RF_TX = 1;
	delay_us(1125-23);
	RF_TX = 0;
	delay_us(375-12);
}

void Sync_48mS(void){	//Envia un SYNC
	RF_TX = 1;
	delay_us(375-5);
	RF_TX = 0;
	delay_us(11625-263);
}

void TX_48mS(rfRemote Data, int num){
int x, y;
	
	for(x=0; x<num;x++){		//repite la emision
		for(y=0;y<24;y++){
			if(Data.Bits[y] == 0)
				Zero_48mS();
			else
				One_48mS();	
		}
		Sync_48mS();
	}
}
/* -------------------------------------------------------------------------- */
void Zero_64mS(void){	//envia un CERO
	RF_TX = 1;
	delay_us(500-11);
	RF_TX = 0;
	delay_us(1500-42);
}

void One_64mS(void){	//Envia un UNO
	RF_TX = 1;
	delay_us(1500-30);
	RF_TX = 0;
	delay_us(500-17);
}

void Sync_64mS(void){	//Envia un SYNC
	RF_TX = 1;
	delay_us(500-11);
	RF_TX = 0;
	delay_us(15500-353);
}

void TX_64mS(rfRemote Data, int num){
int x, y;
	
	for(x=0; x<num;x++){		//repite la emision
		for(y=0;y<24;y++){
			if(Data.Bits[y] == 0)
				Zero_64mS();
			else
				One_64mS();	
		}
		Sync_64mS();
	}
}