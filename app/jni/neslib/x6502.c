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

#include <string.h>

#include "types.h"
#include "x6502.h"
#include "fce.h"
#include "sound.h"
#include "cart.h"

#include "dprintf.h"

#ifdef DEBUG_ASM_6502
#include <stdio.h>
#include <stdlib.h>
extern uint32 PC_prev, OP_prev;
extern uint8  dreads[4];
extern uint32 dwrites_c[2];
extern int dread_count_c, dwrite_count_c;
extern int mapirq_cyc_c;
extern void (*MapIRQHook)(int a);
#define DummyRdMem(...)
#else
#define DummyRdMem RdMem
void FP_FASTAPASS(1) (*MapIRQHook)(int a);
#endif

X6502 X;
uint32 timestamp;

#define _PC              X.PC
#define _A               X.A
#define _X               X.X
#define _Y               X.Y
#define _S               X.S
#define _P               X.P
#define _PI              X.mooPI
//#define _PZ              X.PZ		// unused?
#define _DB              X.DB
#define _count           X.count
#define _tcount          X.tcount
#define _IRQlow          X.IRQlow
#define _jammed          X.jammed


static INLINE uint8 RdMem(unsigned int A)
{
 int _DB1=ARead[A](A);
#ifdef DEBUG_ASM_6502
 //_DB=_DB1;
 //printf("a == %x, pc == %x\n", A, _PC);
 if (A >= 0x2000) {
  if (A == _PC || A == _PC - 1 || A == _PC + 1) {
   //printf("fr: %02x\n", _DB1);
  }
  _DB=_DB1;
 }
 if (A >= 0x2000 && A != _PC - 1) {
  dreads[dread_count_c++] = _DB1;
  if (dread_count_c > 4) { printf("dread_count out of range\n"); exit(1); }
 }
#else
 _DB=_DB1;
#endif
 return _DB1;
}

static INLINE void WrMem(unsigned int A, uint8 V)
{
 //printf("w [%04x] %02x\n", A, V);
 if ((A&0xe000) == 0) { // RAM area (always 0-0x1fff)
  RAM[A&0x7FF] = V;
  return;
 }
 BWrite[A](A,V);
#ifdef DEBUG_ASM_6502
 dwrites_c[dwrite_count_c++] = (A<<8)|V;
 if (dwrite_count_c > 2) { printf("dwrite_count_c out of range\n"); exit(1); }
#endif
}

static INLINE uint8 RdRAM(unsigned int A)
{
 //return((_DB=RAM[A]));
 int _DB1=RAM[A];
#ifndef DEBUG_ASM_6502
 _DB=_DB1;
#endif
 return _DB1;
}

static INLINE void WrRAM(unsigned int A, uint8 V)
{
 RAM[A]=V;
}

static INLINE void ADDCYC(int x)
{
 _tcount+=x;
 _count-=x*48;
 timestamp+=x;
}

void FASTAPASS(1) X6502_AddCycles_c(int x)
{
 ADDCYC(x);
}

static INLINE void PUSH(uint8 V)
{
 WrRAM(0x100+_S,V);
 _S--;
}

static INLINE uint8 POP(void)
{
 _S++;
 return(RdRAM(0x100+_S));
}

#if 0
static uint8 ZNTable[256] = {
        Z_FLAG,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
	N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
        N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
        N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
        N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
        N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
        N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
        N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG
};
#endif
/* Some of these operations will only make sense if you know what the flag
   constants are. */
//#define X_ZN(zort)         _P&=~(Z_FLAG|N_FLAG);_P|=ZNTable[zort]
//#define X_ZNT(zort)	_P|=ZNTable[zort]
#define X_ZN(zort)         _P&=~(Z_FLAG|N_FLAG);if(!zort) _P|=Z_FLAG;else _P|=zort&N_FLAG
#define X_ZNT(zort)	if(!zort) _P|=Z_FLAG;else _P|=(zort&N_FLAG)

/* Care must be taken if you want to turn this into a macro.  Use { and }. */
#define JR();	\
{		\
 uint32 tmp;	\
 int8 disp;	\
 disp=RdMem(_PC++);	\
 ADDCYC(1);	\
 tmp=_PC;	\
 _PC+=disp;	\
 if((tmp^_PC)&0x100)	\
  ADDCYC(1);	\
}

#define LDA	   _A=x;X_ZN(_A)
#define LDX	   _X=x;X_ZN(_X)
#define LDY        _Y=x;X_ZN(_Y)

/*  All of the freaky arithmetic operations. */
#define AND        _A&=x;X_ZN(_A)
//#define BIT        _P&=~(Z_FLAG|V_FLAG|N_FLAG);_P|=ZNTable[x&_A]&Z_FLAG;_P|=x&(V_FLAG|N_FLAG)
#define BIT        _P&=~(Z_FLAG|V_FLAG|N_FLAG);if(!(x&_A)) _P|=Z_FLAG;_P|=x&(V_FLAG|N_FLAG)
#define EOR        _A^=x;X_ZN(_A)
#define ORA        _A|=x;X_ZN(_A)

#define ADC  {	\
	      uint32 l=_A+x+(_P&1);	\
	      _P&=~(Z_FLAG|C_FLAG|N_FLAG|V_FLAG);	\
	      _P|=((((_A^x)&0x80)^0x80) & ((_A^l)&0x80))>>1;	\
	      _P|=(l>>8)&C_FLAG;	\
	      _A=l;	\
	      X_ZNT(_A);	\
	     }
#define SBC  {	\
	      uint32 l=_A-x-((_P&1)^1);	\
	      _P&=~(Z_FLAG|C_FLAG|N_FLAG|V_FLAG);	\
	      _P|=((_A^l)&(_A^x)&0x80)>>1;	\
	      _P|=((l>>8)&C_FLAG)^C_FLAG;	\
	      _A=l;	\
	      X_ZNT(_A);	\
	     }

#define CMPL(a1,a2) {	\
		     uint32 t=a1-a2;	\
		     X_ZN(t&0xFF);	\
		     _P&=~C_FLAG;	\
		     _P|=((t>>8)&C_FLAG)^C_FLAG;	\
		    }

/* Special undocumented operation.  Very similar to CMP. */
#define AXS	    {	\
                     uint32 t=(_A&_X)-x;    \
                     X_ZN(t&0xFF);      \
                     _P&=~C_FLAG;       \
                     _P|=((t>>8)&C_FLAG)^C_FLAG;        \
		     _X=t;	\
                    }

#define CMP		CMPL(_A,x)
#define CPX		CMPL(_X,x)
#define CPY	      	CMPL(_Y,x)

/* The following operations modify the byte being worked on. */
#define DEC       	x--;X_ZN(x)
#define INC		x++;X_ZN(x)

#define ASL        _P&=~C_FLAG;_P|=x>>7;x<<=1;X_ZN(x)
#define LSR	_P&=~(C_FLAG|N_FLAG|Z_FLAG);_P|=x&1;x>>=1;X_ZNT(x)

/* For undocumented instructions, maybe for other things later... */
#define LSRA	_P&=~(C_FLAG|N_FLAG|Z_FLAG);_P|=_A&1;_A>>=1;X_ZNT(_A)

#define ROL	{	\
		 uint8 l=x>>7;	\
		 x<<=1;	\
		 x|=_P&C_FLAG;	\
		 _P&=~(Z_FLAG|N_FLAG|C_FLAG);	\
		 _P|=l;	\
		 X_ZNT(x);	\
		}
#define ROR	{	\
		 uint8 l=x&1;	\
		 x>>=1;	\
		 x|=(_P&C_FLAG)<<7;	\
		 _P&=~(Z_FLAG|N_FLAG|C_FLAG);	\
		 _P|=l;	\
		 X_ZNT(x);	\
		}

/* Icky icky thing for some undocumented instructions.  Can easily be
   broken if names of local variables are changed.
*/

/* Absolute */
#define GetAB(target) 	\
{	\
 target=RdMem(_PC++);	\
 target|=RdMem(_PC++)<<8;	\
}

/* Absolute Indexed(for reads) */
#define GetABIRD(target, i)	\
{	\
 unsigned int tmp;	\
 GetAB(tmp);	\
 target=tmp;	\
 target+=i;	\
 if((target^tmp)&0x100)	\
 {	\
  target&=0xFFFF;	\
  DummyRdMem(target^0x100);	\
  ADDCYC(1);	\
 }	\
}

/* Absolute Indexed(for writes and rmws) */
#define GetABIWR(target, i)	\
{	\
 unsigned int rt;	\
 GetAB(rt);	\
 target=rt;	\
 target+=i;	\
 target&=0xFFFF;	\
 DummyRdMem((target&0x00FF)|(rt&0xFF00));	\
}

/* Zero Page */
#define GetZP(target)	\
{	\
 target=RdMem(_PC++); 	\
}

/* Zero Page Indexed */
#define GetZPI(target,i)	\
{	\
 target=i+RdMem(_PC++);	\
}

/* Indexed Indirect */
#define GetIX(target)	\
{	\
 uint8 tmp;	\
 tmp=RdMem(_PC++);	\
 tmp+=_X;	\
 target=RdRAM(tmp++);	\
 target|=RdRAM(tmp)<<8;	\
}

/* Indirect Indexed(for reads) */
#define GetIYRD(target)	\
{	\
 unsigned int rt;	\
 uint8 tmp;	\
 tmp=RdMem(_PC++);	\
 rt=RdRAM(tmp++);	\
 rt|=RdRAM(tmp)<<8;	\
 target=rt;	\
 target+=_Y;	\
 if((target^rt)&0x100)	\
 {	\
  target&=0xFFFF;	\
  DummyRdMem(target^0x100);	\
  ADDCYC(1);	\
 }	\
}

/* Indirect Indexed(for writes and rmws) */
#define GetIYWR(target)	\
{	\
 unsigned int rt;	\
 uint8 tmp;	\
 tmp=RdMem(_PC++);	\
 rt=RdRAM(tmp++);	\
 rt|=RdRAM(tmp)<<8;	\
 target=rt;	\
 target+=_Y;	\
 DummyRdMem((target&0x00FF)|(rt&0xFF00));	\
}

/* Now come the macros to wrap up all of the above stuff addressing mode functions
   and operation macros.  Note that operation macros will always operate(redundant
   redundant) on the variable "x".
*/

#define RMW_A(op) {uint8 x=_A; op; _A=x; break; } /* Meh... */
#define RMW_AB(op) {unsigned int A; uint8 x; GetAB(A); x=RdMem(A); WrMem(A,x); op; WrMem(A,x); break; }
#define RMW_ABI(reg,op) {unsigned int A; uint8 x; GetABIWR(A,reg); x=RdMem(A); WrMem(A,x); op; WrMem(A,x); break; }
#define RMW_ABX(op)	RMW_ABI(_X,op)
#define RMW_ABY(op)	RMW_ABI(_Y,op)
#define RMW_IX(op)  {unsigned int A; uint8 x; GetIX(A); x=RdMem(A); WrMem(A,x); op; WrMem(A,x); break; }
#define RMW_IY(op)  {unsigned int A; uint8 x; GetIYWR(A); x=RdMem(A); WrMem(A,x); op; WrMem(A,x); break; }
#define RMW_ZP(op)  {uint8 A; uint8 x; GetZP(A); x=RdRAM(A); op; WrRAM(A,x); break; }
#define RMW_ZPX(op) {uint8 A; uint8 x; GetZPI(A,_X); x=RdRAM(A); op; WrRAM(A,x); break;}

#define LD_IM(op)	{uint8 x; x=RdMem(_PC++); op; break;}
#define LD_ZP(op)	{uint8 A; uint8 x; GetZP(A); x=RdRAM(A); op; break;}
#define LD_ZPX(op)  {uint8 A; uint8 x; GetZPI(A,_X); x=RdRAM(A); op; break;}
#define LD_ZPY(op)  {uint8 A; uint8 x; GetZPI(A,_Y); x=RdRAM(A); op; break;}
#define LD_AB(op)	{unsigned int A; uint8 x; GetAB(A); x=RdMem(A); op; break; }
#define LD_ABI(reg,op)  {unsigned int A; uint8 x; GetABIRD(A,reg); x=RdMem(A); op; break;}
#define LD_ABX(op)	LD_ABI(_X,op)
#define LD_ABY(op)	LD_ABI(_Y,op)
#define LD_IX(op)	{unsigned int A; uint8 x; GetIX(A); x=RdMem(A); op; break;}
#define LD_IY(op)	{unsigned int A; uint8 x; GetIYRD(A); x=RdMem(A); op; break;}

#define ST_ZP(r)	{uint8 A; GetZP(A); WrRAM(A,r); break;}
#define ST_ZPX(r)	{uint8 A; GetZPI(A,_X); WrRAM(A,r); break;}
#define ST_ZPY(r)	{uint8 A; GetZPI(A,_Y); WrRAM(A,r); break;}
#define ST_AB(r)	{unsigned int A; GetAB(A); WrMem(A,r); break;}
#define ST_ABI(reg,r)	{unsigned int A; GetABIWR(A,reg); WrMem(A,r); break; }
#define ST_ABX(r)	ST_ABI(_X,r)
#define ST_ABY(r)	ST_ABI(_Y,r)
#define ST_IX(r)	{unsigned int A; GetIX(A); WrMem(A,r); break; }
#define ST_IY(r)	{unsigned int A; GetIYWR(A); WrMem(A,r); break; }

static uint8 CycTable[256] =
{
/*       0 1 2 3 4 5 6 7 8 9 a b c d e f */
/*0x00*/ 7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
/*0x10*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x20*/ 6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
/*0x30*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x40*/ 6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
/*0x50*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x60*/ 6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
/*0x70*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0x90*/ 2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
/*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0xB0*/ 2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
/*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xD0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0xE0*/ 2,6,3,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xF0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

void FASTAPASS(1) X6502_IRQBegin_c(int w)
{
 dprintf("IRQB %02x",w);
 _IRQlow|=w;
}

void FASTAPASS(1) X6502_IRQEnd_c(int w)
{
 dprintf("IRQE %02x",w);
 _IRQlow&=~w;
}

void TriggerIRQ_c(void)	/* This function should probably be phased out. */
{
 _IRQlow|=FCEU_IQTEMP;
}

void TriggerNMI_c(void)
{
 _IRQlow|=FCEU_IQNMI;
}

static void TriggerNMIReal(void)
{
 if(!_jammed)
 {
  dprintf("NMI");
  ADDCYC(7);
  PUSH(_PC>>8);
  PUSH(_PC);
  _P&=~B_FLAG;
  PUSH(_P|U_FLAG);
  _PC=RdMem(0xFFFA);
  _PC|=RdMem(0xFFFB)<<8;
#ifdef DEBUG_ASM_6502
  PC_prev = _PC;
  OP_prev = 0x100;
#endif
 }
}

void TriggerIRQReal(void)
{
 if(!(_PI&I_FLAG) && !_jammed)
 {
  dprintf("IRQ");
  ADDCYC(7);
  PUSH(_PC>>8);
  PUSH(_PC);
  _P&=~B_FLAG;
  PUSH(_P|U_FLAG);
  _P|=I_FLAG;
  _PC=RdMem(0xFFFE);
  _PC|=RdMem(0xFFFF)<<8;
#ifdef DEBUG_ASM_6502
  PC_prev = _PC;
  OP_prev = 0x101;
#endif
 }
}

void X6502_Reset_c(void)
{
  _PC=RdMem(0xFFFC);
  _PC|=RdMem(0xFFFD)<<8;
  _jammed=0;
  _PI=_P=I_FLAG;
}

void X6502_Power_c(void)
{
 memset((void *)&X,0,sizeof(X));
 timestamp=0;
 X6502_Reset_c();
}


//int asdc = 0;
void X6502_Run_c(void/*int32 cycles*/)
{
/*
	if(PAL)
	 cycles*=15;          // 15*4=60
	else
	 cycles*=16;          // 16*4=64

	_count+=cycles;
*/
//	if (_count <= 0) asdc++;

	while(_count>0)
	{
	 int32 temp;
	 uint8 b1;

	 if(_IRQlow)
	 {
	  if(_IRQlow&FCEU_IQNMI)
	   TriggerNMIReal();
	  else
	   TriggerIRQReal();

	  _IRQlow&=~(FCEU_IQTEMP|FCEU_IQNMI);
	  if(_count<=0)
	  {
#ifdef DEBUG_ASM_6502
	   if(MapIRQHook) mapirq_cyc_c = _tcount;
	   _tcount=0;
#endif
	   _PI=_P;
	   return; /* Should increase accuracy without a major speed hit. */
	  }
	 }
	 _PI=_P;
#ifdef DEBUG_ASM_6502
	 b1=RdMem(_PC++); _PC--;
#else
	 b1=RdMem(_PC);
#endif
	 ADDCYC(CycTable[b1]);
	 temp=_tcount;

	 temp*=48;

	 fhcnt-=temp;
	 if(fhcnt<=0)
	 {
	  FrameSoundUpdate();
	  fhcnt+=fhinc;
	 }

	 if(PCMIRQCount>0)
	 {
	  PCMIRQCount-=temp;
	  if(PCMIRQCount<=0)
	  {
	   vdis=1;
	   if((PSG[0x10]&0x80) && !(PSG[0x10]&0x40))
	   {
	    extern uint8 SIRQStat;
	    SIRQStat|=0x80;
	    X6502_IRQBegin(FCEU_IQDPCM);
	   }
	  }
	 }

#ifdef DEBUG_ASM_6502
	 PC_prev = _PC;
	 OP_prev = b1;
#endif
	  //printf("$%04x:$%02x\n",_PC,b1);
	 //_PC++;
	 //printf("$%02x\n",b1);
	 _PC++;
         switch(b1)
         {
          #include "ops.h"
         }

	 temp=_tcount; /* Gradius II (J) glitches if _tcount is not used */
	 _tcount=0;

	 if(MapIRQHook) {
#ifdef DEBUG_ASM_6502
	  mapirq_cyc_c = temp;
#endif
	  MapIRQHook(temp);
	 }

#ifdef DEBUG_ASM_6502
	 _PI=_P;
#endif
	}
}


