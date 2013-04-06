#ifndef __SPI_H_
#define __SPI_H_

/*****************************************/
/*             register                  */
/*****************************************/
#define SPMODE(base)  (CAST(VUINT16 *)((base) + 0x11AA0))/* SPI Mode Reg */      
/*MASK define for SPMODE*/
#define LOOP_BACK   0x4000
#define CLK_INVERT  0x2000
#define CLK_PHASE   0x1000
#define DIV_16			0x0800  /*BRGCLK = SYSCLK/16*/
#define NO_REV_DATA    0x0400  /*USE REV_DATA=1*/
#define MASTER      0x0200
#define ENABLE_SPI  0x0100   /*when change SPI config , need disable SPI first*/
#define LEN_MASK   	0x00f0
#define PRESCALE    0x000f

#define LEN_OFFSET  0x4
#define PRESCALE_OFFSET  0


#define SPIE(base)  (CAST(VUINT8 *)((base) + 0x11AA6))/* SPI Event Status */ /*set 1 to relative bit to clear event*/     
#define SPIM(base)  (CAST(VUINT8 *)((base) + 0x11AAA))/* SPI Event Mask */              
#define MSK_MME   0x20
#define MSK_TXE   0x10
#define MSK_BSY   0x04
#define MSK_TXB   0x02
#define MSK_RXB   0x01

#define SPCOM(base)  (CAST(VUINT8 *)((base) + 0x11AAD))/* SPI Command Reg */                
#define STR    0x80    /*start tx & rx*/

/*****************************************/
/*           parameter ram               */
/*****************************************/
#define SPI_BASE(base)	(base + 0x89FC)  /*need 64-bytes aligned in bank 1-8,11,12*/

#pragma  pack(1)
typedef  struct
		{
		u_short rbase;
		u_short tbase;
		u_char  rfcr;
		u_char  tfcr;
		u_short mlblr;
		u_long rstate;
		u_long rptr;
		u_short rbptr;
		u_short rcount;
		u_long rtemp;
		u_long tstate;
		u_long tptr;
		u_short tbptr;
		u_short tcount;
		u_long ttemp;
		u_long fill[12];
		u_long sdmatemp;
		} SPI_PARAM;
#pragma pack(4)

/*****************************************/
/*         spi command                   */
/*****************************************/
#include <drv/sio/m8260cp.h>
#define INIT_TX_PARAMETER				M8260_CPCR_TX_INIT
#define CLOSE_RXBD							M8260_CPCR_RX_CLOSE
#define INIT_RX_PARAMETER  			M8260_CPCR_RX_INIT
#define INIT_RX_TX_PARAMTETER  	M8260_CPCR_RT_INIT

/*****************************************/
/*         BD table                      */
/*****************************************/
typedef struct BufferDescriptor 
{
  u_short bd_cstatus;     /* BD control and status   */
  u_short bd_length;      /* BD Data length transfer   */
  u_long bd_buffer;      /* BD Data Buffer Pointer   */
} spiBD;

#define RX_E  	0x8000  /*empty*/
#define RX_W		0x2000  /*wrap*/
#define RX_I		0x1000  /*interrupt enable*/
#define RX_L		0x0800  /*last , only used in slave mode*/
#define RX_CM		0x0200  /*continue mode*/
#define RX_OV   0x0002  /*overrun state*/
#define RX_ME   0x0001  /*multimaster error*/

#define TX_R		0x8000  /*ready*/
#define TX_W    0x2000  /*wrap*/
#define TX_I    0x1000  /*interrupt enable*/
#define TX_L    0x0800  /*last*/
#define TX_CM   0x0200  /*continue mode*/
#define TX_UN   0x0002  /*underrun state*/
#define TX_ME   0x0001  /*multimaster error*/

#define MAX_RX_LEN    0x2
#define MAX_TX_LEN    0x2

#define EIEIO_SYNC WRS_ASM (" eieio; sync")

#define PD19    (0x00001000)
#define PD18    (0x00002000)
#define PD17    (0x00004000)
#define PD16    (0x00008000)

#define PC30    (0x00000002)


#define  AUDIO_ADR   0
#define CHECK_ADR   1


#endif