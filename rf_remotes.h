/* -----------------------------------------------------------------------------
 *	>	rfAddr es un tipo de variable que permite almacenar direcciones RF.
 *		Ocupa 2 bytes cada direccion
 * 
 *	>	rfRemote es un tipo de variable que permite almacenar una trama
 *		entera de RF. Para facilitar la programacion rfRemote ocupa 4 bytes
 *		(32 bits) pero solo se utilizan 3 bytes (24 bits) y el resto se desperdicia
 * 
 *	>	En mandos y receptores de codigo fijo se usa la siguiente convencion
 *		trinaria para los pines:
 *			Pin a Gnd		= 00
 *			Pin a Vdd		= 11
 *			Pin flotando	= 01
 * 
 *	>	Se puede acceder a los datos de la siguiente manera:
 * 
 *			rfAddr Direccion;	//declaracion de nueva variable
 *			rfRemote Mando;		//declaracion de nueva variable
 * 
 *				rfAddr.
 *						A0			bits 0-1
 *						A1			bits 2-3
 *						A2			bits 4-5
 *						A3			bits 6-7
 *						A4			bits 8-9
 *						A5			bits 10-11
 *						A6			bits 12-13
 *						A7			bits 14-15
 * 
 *						AddrLo		bits 0-7
 *						AddrHi		bits 8-15
 * 
 *						Addr		bits 0-15
 * 
 *						Bits[16]	16 bits
 * 
 *				rfRemote.
 *						Ch4.	---------
 *							A0			bits 0-1
 *							A1			bits 2-3
 *							A2			bits 4-5
 *							A3			bits 6-7
 *							A4			bits 8-9
 *							A5			bits 10-11
 *							A6			bits 12-13
 *							A7			bits 14-15
 * 
 *							AddrLo		bits 0-7
 *							AddrHi		bits 8-15
 * 
 *							Addr		bits 0-15
 * 
 *							D3			bits 16-17
 *							D2			bits 18-19
 *							D1			bits 20-21
 *							D0			bits 22-23
 * 
 *							Dat			bits 16-23
 *							
 *							Nul			bits 24-31
 *
 *						Ch6.	---------
 *							A0			bits 0-1
 *							A1			bits 2-3
 *							A2			bits 4-5
 *							A3			bits 6-7
 *							A4			bits 8-9
 *							A5			bits 10-11
 * 
 *							AddrLo		bits 0-7
 *							AddrHi		bits 8-11
 * 
 *							D5			bits 12-13
 *							D4			bits 14-15
 *							D3			bits 16-17
 *							D2			bits 18-19
 *							D1			bits 20-21
 *							D0			bits 22-23
 * 
 *							DatLo		bits 12-15
 *							DatHi		bits 16-23
 *							
 *							Nul			bits 24-31
 * 
 *						Ch8.	---------
 *							A0			bits 0-1
 *							A1			bits 2-3
 *							A2			bits 4-5
 *							A3			bits 6-7
 *							A4			bits 8-9
 *							A5			bits 10-11
 *							A6			bits 12-13
 *							A7			bits 14-15
 * 
 *							AddrLo		bits 0-7
 *							AddrHi		bits 8-15
 * 
 *							Addr		bits 0-15
 * 
 *							D3			bits 16-17
 *							D2			bits 18-19
 *							D1			bits 20-21
 *							D0			bits 22-23
 * 
 *							Dat			bits 16-23
 *							
 *							Nul			bits 24-31
 * 
 *						Especial.	---------
 *							AddrLo		bits 0-7
 *							AddrHi		bits 8-11
 *							Typ			bits 12-15
 *							Dat			bits 16-19
 *							Com			bits 20-23
 *							Nul			bits 24-31
 *
 *						Sens.	---------
 *							AddrLo		bits 0-7
 *							AddrHi		bits 8-11
 *							Typ			bits 12-15
 *							Val			bits 16-23
 *							Nul			bits 24-31
 * 
 *						Bytes.	---------
 *							Lo			bits 0-7
 *							Mi			bits 8-15
 *							Hi			bits 16-23
 *							Nul			bits 24-31
 * 
 *						Bits[x]			bits 0-31 independientes
 * 
 *						Completo		bits 0-31
 ---------------------------------------------------------------------------- */

#ifndef RF_REMOTES_H
#define	RF_REMOTES_H

//mando de 4ch (normal): 16 bits de direccion, 8 bits de datos (dos para cada canal)
//valores posibles obtenidos de Ch4.Dat
#define BTN_4CH_D0	0b00000011	//Mando tapa = D / Mando potencia = D / Mando alargado = D / Mando redondo = B
#define BTN_4CH_D1	0b00001100	//Mando tapa = B / Mando potencia = C / Mando alargado = C / Mando redondo = C
#define BTN_4CH_D2	0b00110000	//Mando tapa = A / Mando potencia = B / Mando alargado = B / Mando redondo = D
#define BTN_4CH_D3	0b11000000	//Mando tapa = C / Mando potencia = A / Mando alargado = A / Mando redondo = A

//mando de 6ch: 12 bits de direccion, 12 bits de datos (dos para cada canal)
//valores posibles obtenidos de:
//long Dat6Ch = (long)Recibido.Ch6.DatHi<<4 | Recibido.Ch6.DatLo;
#define BTN_6CH_D0	0b000000000011	//boton 1
#define BTN_6CH_D1	0b000000001100	//boton 2
#define BTN_6CH_D2	0b000000110000	//boton 3
#define BTN_6CH_D3	0b000011000000	//boton 4
#define BTN_6CH_D4	0b001100000000	//boton 5
#define BTN_6CH_D5	0b110000000000	//boton 6

/* 
 * MANDO 8CH
 * 16 bits de direccion, 8 bits de datos (4 parejas de bits, codificado en binario)
 */
//pulsadores
#define BTN_8CH_1	0b00000011	//boton 1
#define BTN_8CH_2	0b00001100	//boton 2
#define BTN_8CH_3	0b00001111	//boton 3
#define BTN_8CH_4	0b00110000	//boton 4
#define BTN_8CH_5	0b00110011	//boton 5
#define BTN_8CH_6	0b00111100	//boton 6
#define BTN_8CH_7	0b00111111	//boton 7
#define BTN_8CH_8	0b11000000	//boton 8

/* 
 * MANDO 6CH y MINI MANDO iRF
 * 12 bits direccion, 4 bits de tipo de mando compartidos con 12 bits de datos (dos para cada pulsador)
 */
//posibles IDs del mando de 6ch:
#define ID_IRF_1	0x00		//vol+/vol-/next/prev
#define ID_IRF_2	0x0C		//play
#define ID_IRF_3	0x03		//pause
#define ID_IRF_4	0x0F		//play y pause juntos

//pulsadores
#define IRF_PAUSE	BTN_6CH_D0	//pause
#define IRF_PLAY	BTN_6CH_D1	//play
#define IRF_NEXT	BTN_6CH_D2	//next
#define IRF_PREV	BTN_6CH_D3	//prev
#define IRF_VDW		BTN_6CH_D4	//vol-
#define IRF_VUP		BTN_6CH_D5	//vol+

/* 
 * MANDO AVANZADO
 * 12 bits direccion, 4 bits de ID (tipo de mando), 4 bits de comando, 4 bits de valor
 */
//ID
#define ID_ADV_IRF	0x08

//comando
#define ADV_IRF_CHL	1	//change list
#define ADV_IRF_CHS	2	//change song
#define ADV_IRF_SHW	3	//show display
#define ADV_IRF_RET	4	//return to previous song

/* 
 * MANDO SENSOR
 * 12 bits direccion, 4 bits de ID (tipo de mando), 8 bits de valor
 */
//ID
#define ID_SENS		0x03

//constantes para RemoteMatch
#define RF_CHANNEL_1	0b00000001
#define RF_CHANNEL_2	0b00000010
#define RF_CHANNEL_3	0b00000100
#define RF_CHANNEL_4	0b00001000
#define RF_CHANNEL_5	0b00010000
#define RF_CHANNEL_6	0b00100000
#define RF_CHANNEL_7	0b01000000
#define RF_CHANNEL_8	0b10000000

/* --- TYPEDEF --- */
// rfAddr
typedef union{
	//A0 A1 A2 A3 A4 A5 A6 A7 (orden de los datos recibidos, cada uno son 2 bits, A7 es MSB)
	
	//parejas de bits
	struct{
		int A0:2;	//bits 0-1
		int A1:2;	//bits 2-3
		int A2:2;	//bits 4-5
		int A3:2;	//bits 6-7
		int A4:2;	//bits 8-9
		int A5:2;	//bits 10-11
		int A6:2;	//bits 12-13
		int A7:2;	//bits 14-15
	};
	
	//bytes
	struct{
		int AddrLo;		//bits 0-7
		int AddrHi;		//bits 8-15
	};
	
	//entero
	long Addr;	//bits 0-15
	
	//bits
	short Bits[16];	//16 bits
	
}rfAddr;

// rfRemote
typedef union{
	/* 4CH */
	//A0 A1 A2 A3 A4 A5 A6 A7 D3 D2 D1 D0 (orden de los datos recibidos, cada uno son 2 bits, D0 es MSB)
	struct{
		//direccion
		union{
			struct{
				int A0:2;	//bits 0-1
				int A1:2;	//bits 2-3
				int A2:2;	//bits 4-5
				int A3:2;	//bits 6-7
				int A4:2;	//bits 8-9
				int A5:2;	//bits 10-11
				int A6:2;	//bits 12-13
				int A7:2;	//bits 14-15
			};
			
			struct{
				int AddrLo;	//bits 0-7
				int AddrHi;	//bits 8-15
			};
			
			long Addr;		//bits 0-15
		};
		
		//datos
		union{
			struct{
				int D3:2;	//bits 16-17
				int D2:2;	//bits 18-19
				int D1:2;	//bits 20-21
				int D0:2;	//bits 22-23
			};
			
			int Dat;		//bits 16-23
		};

		int Nul;			//bits 24-32
	}Ch4;	//4 ch remote

	/* 6CH */
	//A0 A1 A2 A3 A4 A5 D5 D4 D3 D2 D1 D0 (orden de los datos recibidos, cada uno son 2 bits, D0 es MSB)
	struct{
		//direccion
		union{
			struct{
				int A0:2;	//bits 0-1
				int A1:2;	//bits 2-3
				int A2:2;	//bits 4-5
				int A3:2;	//bits 6-7
			};
			
			int AddrLo;		//bits 0-7
		};
		
		//direccion y datos
		union{
			struct{
				int A4:2;	//bits 8-9
				int A5:2;	//bits 10-11
				int D5:2;	//bits 12-13
				int D4:2;	//bits 14-15
			};
			
			struct{
				int AddrHi:4;	//bits 8-11
				int DatLo:4;	//bits 12-15
			};
		};
		
		//datos
		union{
			struct{
				int D3:2;	//bits 16-17
				int D2:2;	//bits 18-19
				int D1:2;	//bits 20-21
				int D0:2;	//bits 22-23
			};
			
			int DatHi;		//bits 16-23
		};
		
		int Nul;			//bits 24-31
	}Ch6;	//6 ch remote

	/* 8CH */
	//A0 A1 A2 A3 A4 A5 A6 A7 D3 D2 D1 D0 (orden de los datos recibidos, cada uno son 2 bits, D0 es MSB)
	struct{
		//direccion
		union{
			struct{
				int A0:2;	//bits 0-1
				int A1:2;	//bits 2-3
				int A2:2;	//bits 4-5
				int A3:2;	//bits 6-7
				int A4:2;	//bits 8-9
				int A5:2;	//bits 10-11
				int A6:2;	//bits 12-13
				int A7:2;	//bits 14-15
			};
			
			struct{
				int AddrLo;	//bits 0-7
				int AddrHi;	//bits 8-15
			};
			
			long Addr;	//bits 0-15
		};
		
		//datos
		union{
			struct{
				int D3:2;	//bits 16-17
				int D2:2;	//bits 18-19
				int D1:2;	//bits 20-21
				int D0:2;	//bits 22-23
			};
			
			int Dat;		//bits 16-23
		};

		int Nul;		//bits 24-32
	}Ch8;	//8 ch remote
	
	/* SPECIAL REMOTE */
	//A0 A1 A2 A3 A4 A5 T0 T1 D0 D1 C0 C1 (orden de los datos recibidos, cada uno son 2 bits, C1 es MSB) �esta bien?
	struct{
		int AddrLo;		//bits 0-7
		int AddrHi:4;	//bits 8-11
		int Typ:4;		//bits 12-15
		int Dat:4;		//bits 16-19
		int Com:4;		//bits 20-23
		int Nul;		//bits 24-31
	}Especial;	//special remote
	
	/* SENSOR REMOTE */
	//A0 A1 A2 A3 A4 A5 T0 T1 V0 V1 V2 V3 (orden de los datos recibidos, cada uno son 2 bits, V3 es MSB) ¿esta bien?
	struct{
		int AddrLo;		//bits 0-7
		int AddrHi:4;	//bits 8-11
		int Typ:4;		//bits 12-15
		int Val;		//bits 16-23
		int Nul;		//bits 24-31
	}Sens;	//sensor remote
	
	/* BYTES */
	struct{	
		int Lo;			//bits 0-7
		int Mi;			//bits 8-15	
		int Hi;			//bits 16-23
		int Nul;		//bits 24-31
	}Bytes;	//4 bytes

	/* BITS */
	short Bits[32];		//32 bits
	
	/* GENERIC */
	struct{
		long Addr;		//bits 0-15
		int Dat;		//bits 16-23
		int Nul;		//bits 24-31
	}Gen;
	
	/* COMPLETE */
	int32 Completo;
	
}rfRemote;

#endif	/* RF_REMOTES_H */