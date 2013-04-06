#include <vxworks.h>
#include <drv/mem/m82xxDpramLib.h>
#include <drv/sio/m8260Cp.h>
#include <drv/intrCtl/m8260IntrCtl.h>
#include <cacheLib.h>
#include <drv/parallel/m8260IOPort.h>
#include "spi.h"

#define IVEC   INUM_SPI

void spiInit();
void spiEnable();
void spiDisable();
short spiSend(char adr, short val);
void spiIsr();
void spiSetIsr(void (*func)(short));

u_long immrVal;
u_long spiRxIsr=0,spiTxErrIsr=0,spiRxBusyIsr=0,spiTxIsr=0;
spiBD *rxBd, *txBd;
void * pPram, *pBd;
void (*_isr_func)(short val);
int spiInited = 0;

void spiFree()
{
	m82xxDpramFree(pPram);
	m82xxDpramFree(pBd);
}

void spiInit()
{
	
	short *spiBase;
	SPI_PARAM *spiParam;
	char *txBuffer, *rxBuffer;
	u_long cpcrVal;
	int ix;
	
	pPram  = m82xxDpramAlignedMalloc(0x40,64);
	if(pPram == NULL)
	{
		printf("there is not enough space for SPI PARAMETER RAM\n");
		return;
	}
	pBd    = m82xxDpramAlignedMalloc(0x40,64);
	if(pBd == NULL)
	{
		printf("there is not enough space for SPI BD table\n");
		return;
	}
	
	immrVal = vxImmrGet();	
	
	intDisable(IVEC);
	
	/*disable SPI*/
	spiDisable();
		
	
	/*config PORT D to enable SPIMISO, SPIMOSI, SPICLK*/
	* M8260_IOP_PDPAR(immrVal) |= PD16|PD17|PD18 ;
	* M8260_IOP_PDSO(immrVal)  |= PD16|PD17|PD18 ;
	* M8260_IOP_PDDIR(immrVal) &= ~(PD16|PD17|PD18) ;
	
	/*config SPI select output for AUDIO*/ /*use PD19*/
	* M8260_IOP_PDPAR(immrVal) &= ~PD19 ; /*general I/0*/
	* M8260_IOP_PDDIR(immrVal) |= PD19 ;/*output*/
	* M8260_IOP_PDDAT(immrVal) |= PD19 ;/*output*/	
	
	/*config SPI select output for CHECK*/ /*use PC30*/
	* M8260_IOP_PCPAR(immrVal) &= ~PC30 ; /*general I/0*/
	* M8260_IOP_PCDIR(immrVal) |= PC30 ;/*output*/
	* M8260_IOP_PCDAT(immrVal) |= PC30 ;/*output*/
	
	
	/*in 0x89FC,set parameterRam base address*/
	spiBase = (short*)SPI_BASE(immrVal);
	*spiBase = (short)((long)pPram & 0xffff); /*only offset in dpram space???*/
		
	/*write RBASE and TBASE in SPI parameter ram*/
	spiParam = (SPI_PARAM *)(pPram);
	spiParam->rbase = (short)((long)pBd & 0xffff); /*only one rx bd*/
	spiParam->tbase = (short)(((long)pBd &0xffff) + 8); /*only one tx bd*/
	
	/*write RFCR & TFCR*/
	spiParam->rfcr = 0x10;/*use 60x bus*/
	spiParam->tfcr = 0x10;/*use 60x bus*/
	
	/*write MRBLR with maximum number of bytes per RX buffer*/
	spiParam->mlblr = MAX_RX_LEN ; /*max 16 bytes per RX buffer*/
	
	/*initinize RX BD and TX BD*/
	rxBuffer = (char*)cacheDmaMalloc(MAX_RX_LEN);
	txBuffer = (char*)cacheDmaMalloc(MAX_TX_LEN);
	rxBd = (spiBD *)(pBd);
	txBd = (spiBD *)((char*)pBd+8);
	
	rxBd->bd_cstatus = RX_E |RX_W | RX_I; 
	rxBd->bd_length  =0;
	rxBd->bd_buffer = (u_long)rxBuffer;
	
	txBd->bd_cstatus = TX_W |TX_L; /*need interrupt?*/
	txBd->bd_length  =2;
	txBd->bd_buffer = (u_long)txBuffer;
	
	/*excute INIT_RX_TX_PARAMETER to CPCR*/
	ix = 0;
	do
	{
		cpcrVal = *M8260_CPCR (immrVal);
		if (ix++ == M8260_CPCR_LATENCY)
       break;
  }   while (cpcrVal & M8260_CPCR_FLG); 
	if (ix >= M8260_CPCR_LATENCY)
	{	
    printf("SEND COMMAND TO CPCR FAILED\n");
    return;
  }
  cpcrVal = 0x25410000;
  *M8260_CPCR (immrVal) = cpcrVal;
  CACHE_PIPE_FLUSH ();
	ix = 0;
	do
	{
		cpcrVal = *M8260_CPCR (immrVal);
		if (ix++ == M8260_CPCR_LATENCY)
       break;
  }   while (cpcrVal & M8260_CPCR_FLG) ;



#if 1	
	*SPIM(immrVal) = MSK_TXE|MSK_BSY|MSK_TXB|MSK_RXB;
#else
	*SPIM(immrVal) = 0;
#endif
	
	/*set SPI MODE*/
	*SPMODE(immrVal) = /*|LOOP_BACK|*/MASTER|(15<<LEN_OFFSET)|(0<<PRESCALE_OFFSET)|DIV_16; /*CI,CP=0 ,REV=0,LEN=15,M/S=1(master), DIV_16=0,PRESCALE=2*/
	
	/*enable SPI*/
	spiEnable();
	
	_isr_func = NULL;
	
#if 0	
	*M8260_SIMR_L(immrVal) &= ~(1<<14);
	/*clear all interrupt, enable interrupt*/
	*SPIE(immrVal) = 0xff;
	(void)intConnect((VOIDFUNCPTR *)IVEC, (VOIDFUNCPTR)(void *)spiIsr, 0);
	*M8260_SIMR_L(immrVal) |= 1<<14;
#endif

	spiInited = 1;
}

void spiEnable()
{
	*SPMODE(immrVal) |= ENABLE_SPI;
}

void spiDisable()
{
	*SPMODE(immrVal) &= ~ENABLE_SPI;
}


void setSS(char adr, int val)
{
  if(adr == AUDIO_ADR) /*audio*/
  {
	  if(val)
		  * M8260_IOP_PDDAT(immrVal) |= PD19;
	  else
		  * M8260_IOP_PDDAT(immrVal) &= ~PD19;
  }
  else if(adr == CHECK_ADR) /*self test*/
  {
    if(val)
		  * M8260_IOP_PCDAT(immrVal) |= PC30;
	  else
		  * M8260_IOP_PCDAT(immrVal) &= ~PC30;               
  }
}


short spiSend(char adr, short val)
{
	int state;
	int i;
        short recv_value;

	/*wait last send finished*/
	state = txBd->bd_cstatus;
	while(state&0x8000) state = txBd->bd_cstatus;
	*(short*)txBd->bd_buffer = val;	
	txBd->bd_cstatus |= TX_R;

	setSS(adr, 0);	

	*SPCOM(immrVal) =STR; 
	/*wait send complete*/
	for(i=0;i<3000;i++);
        
        /*return receive value*/
	recv_value = *(short*)rxBd->bd_buffer;
  rxBd->bd_cstatus |= RX_E;

	setSS(adr, 1);
  return recv_value;
}

void spiIsr()
{
	u_char spie;
	spie = *SPIE(immrVal);
	*SPIE(immrVal) = spie;
	
	if(spie &MSK_RXB) /*receive a word*/
	{
		spiRxIsr++;
		/*logMsg("receive val: %x\n",*(short*)rxBd->bd_buffer,2,3,4,5,6);*/
		rxBd->bd_cstatus |= RX_E;
		if(_isr_func)
			_isr_func(*(short*)rxBd->bd_buffer);
	}
	
	if(spie &MSK_TXE) /*tx error*/
	{
		spiTxErrIsr++;
	}
	
	if(spie	&MSK_BSY) /*rx buffer full*/
	{
		spiRxBusyIsr++;
	}
	
	if(spie &MSK_TXB) /*tx one word,need wait two characters*/
	{
		spiTxIsr++;
	}		
	
}

void spiSetIsr(void (*func)(short))
{
	_isr_func = func;
}

#if 0
void testSend(int cnt,short val)
{
	int i;
  for(i=0;i<cnt;i++)
	{
		taskDelay(100);
		spiSend(val);
	}
	
}

void looptest(int cnt)
{
int i;
if(!spiInited)
 	spiInit();

spiRxIsr=0;spiTxErrIsr=0;spiRxBusyIsr=0;spiTxIsr=0;

for(i=0;i<cnt;i++)
spiSend(0xaa55);
}
void testSysTime(int val)
{
	setSS(0);	
	taskDelay(val);
	setSS(1);
}
#endif