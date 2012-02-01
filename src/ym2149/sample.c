/*******************************************

  TEST CODE

*******************************************/
#include <stdio.h>
#include <time.h>
#include "emu2149.h"
#include "assert.h"

/* Standard clock = MSX clock */
#define MSX_CLK 3579545

/* YM2149 internal clock */
#define YM2149_CLK (MSX_CLK/2)

#define SAMPLERATE 44100
#define DATALENGTH (SAMPLERATE*2)

static void WORD(char *buf, uint32 data){

  buf[0] = data & 0xff ;
  buf[1] = (data & 0xff00) >> 8 ;

}


static void DWORD(char *buf, uint32 data){

  buf[0] = data & 0xff ;
  buf[1] = (data & 0xff00) >> 8 ;
  buf[2] = (data & 0xff0000) >> 16 ;
  buf[3] = (data & 0xff000000) >> 24 ;

}

static void chunkID(char *buf, char id[4]){

  buf[0] = id[0] ;
  buf[1] = id[1] ;
  buf[2] = id[2] ;
  buf[3] = id[3] ;

}

static int16 wave[DATALENGTH] ;

int main(int argc, char *argv[]){

  static char FILENAME[16] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

  char header[46] ;
  int i , t = 0 , e ;

  FILE *fp ;
  PSG *psg ;

  chunkID(header,"RIFF") ;
  DWORD(header+4,DATALENGTH*2+36) ;
  chunkID(header+8,"WAVE") ;
  chunkID(header+12,"fmt ") ;
  DWORD(header+16,16) ;
  WORD(header+20,1) ;				/* WAVE_FORMAT_PCM */
  WORD(header+22,1) ;				/* channel 1=mono,2=stereo */
  DWORD(header+24,SAMPLERATE) ;		/* samplesPerSec */
  DWORD(header+28,2*SAMPLERATE) ;	/* bytesPerSec */
  WORD(header+32,2) ;				/* blockSize */
  WORD(header+34,16) ;				/* bitsPerSample */
  chunkID(header+36,"data") ;
  DWORD(header+40,2*DATALENGTH) ;

  for(e=8;e<16;e++){

  t = 0 ;
  sprintf(FILENAME,"ENV%02x.WAV",e) ;
  printf("Target: %s\n",FILENAME) ;

  PSG_init(YM2149_CLK,SAMPLERATE) ;
  psg = PSG_new() ;
  PSG_reset(psg) ;

  PSG_writeReg(psg,0,0x58) ;
  PSG_writeReg(psg,1,0x00) ;
  PSG_writeReg(psg,11,0) ;
  PSG_writeReg(psg,12,8) ;

  PSG_writeReg(psg,13,e) ;
  PSG_writeReg(psg,8,16) ;

  for(i=0;i<DATALENGTH;i++){ wave[t++]=PSG_calc(psg); }

  PSG_delete(psg) ;
  PSG_close() ;

  fp = fopen(FILENAME,"wb") ;
  
  if(fp == NULL){
	  printf("Can't open %s.\n",FILENAME) ;
	  return 1 ;
  }

  fwrite(header,46,sizeof(char),fp) ;
  fwrite(wave,DATALENGTH,sizeof(int16),fp) ;
  
  fclose(fp) ;

  printf("Wrote : %s\n",FILENAME) ;

  }

  return 0 ;

}
