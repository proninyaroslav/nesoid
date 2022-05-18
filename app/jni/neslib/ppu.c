/**
 *  For whatever reason, breaking this out of fce.c made sprites not corrupt
 */


#include        <string.h>
#include	<stdio.h>
#include	<stdlib.h>

#include	"types.h"
#include	"x6502.h"
#include	"fce.h"
#include	"sound.h"
#include        "svga.h"
#include	"netplay.h"
#include	"general.h"
#include	"_endian.h"
#include	"version.h"
#include        "_memory.h"

#include	"cart.h"
#include	"nsf.h"
#include	"fds.h"
#include	"ines.h"
#include	"unif.h"
#include        "cheat.h"

#define MMC5SPRVRAMADR(V)      &MMC5SPRVPage[(V)>>10][(V)]
//#define MMC5BGVRAMADR(V)      &MMC5BGVPage[(V)>>10][(V)]
#define	VRAMADR(V)	&VPage[(V)>>10][(V)]

#define	V_FLIP	0x80
#define	H_FLIP	0x40
#define	SP_BACK	0x20

uint8 SPRAM[0x100] __attribute__ ((aligned (4)));
uint8 SPRBUF[0x100] __attribute__ ((aligned (4)));

uint8 sprlinebuf[256+8] __attribute__ ((aligned (4)));

int32 sphitx;
uint8 sphitdata;
int spork=0;            /* spork the world.  Any sprites on this line?
                           Then this will be set to 1.  Needed for zapper
                           emulation and *gasp* sprite emulation.
                        */


extern void BGRender(uint8 *target);


int maxsprites=8;
static int sprlinebuf_empty=0;


void FCEUI_DisableSpriteLimitation(int a)
{
 maxsprites=a?64:8;
}


//int printed=1;
typedef struct {
        uint8 y,no,atr,x;
} SPR __attribute__((aligned(1)));

typedef struct {
  //	uint8 ca[2],atr,x;
  	uint8 ca[2],atr,x;
  //  union {  int z; }


} SPRB __attribute__((aligned(1)));



static uint8 nosprites,SpriteBlurp;

void FetchSpriteData(void)
{
	SPR *spr;
	uint8 H;
	int n,vofs;

	spr=(SPR *)SPRAM;
	H=8;

	nosprites=SpriteBlurp=0;

        vofs=(unsigned int)(PPU[0]&0x8&(((PPU[0]&0x20)^0x20)>>2))<<9;
	H+=(PPU[0]&0x20)>>2;

        if(!PPU_hook)
         for(n=63;n>=0;n--,spr++)
         {
                if((unsigned int)(scanline-spr->y)>=H) continue;

                if(nosprites<maxsprites)
                {
                 if(n==63) SpriteBlurp=1;

		 {
		  SPRB dst;
		  uint8 *C;
                  int t;
                  unsigned int vadr;

                  t = (int)scanline-(spr->y);

                  if (Sprite16)
                   vadr = ((spr->no&1)<<12) + ((spr->no&0xFE)<<4);
                  else
                   vadr = (spr->no<<4)+vofs;

                  if (spr->atr&V_FLIP)
                  {
                        vadr+=7;
                        vadr-=t;
                        vadr+=(PPU[0]&0x20)>>1;
                        vadr-=t&8;
                  }
                  else
                  {
                        vadr+=t;
                        vadr+=t&8;
                  }

		  /* Fix this geniestage hack */
      	          if(MMC5Hack && geniestage!=1) C = MMC5SPRVRAMADR(vadr);
                  else C = VRAMADR(vadr);


		  dst.ca[0]=C[0];
		  dst.ca[1]=C[8];
		  dst.x=spr->x;
		  dst.atr=spr->atr;


		  *(uint32 *)&SPRBUF[nosprites<<2]=*(uint32 *)&dst;
		 }

                 nosprites++;
                }
                else
                {
                  PPU_status|=0x20;
                  break;
                }
         }
	else
         for(n=63;n>=0;n--,spr++)
         {
                if((unsigned int)(scanline-spr->y)>=H) continue;

                if(nosprites<maxsprites)
                {
                 if(n==63) SpriteBlurp=1;

                 {
                  SPRB dst;
                  uint8 *C;
                  int t;
                  unsigned int vadr;

                  t = (int)scanline-(spr->y);

                  if (Sprite16)
                   vadr = ((spr->no&1)<<12) + ((spr->no&0xFE)<<4);
                  else
                   vadr = (spr->no<<4)+vofs;

                  if (spr->atr&V_FLIP)
                  {
                        vadr+=7;
                        vadr-=t;
                        vadr+=(PPU[0]&0x20)>>1;
                        vadr-=t&8;
                  }
                  else
                  {
                        vadr+=t;
                        vadr+=t&8;
                  }

                  if(MMC5Hack) C = MMC5SPRVRAMADR(vadr);
                  else C = VRAMADR(vadr);
                  dst.ca[0]=C[0];
		  if(nosprites<8)
		  {
		   PPU_hook(0x2000);
                   PPU_hook(vadr);
		  }
                  dst.ca[1]=C[8];
                  dst.x=spr->x;
                  dst.atr=spr->atr;


                  *(uint32 *)&SPRBUF[nosprites<<2]=*(uint32 *)&dst;
                 }

                 nosprites++;
                }
                else
                {
                  PPU_status|=0x20;
                  break;
                }
         }

        if(nosprites>8) PPU_status|=0x20;  /* Handle case when >8 sprites per
					   scanline option is enabled. */
	else if(PPU_hook)
	{
	 for(n=0;n<(8-nosprites);n++)
	 {
                 PPU_hook(0x2000);
                 PPU_hook(vofs);
	 }
	}

}

#ifdef FRAMESKIP
extern int FSkip;
#endif

void RefreshSprites(void)
{
	int n;
        SPRB *spr;

        spork=0;
        if(!nosprites) return;
	#ifdef FRAMESKIP
	if(FSkip)
	{
	 if(!SpriteBlurp)
	 {
	  nosprites=0;
	  return;
	 }
	 else
	  nosprites=1;
	}
	#endif

        nosprites--;
        spr = (SPRB*)SPRBUF+nosprites;

       if (!sprlinebuf_empty)
       {
        FCEU_dwmemset(sprlinebuf,0x80808080,256);
	sprlinebuf_empty = 1;
       }

       for(n=nosprites;n>=0;n--,spr--)
       {
        register uint32 J;

        J=spr->ca[0]|spr->ca[1];

        if (J)
        {
          register uint8 atr,c1,c2;
          uint8 *C;
          uint8 *VB;
	  int x=spr->x;
	  atr=spr->atr;

			if(n==0 && SpriteBlurp && !(PPU_status&0x40))
                        {
                         sphitx=x;
                         sphitdata=J;
                         if(atr&H_FLIP)
                          sphitdata=    ((J<<7)&0x80) |
                                        ((J<<5)&0x40) |
                                        ((J<<3)&0x20) |
                                        ((J<<1)&0x10) |
                                        ((J>>1)&0x08) |
                                        ((J>>3)&0x04) |
                                        ((J>>5)&0x02) |
                                        ((J>>7)&0x01);
                        }


         c1=((spr->ca[0]>>1)&0x55)|(spr->ca[1]&0xAA);
	 c2=(spr->ca[0]&0x55)|((spr->ca[1]<<1)&0xAA);

	 C = sprlinebuf+x;
         VB = (PALRAM+0x10)+((atr&3)<<2);

         {
	  J &= 0xff;
	  if(atr&SP_BACK) J |= 0x4000;
          if (atr&H_FLIP)
          {
           if (J&0x02)  C[1]=VB[c1&3]|(J>>8);
           if (J&0x01)  *C=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x08)  C[3]=VB[c1&3]|(J>>8);
           if (J&0x04)  C[2]=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x20)  C[5]=VB[c1&3]|(J>>8);
           if (J&0x10)  C[4]=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x80)  C[7]=VB[c1]|(J>>8);
           if (J&0x40)  C[6]=VB[c2]|(J>>8);
	  } else  {
           if (J&0x02)  C[6]=VB[c1&3]|(J>>8);
           if (J&0x01)  C[7]=VB[c2&3]|(J>>8);
	   c1>>=2;c2>>=2;
           if (J&0x08)  C[4]=VB[c1&3]|(J>>8);
           if (J&0x04)  C[5]=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x20)  C[2]=VB[c1&3]|(J>>8);
           if (J&0x10)  C[3]=VB[c2&3]|(J>>8);
           c1>>=2;c2>>=2;
           if (J&0x80)  *C=VB[c1]|(J>>8);
           if (J&0x40)  C[1]=VB[c2]|(J>>8);
	  }
         }
	 sprlinebuf_empty = 0;
      }
     }

     nosprites=0;
     spork=1;
}


void CopySprites(uint8 *target)
{
      uint8 n=((PPU[1]&4)^4)<<1;
      //if ((int)n < minx) n = minx & 0xfc;
      loopskie:
      {
       uint32 t=*(uint32 *)(sprlinebuf+n);
       if(t!=0x80808080)
       {
	#ifdef LSB_FIRST
        uint32 tb=*(uint32 *)(target+n);
        if(!(t&0x00000080) && (!(t&0x00000040) || (tb&0x00000040))) { // have sprite pixel AND (normal sprite OR behind bg with no bg)
          tb &= ~0x000000ff; tb |= t & 0x000000ff;
        }

        if(!(t&0x00008000) && (!(t&0x00004000) || (tb&0x00004000))) {
          tb &= ~0x0000ff00; tb |= t & 0x0000ff00;
        }

        if(!(t&0x00800000) && (!(t&0x00400000) || (tb&0x00400000))) {
          tb &= ~0x00ff0000; tb |= t & 0x00ff0000;
        }

        if(!(t&0x80000000) && (!(t&0x40000000) || (tb&0x40000000))) {
          tb &= ~0xff000000; tb |= t & 0xff000000;
        }
	*(uint32 *)(target+n)=tb;
	#else
	#error not implemented
	#endif
       }
      }
      n+=4;
      if(n) goto loopskie;
}


