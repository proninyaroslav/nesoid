/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/********************************************************/
/*******		sound.c				*/
/*******						*/
/*******  Sound emulation code and waveform synthesis 	*/
/*******  routines.  A few ideas were inspired		*/
/*******  by code from Marat Fayzullin's EMUlib		*/
/*******						*/
/********************************************************/

#include <stdlib.h>
#include <stdio.h>

#include <string.h>

#include "types.h"
#include "x6502.h"

#include "fce.h"
#include "svga.h"
#include "sound.h"

uint32 Wave[2048+512];
int16 WaveFinalMono[2048+512];

EXPSOUND GameExpSound={0,0,0,0,0,0};

uint8 trimode=0;
uint8 tricoop=0;
uint8 PSG[0x18];

uint8 decvolume[3];
uint8 realvolume[3];

static int32 count[5];
static int32 sqacc[2]={0,0};
uint8 sqnon=0;

uint32 soundtsoffs=0;

#undef printf
uint16 nreg;

static int32 lengthcount[4];

extern int soundvol;

static const uint8 Slengthtable[0x20]=
{
 0x5,0x7f,0xA,0x1,0x14,0x2,0x28,0x3,0x50,0x4,0x1E,0x5,0x7,0x6,0x0E,0x7,
 0x6,0x08,0xC,0x9,0x18,0xa,0x30,0xb,0x60,0xc,0x24,0xd,0x8,0xe,0x10,0xf
};

static uint32 lengthtable[0x20];

static const uint32 SNoiseFreqTable[0x10]=
{
 2,4,8,0x10,0x20,0x30,0x40,0x50,0x65,0x7f,0xbe,0xfe,0x17d,0x1fc,0x3f9,0x7f2
};
static uint32 NoiseFreqTable[0x10];

int32 nesincsize;
uint32 soundtsinc;
uint32 soundtsi;


static const uint8 NTSCPCMTable[0x10]=
{
 0xd6,0xbe,0xaa,0xa0,0x8f,0x7f,0x71,0x6b,
 0x5f,0x50,0x47,0x40,0x35,0x2a,0x24,0x1b
};

static const uint8 PALPCMTable[0x10]=	// These values are just guessed.
{
 0xc6,0xb0,0x9d,0x94,0x84,0x75,0x68,0x63,
 0x58,0x4a,0x41,0x3b,0x31,0x27,0x21,0x19
};

uint32 PSG_base;

// $4010        -        Frequency
// $4011        -        Actual data outputted
// $4012        -        Address register: $c000 + V*64
// $4013        -        Size register:  Size in bytes = (V+1)*64


static int32 PCMacc=0;
static int PCMfreq;
int32 PCMIRQCount;
uint8 PCMBitIndex=0;
uint32 PCMAddressIndex=0;
int32 PCMSizeIndex=0;
uint8 PCMBuffer=0;
int vdis=0;

static void Dummyfunc(int end) {};

static void (*DoNoise)(int end)=Dummyfunc;
static void (*DoTriangle)(int end)=Dummyfunc;
static void (*DoPCM)(int end)=Dummyfunc;
static void (*DoSQ1)(int end)=Dummyfunc;
static void (*DoSQ2)(int end)=Dummyfunc;

static void CalcDPCMIRQ(void)
{
 uint32 freq;
 uint32 honk;
 uint32 cycles;

 if(PAL)
  freq=(PALPCMTable[PSG[0x10]&0xF]<<4);
 else
  freq=(NTSCPCMTable[PSG[0x10]&0xF]<<4);

 cycles=(((PSG[0x13]<<4)+1));
 cycles*=freq/14;
 honk=((PSG[0x13]<<4)+1)*freq;
 honk-=cycles;
 //if(PAL) honk/=107;
 //else honk/=(double)113.66666666;
 PCMIRQCount=honk*48;
 //PCMIRQCount=honk*3; //180;
 //if(PAL) PCMIRQCount*=.93;
 vdis=0;
}

static void PrepDPCM()
{
 PCMAddressIndex=0x4000+(PSG[0x12]<<6);
 PCMSizeIndex=(PSG[0x13]<<4)+1;
 PCMBitIndex=0;
 //PCMBuffer=ARead[0x8000+PCMAddressIndex](0x8000+PCMAddressIndex);
 if(PAL)
  PCMfreq=PALPCMTable[PSG[0x10]&0xF];
 else
  PCMfreq=NTSCPCMTable[PSG[0x10]&0xF];
 PCMacc=PCMfreq<<18;
}

uint8 sweepon[2]={0,0};
int32 curfreq[2]={0,0};


uint8 SIRQStat=0;

uint8 SweepCount[2];
uint8 DecCountTo1[3];

uint8 fcnt=0;
int32 fhcnt=0;
int32 fhinc;

static uint8 laster;

/* Instantaneous?  Maybe the new freq value is being calculated all of the time... */
static int FASTAPASS(2) CheckFreq(uint32 cf, uint8 sr)
{
 uint32 mod;
 if(!(sr&0x8))
 {
  mod=cf>>(sr&7);
  if((mod+cf)&0x800)
   return(0);
 }
 return(1);
}

static DECLFW(Write0x11)
{
 DoPCM(0);
 PSG[0x11]=V&0x7F;
}

static uint8 DutyCount[2]={0,0};

static DECLFW(Write_PSG)
{
 //if((A>=0x4004 && A<=0x4007) || A==0x4015)
  //printf("$%04x:$%02x, %d\n",A,V,SOUNDTS);
 A&=0x1f;
 switch(A)
 {
  case 0x0:
           DoSQ1(0);
           if(V&0x10)
            realvolume[0]=V&0xF;
           break;
  case 0x1:
           sweepon[0]=V&0x80;
           break;
  case 0x2:
           DoSQ1(0);
           curfreq[0]&=0xFF00;
           curfreq[0]|=V;
           break;
  case 0x3:
           if(PSG[0x15]&1)
           {
            DoSQ1(0);
            lengthcount[0]=lengthtable[(V>>3)&0x1f];
            sqnon|=1;
	   }
           sweepon[0]=PSG[1]&0x80;
           curfreq[0]=PSG[0x2]|((V&7)<<8);
	   decvolume[0]=0xF;
	   DecCountTo1[0]=(PSG[0]&0xF)+1;
           SweepCount[0]=((PSG[0x1]>>4)&7)+1;
	   DutyCount[0]=0;
           sqacc[0]=((int32)curfreq[0]+1)<<18;
           break;

  case 0x4:
	   DoSQ2(0);
           if(V&0x10)
            realvolume[1]=V&0xF;
	   break;
  case 0x5:
          sweepon[1]=V&0x80;
          break;
  case 0x6:
          DoSQ2(0);
          curfreq[1]&=0xFF00;
          curfreq[1]|=V;
          break;
  case 0x7:
          if(PSG[0x15]&2)
          {
	   DoSQ2(0);
	   lengthcount[1]=lengthtable[(V>>3)&0x1f];
           sqnon|=2;
	  }
          sweepon[1]=PSG[0x5]&0x80;
          curfreq[1]=PSG[0x6]|((V&7)<<8);
          decvolume[1]=0xF;
 	  DecCountTo1[1]=(PSG[0x4]&0xF)+1;
          SweepCount[1]=((PSG[0x5]>>4)&7)+1;
          DutyCount[1]=0;
          sqacc[1]=((int32)curfreq[1]+1)<<18;
          break;
  case 0x8:
          DoTriangle(0);
	  if(laster&0x80)
	  {
            tricoop=V&0x7F;
            trimode=V&0x80;
          }
          if(!(V&0x7F))
           tricoop=0;
          laster=V&0x80;
          break;
  case 0xa:DoTriangle(0);
	   break;
  case 0xb:
	  if(PSG[0x15]&0x4)
	  {
	   DoTriangle(0);
           sqnon|=4;
           lengthcount[2]=lengthtable[(V>>3)&0x1f];
	  }
          laster=0x80;
          tricoop=PSG[0x8]&0x7f;
          trimode=PSG[0x8]&0x80;
          break;
  case 0xC:DoNoise(0);
           if(V&0x10)
            realvolume[2]=V&0xF;
           break;
  case 0xE:DoNoise(0);break;
  case 0xF:
           if(PSG[0x15]&8)
           {
	    DoNoise(0);
            sqnon|=8;
	    lengthcount[3]=lengthtable[(V>>3)&0x1f];
	   }
           decvolume[2]=0xF;
	   DecCountTo1[2]=(PSG[0xC]&0xF)+1;
           break;
 case 0x10:DoPCM(0);
	   if(!(V&0x80))
	    X6502_IRQEnd(FCEU_IQDPCM);
	   break;
 case 0x15:
	   {
	    int t=V^PSG[0x15];

            if(t&1)
             DoSQ1(0);
            if(t&2)
             DoSQ2(0);
            if(t&4)
             DoTriangle(0);
            if(t&8)
             DoNoise(0);
            if(t&0x10)
             DoPCM(0);
            sqnon&=V;
            if(V&0x10)
            {
             if(!(PSG[0x15]&0x10))
             {
              PrepDPCM();
              CalcDPCMIRQ();
             }
             else if(vdis)
              CalcDPCMIRQ();
            }
            else
             PCMIRQCount=0;
            X6502_IRQEnd(FCEU_IQDPCM);
	   }
           break;
 }
 PSG[A]=V;
}

DECLFR(Read_PSG)
{
   uint8 ret;
   if(PSG[0x15]&0x10)
    DoPCM(0);
   ret=(PSG[0x15]&(sqnon|0x10))|SIRQStat;
   SIRQStat&=~0x40;
   X6502_IRQEnd(/*FCEU_IQDPCM|*/FCEU_IQFCOUNT);
   return ret;
}

DECLFR(Read_PSGDummy)
{
   uint8 ret;

   ret=(PSG[0x15]&sqnon)|SIRQStat;
   SIRQStat&=~0x40;
   X6502_IRQEnd(/*FCEU_IQDPCM|*/FCEU_IQFCOUNT);
   return ret;
}

static void FASTAPASS(1) FrameSoundStuff(int V)
{
 int P;
 uint32 end = (SOUNDTS<<16)/soundtsinc;

 DoSQ1(end);
 DoSQ2(end);
 DoNoise(end);

 switch((V&1))
 {
  case 1:       /* Envelope decay, linear counter, length counter, freq sweep */
        if(PSG[0x15]&4 && sqnon&4)
         if(!(PSG[8]&0x80))
         {
          if(lengthcount[2]>0)
          {
            lengthcount[2]--;
            if(lengthcount[2]<=0)
             {
              DoTriangle(0);
              sqnon&=~4;
             }
           }
         }

        for(P=0;P<2;P++)
        {
         if(PSG[0x15]&(P+1) && sqnon&(P+1))
 	 {
          if(!(PSG[P<<2]&0x20))
          {
           if(lengthcount[P]>0)
           {
            lengthcount[P]--;
            if(lengthcount[P]<=0)
             {
              sqnon&=~(P+1);
             }
           }
          }
	 }
		/* Frequency Sweep Code Here */
		/* xxxx 0000 */
		/* xxxx = hz.  120/(x+1)*/
	  if(sweepon[P])
          {
           int32 mod=0;

	   if(SweepCount[P]>0) SweepCount[P]--;
	   if(SweepCount[P]<=0)
	   {
	    SweepCount[P]=((PSG[(P<<2)+0x1]>>4)&7)+1; //+1;
            {
             if(PSG[(P<<2)+0x1]&0x8)
             {
              mod-=(P^1)+((curfreq[P])>>(PSG[(P<<2)+0x1]&7));

              if(curfreq[P] && (PSG[(P<<2)+0x1]&7)/* && sweepon[P]&0x80*/)
              {
               curfreq[P]+=mod;
              }
             }
             else
             {
              mod=curfreq[P]>>(PSG[(P<<2)+0x1]&7);
              if((mod+curfreq[P])&0x800)
              {
               sweepon[P]=0;
               curfreq[P]=0;
              }
              else
              {
               if(curfreq[P] && (PSG[(P<<2)+0x1]&7)/* && sweepon[P]&0x80*/)
               {
                curfreq[P]+=mod;
               }
              }
             }
            }
	   }
          }
         }

       if(PSG[0x15]&0x8 && sqnon&8)
        {
         if(!(PSG[0xC]&0x20))
         {
          if(lengthcount[3]>0)
          {
           lengthcount[3]--;
           if(lengthcount[3]<=0)
           {
            sqnon&=~8;
           }
          }
         }
        }

  case 0:       /* Envelope decay + linear counter */
         if(!trimode)
         {
           laster=0;
           if(tricoop)
           {
            if(tricoop==1) DoTriangle(0);
            tricoop--;
           }
         }

        for(P=0;P<2;P++)
        {
	  if(DecCountTo1[P]>0) DecCountTo1[P]--;
          if(DecCountTo1[P]<=0)
          {
	   DecCountTo1[P]=(PSG[P<<2]&0xF)+1;
           if(decvolume[P] || PSG[P<<2]&0x20)
           {
            decvolume[P]--;
	    /* Step from 0 to full volume seems to take twice as long
	       as the other steps.  I don't know if this is the correct
	       way to double its length, though(or if it even matters).
	    */
            if((PSG[P<<2]&0x20) && (decvolume[P]==0))
             DecCountTo1[P]<<=1;
	    decvolume[P]&=15;
           }
          }
          if(!(PSG[P<<2]&0x10))
           realvolume[P]=decvolume[P];
        }

         if(DecCountTo1[2]>0) DecCountTo1[2]--;
         if(DecCountTo1[2]<=0)
         {
          DecCountTo1[2]=(PSG[0xC]&0xF)+1;
          if(decvolume[2] || PSG[0xC]&0x20)
          {
            decvolume[2]--;
            /* Step from 0 to full volume seems to take twice as long
               as the other steps.  I don't know if this is the correct
               way to double its length, though(or if it even matters).
            */
            if((PSG[0xC]&0x20) && (decvolume[2]==0))
             DecCountTo1[2]<<=1;
            decvolume[2]&=15;
          }
         }
         if(!(PSG[0xC]&0x10))
          realvolume[2]=decvolume[2];

        break;
 }

}

void FrameSoundUpdate(void)
{
 // Linear counter:  Bit 0-6 of $4008
 // Length counter:  Bit 4-7 of $4003, $4007, $400b, $400f

 if(fcnt==3)
 {
	if(PSG[0x17]&0x80)
	 fhcnt+=fhinc;
        if(!(PSG[0x17]&0xC0))
        {
         SIRQStat|=0x40;
	 X6502_IRQBegin(FCEU_IQFCOUNT);
        }
 }
 //if(SIRQStat&0x40) X6502_IRQBegin(FCEU_IQFCOUNT);
 FrameSoundStuff(fcnt);
 fcnt=(fcnt+1)&3;
}

static uint32 ChannelBC[5];

static uint32 RectAmp[2][8];

static void FASTAPASS(1) CalcRectAmp(int P)
{
  static int tal[4]={1,2,4,6};
  int V;
  int x;
  uint32 *b=RectAmp[P];
  int m;

  //if(PSG[P<<2]&0x10)
   V=realvolume[P]<<4;
  //V=(PSG[P<<2]&15)<<4;
  //else
  // V=decvolume[P]<<4;
  m=tal[(PSG[P<<2]&0xC0)>>6];
  for(x=0;x<m;x++,b++)
   *b=0;
  for(;x<8;x++,b++)
   *b=V;
}

static void RDoPCM(int32 end)
{
   int32 V;
   int32 start;
   int32 freq;
   uint32 out=PSG[0x11]<<3;

   start=ChannelBC[4];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[4]=end;

   if(PSG[0x15]&0x10)
   {
      freq=PCMfreq;
      freq<<=18;

      for(V=start;V<end;V++)
      {
       PCMacc-=nesincsize;
       if(PCMacc<=0)
       {
	if(!PCMBitIndex)
	{
         PCMSizeIndex--;
         if(!PCMSizeIndex)
         {
          if(PSG[0x10]&0x40)
           PrepDPCM();
          else
          {
           PSG[0x15]&=~0x10;
           for(;V<end;V++)
            Wave[V>>4]+=PSG[0x11]<<3;
           goto endopcmo;
          }
         }
         else
         {
          PCMBuffer=ARead[0x8000+PCMAddressIndex](0x8000+PCMAddressIndex);
          PCMAddressIndex=(PCMAddressIndex+1)&0x7fff;
	 }
	}

	{
         int t=(((PCMBuffer>>PCMBitIndex)&1)<<2)-2;
         uint8 bah=PSG[0x11];

	 PCMacc+=freq;
         PSG[0x11]+=t;
         if(PSG[0x11]&0x80)
          PSG[0x11]=bah;
	 else
	  out=PSG[0x11]<<3;
	}
	PCMBitIndex=(PCMBitIndex+1)&7;
       }
       Wave[V>>4]+=out; //(PSG[0x11]-64)<<3;
      }
   }
   else
   {
     if((end-start)>64)
     {
      for(V=start;V<=(start|15);V++)
       Wave[V>>4]+=out;
      out<<=4;
      for(V=(start>>4)+1;V<(end>>4);V++)
       Wave[V]+=out;
      out>>=4;
      for(V=end&(~15);V<end;V++)
       Wave[V>>4]+=out;
     }
     else
      for(V=start;V<end;V++)
       Wave[V>>4]+=out;
   }
    endopcmo:;
}

static void RDoSQ1(int32 end)
{
   int32 V;
   int32 start;
   int32 freq;

   start=ChannelBC[0];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[0]=end;

   if(!(PSG[0x15]&1 && sqnon&1))
    return;

   if(curfreq[0]<8 || curfreq[0]>0x7ff)
    return;
   if(!CheckFreq(curfreq[0],PSG[0x1]))
    return;

   CalcRectAmp(0);

   {
    uint32 out=RectAmp[0][DutyCount[0]];
    freq=curfreq[0]+1;
    {
      freq<<=18;
      for(V=start;V<end;V++)
      {
       Wave[V>>4]+=out;
       sqacc[0]-=nesincsize;
       if(sqacc[0]<=0)
       {
        rea:
        sqacc[0]+=freq;
        DutyCount[0]++;
        if(sqacc[0]<=0) goto rea;

        DutyCount[0]&=7;
        out=RectAmp[0][DutyCount[0]];
       }
      }
    }
   }
}

static void RDoSQ2(int32 end)
{
   int32 V;
   int32 start;
   int32 freq;

   start=ChannelBC[1];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[1]=end;

   if(!(PSG[0x15]&2 && sqnon&2))
    return;

   if(curfreq[1]<8 || curfreq[1]>0x7ff)
    return;
   if(!CheckFreq(curfreq[1],PSG[0x5]))
    return;

   CalcRectAmp(1);

   {
    uint32 out=RectAmp[1][DutyCount[1]];
    freq=curfreq[1]+1;

    {
      freq<<=18;
      for(V=start;V<end;V++)
      {
       Wave[V>>4]+=out;
       sqacc[1]-=nesincsize;
       if(sqacc[1]<=0)
       {
        rea:
        sqacc[1]+=freq;
        DutyCount[1]++;
	if(sqacc[1]<=0) goto rea;

        DutyCount[1]&=7;
	out=RectAmp[1][DutyCount[1]];
       }
      }
    }
   }
}


static void RDoTriangle(int32 end)
{
   static uint32 tcout=0;
   int32 V;
   int32 start; //,freq;
   int32 freq=(((PSG[0xa]|((PSG[0xb]&7)<<8))+1));

   start=ChannelBC[2];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[2]=end;

    if(! (PSG[0x15]&0x4 && sqnon&4 && tricoop) )
    {	// Counter is halted, but we still need to output.
     for(V=start;V<end;V++)
       Wave[V>>4]+=tcout;
    }
    else if(freq<=4) // 55.9Khz - Might be barely audible on a real NES, but
 	       // it's too costly to generate audio at this high of a frequency
 	       // (55.9Khz * 32 for the stepping).
 	       // The same could probably be said for ~27.8Khz, so we'll
 	       // take care of that too.  We'll just output the average
 	       // value(15/2 - scaled properly for our output format, of course).
	       // We'll also take care of ~18Khz and ~14Khz too, since they should be barely audible.
	       // (Some proof or anything to confirm/disprove this would be nice.).
    {
     for(V=start;V<end;V++)
      Wave[V>>4]+=((0xF<<4)+(0xF<<2))>>1;
    }
    else
    {
     static int32 triacc=0;
     static uint8 tc=0;

      freq<<=17;
      for(V=start;V<end;V++)
      {
       triacc-=nesincsize;
       if(triacc<=0)
       {
        rea:
        triacc+=freq; //t;
        tc=(tc+1)&0x1F;
        if(triacc<=0) goto rea;

        tcout=(tc&0xF);
        if(tc&0x10) tcout^=0xF;
        tcout=(tcout<<4)+(tcout<<2);
       }
       Wave[V>>4]+=tcout;
      }
    }
}

static void RDoNoise(int32 end)
{
   int32 inc,V;
   int32 start;

   start=ChannelBC[3];
   if(end==0) end=(SOUNDTS<<16)/soundtsinc;
   if(end<=start) return;
   ChannelBC[3]=end;

   if(PSG[0x15]&0x8 && sqnon&8)
   {
      uint32 outo;
      uint32 amptab[2];
      uint8 amplitude;

      amplitude=realvolume[2];
      //if(PSG[0xC]&0x10)
      // amplitude=(PSG[0xC]&0xF);
      //else
      // amplitude=decvolume[2]&0xF;

      inc=NoiseFreqTable[PSG[0xE]&0xF];
      amptab[0]=((amplitude<<2)+amplitude+amplitude)<<1;
      amptab[1]=0;
      outo=amptab[nreg&1];

      if(amplitude)
      {
       if(PSG[0xE]&0x80)	// "short" noise
        for(V=start;V<end;V++)
        {
         Wave[V>>4]+=outo;
         if(count[3]>=inc)
         {
          uint8 feedback;

          feedback=((nreg>>8)&1)^((nreg>>14)&1);
          nreg=(nreg<<1)+feedback;
          nreg&=0x7fff;
          outo=amptab[nreg&1];
          count[3]-=inc;
         }
         count[3]+=0x1000;
        }
       else
        for(V=start;V<end;V++)
        {
         Wave[V>>4]+=outo;
         if(count[3]>=inc)
         {
          uint8 feedback;

          feedback=((nreg>>13)&1)^((nreg>>14)&1);
          nreg=(nreg<<1)+feedback;
          nreg&=0x7fff;
          outo=amptab[nreg&1];
          count[3]-=inc;
         }
         count[3]+=0x1000;
        }
      }
   }
}

DECLFW(Write_IRQFM)
{
 PSG[0x17]=V;
 V=(V&0xC0)>>6;
 fcnt=0;
 if(V&0x2)
  FrameSoundUpdate();
 fcnt=1;
 fhcnt=fhinc;
 X6502_IRQEnd(FCEU_IQFCOUNT);
 SIRQStat&=~0x40;
 //IRQFrameMode=V; // IRQFrameMode is PSG[0x17] upper bits
}

void SetNESSoundMap(void)
{
  SetWriteHandler(0x4000,0x4013,Write_PSG);
  SetWriteHandler(0x4011,0x4011,Write0x11);
  SetWriteHandler(0x4015,0x4015,Write_PSG);
  SetWriteHandler(0x4017,0x4017,Write_IRQFM);
  SetReadHandler(0x4015,0x4015,Read_PSG);
}

int32 highp;                   // 0 through 65536, 0 = no high pass, 65536 = max high pass

int32 lowp;                    // 0 through 65536, 65536 = max low pass(total attenuation)
				// 65536 = no low pass
static int32 flt_acc=0, flt_acc2=0;

static void FilterSound(uint32 *in, int16 *outMono, int count)
{
// static int min=0, max=0;
 int sh=2;
 if (soundvol < 5) sh += 5 - soundvol;

 for(;count;count--,in++,outMono++)
 {
  int32 diff;

  diff = *in - flt_acc;

  flt_acc += (diff*highp)>>16;
  flt_acc2+= (int32) (((int64)((diff-flt_acc2)*lowp))>>16);
  *in=0;

  *outMono = flt_acc2*7 >> sh; // * 7 >> 2 = * 1.75
//  if (acc2 < min) { printf("min: %i %04x\n", acc2, acc2); min = acc2; }
//  if (acc2 > max) { printf("max: %i %04x\n", acc2, acc2); max = acc2; }
 }
}



static int32 inbuf=0;
int FlushEmulateSound(void)
{
  int x;
  uint32 end;

  if(!timestamp) return(0);

  if(!FSettings.SndRate || (soundvol == 0))
  {
   end=0;
   goto nosoundo;
  }

  end=(SOUNDTS<<16)/soundtsinc;
  DoSQ1(end);
  DoSQ2(end);
  DoTriangle(end);
  DoNoise(end);
  DoPCM(end);

  if(GameExpSound.Fill)
   GameExpSound.Fill(end&0xF);

  FilterSound(Wave,WaveFinalMono,end>>4);

  if(end&0xF)
   Wave[0]=Wave[(end>>4)];
  Wave[(end>>4)]=0;

  nosoundo:
  for(x=0;x<5;x++)
   ChannelBC[x]=end&0xF;
  soundtsoffs=(soundtsinc*(end&0xF))>>16;
  end>>=4;
  inbuf=end;
  return(end);
}

int GetSoundBuffer(int16 **W)
{
 *W=WaveFinalMono;
 return inbuf;
}

void PowerSound(void)
{
	int x;

        SetNESSoundMap();

        for(x=0;x<0x16;x++)
         if(x!=0x14)
          BWrite[0x4000+x](0x4000+x,0);
        PSG[0x17]=0; //x40;
        fhcnt=fhinc;
        fcnt=0;
        nreg=1;
        soundtsoffs=0;
}

void ResetSound(void)
{
        int x;
        for(x=0;x<0x16;x++)
         if(x!=1 && x!=5 && x!=0x14) BWrite[0x4000+x](0x4000+x,0);
        PSG[0x17]=0;
        fhcnt=fhinc;
        fcnt=0;
        nreg=1;
}

void SetSoundVariables(void)
{
  int x;

  fhinc=PAL?16626:14915;	// *2 CPU clock rate
  fhinc*=24;
  for(x=0;x<0x20;x++)
   lengthtable[x]=Slengthtable[x]<<1;

  if(FSettings.SndRate)
  {
   DoNoise=RDoNoise;
   DoTriangle=RDoTriangle;
   DoPCM=RDoPCM;
   DoSQ1=RDoSQ1;
   DoSQ2=RDoSQ2;
  }
  else
  {
   DoNoise=DoTriangle=DoPCM=DoSQ1=DoSQ2=Dummyfunc;
  }

  if(!FSettings.SndRate) return;
  if(GameExpSound.RChange)
   GameExpSound.RChange();

  // nesincsizeLL=(int64)((int64)562949953421312*(double)(PAL?PAL_CPU:NTSC_CPU)/(FSettings.SndRate OVERSAMPLE));
  nesincsize=(int32)(((int64)1<<17)*(double)(PAL?PAL_CPU:NTSC_CPU)/(FSettings.SndRate * 16)); // 308845 - 1832727
  PSG_base=(uint32)(PAL?(long double)PAL_CPU/16:(long double)NTSC_CPU/16);

  for(x=0;x<0x10;x++)
  {
   long double z;
   z=SNoiseFreqTable[x]<<1;
   z=(PAL?PAL_CPU:NTSC_CPU)/z;
   z=(long double)((uint32)((FSettings.SndRate OVERSAMPLE)<<12))/z;
   NoiseFreqTable[x]=z;
  }
  soundtsinc=(uint32)((uint64)(PAL?(long double)PAL_CPU*65536:(long double)NTSC_CPU*65536)/(FSettings.SndRate OVERSAMPLE));
  memset(Wave,0,sizeof(Wave));
  for(x=0;x<5;x++)
   ChannelBC[x]=0;
  highp=(250<<16)/FSettings.SndRate;  // Arbitrary
  lowp=(25000<<16)/FSettings.SndRate; // Arbitrary

  if(highp>(1<<16)) highp=1<<16;
  if(lowp>(1<<16)) lowp=1<<16;

  flt_acc=flt_acc2=0;
}

void FixOldSaveStateSFreq(void)
{
        int x;
        for(x=0;x<2;x++)
        {
         curfreq[x]=PSG[0x2+(x<<2)]|((PSG[0x3+(x<<2)]&7)<<8);
        }
}

void FCEUI_Sound(int Rate)
{
 FSettings.SndRate=Rate;
 SetSoundVariables();
}

void FCEUI_SetSoundVolume(uint32 volume)
{
 FSettings.SoundVolume=volume;
}
