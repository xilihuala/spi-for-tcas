#include <vxworks.h>
#include <drv/mem/m82xxDpramLib.h>
#include <drv/sio/m8260Cp.h>
#include <drv/intrCtl/m8260IntrCtl.h>
#include <cacheLib.h>
#include <drv/parallel/m8260IOPort.h>
#include "spi.h"


/*op code*/
#define POWERUP 0x04
#define SETPLAY 0x1c
#define PLAY    0x1e
#define SETREC  0x14
#define REC     0x16
#define SETMC   0x1d
#define MC			0x1f
#define STOP		0x6
#define STOPPWRDN 0x2
#define RINT    0x6
#define INVALID_CMD   0xf


#define CMD(op,adr)  (op<<11) |(adr&0x7ff)

extern int immrVal;
extern int spiInited;

void audioIsr();
void audioReadIntr(short val);
void audioSendCmd(short c,int addr);
void audioInit();
void audioPlay(int addr);
void audioStop();
void audioRecord(int addr);


short CONVERT(short val)
{
	short temp;
/*  temp = ((((val)&0xf0))>>4)| (((val)&0xf)<<4)|(((val)&0xf00)<<4)|(((val)&0xf000)>>4);*/
	temp = ((val&0xff)<<8) |((val&0xff00)>>8);
	return temp;
}

void audioInit()
{
	if(!spiInited)	
		spiInit();
		
	/*set spi receive isr*/
  spiSetIsr(audioReadIntr);

#if 1	
	/*connect audio isr*/
	*M8260_SIMR_H(immrVal) &= ~(1<<9);
	*M8260_SIPNR_H(immrVal)= 1<<9;
	(void)intConnect((VOIDFUNCPTR *)INUM_IRQ6, (VOIDFUNCPTR)(void *)audioIsr, 0);
	*M8260_SIEXR(immrVal) |= (1<<9);
	*M8260_SIMR_H(immrVal) |= 1<<9;
#endif
}
short swapBit(short val)
{
	int i;
	short r;
	r  = 0;
	for(i=0;i<16;i++)
		r |= (val & (1<<i)) <<(15-i);
}		
void audioReadIntr(short val)
{
	unsigned int ovf,eom,addr;
	unsigned short temp;
	ovf = val & 0x0100;
	eom = val & 0x0200;
	temp = ((val&0xff)<<8) |((val&0xff00)>>8);
	addr = (temp & 0x1ffc) >> 2;
	temp = swapBit(temp);
	logMsg("OVF=%x, EOM=%x, addr=%x , val = %x\n",ovf?1:0,eom?1:0,addr,val);
}

void audioSendCmd(short c,int addr)
{
	short cmd;
	logMsg("send cmd : %x\n",c);
	cmd = CMD(c,addr);
	cmd = CONVERT(cmd);
 	spiSend(AUDIO_ADR, cmd);
	
}
void audioIsr()
{	
	*M8260_SIPNR_H(immrVal)= 1<<9;
	audioSendCmd(INVALID_CMD,0);
	/*audioSendCmd(RINT,0);*/
}	

void audioPlay(int addr)
{
	int ilev;
	intDisable(INUM_IRQ6);
	audioSendCmd(POWERUP,addr);
	taskDelay(3);/*wait at least 25ms*/
	audioSendCmd(SETPLAY,addr);
	audioSendCmd(PLAY,addr);
	intEnable(INUM_IRQ6);
}

void audioRecord(int addr)
{
	audioSendCmd(POWERUP,addr);
	taskDelay(4);/*wait at least 25ms*/
	audioSendCmd(POWERUP,addr);
	taskDelay(8);/*wait at least 2*25ms*/
	audioSendCmd(SETREC,addr);
	audioSendCmd(REC,addr);
}

void audioEnd()
{
	audioSendCmd(STOP,0);
}

void audioStop()
{
	audioSendCmd(SETMC,0);
	audioSendCmd(MC,0);
}

void audioStopPdn()
{
	audioSendCmd(STOPPWRDN,0);
}

void audioTestCmd(short cmd)
{
	audioSendCmd(cmd,0);
}



	