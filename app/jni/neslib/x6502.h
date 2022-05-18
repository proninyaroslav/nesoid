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

typedef struct {
        int32 count;            /* Cycle counter           */
        int32 tcount;           /* Temporary cycle counter */
        uint16 PC;		/* I'll change this to uint32 later... */
				/* I'll need to AND PC after increments to 0xFFFF */
				/* when I do, though.  Perhaps an IPC() macro? */
        uint8 A,X,Y,S,P,mooPI;
        uint8 DB;               /* Data bus "cache" for reads from certain areas */
	uint8 IRQlow;		/* Simulated IRQ pin held low(or is it high?). */
	uint8 jammed;
} X6502;

extern X6502 X;

#define N_FLAG  0x80
#define V_FLAG  0x40
#define U_FLAG  0x20
#define B_FLAG  0x10
#define D_FLAG  0x08
#define I_FLAG  0x04
#define Z_FLAG  0x02
#define C_FLAG  0x01

extern uint32 timestamp;
extern void FP_FASTAPASS(1) (*MapIRQHook)(int a);

#define NTSC_CPU 1789772.7272727272727272
#define PAL_CPU  1662607.125

#define FCEU_IQEXT      0x01
#define FCEU_IQNMI	0x08
#define FCEU_IQDPCM     0x10
#define FCEU_IQFCOUNT   0x20
#define FCEU_IQTEMP     0x80
// from 0.98.15
#define FCEU_IQEXT2	0x02

#if defined(DEBUG_ASM_6502)
#define TriggerIRQ TriggerIRQ_d
#define TriggerNMI TriggerNMI_d
#define X6502_Run X6502_Run_d
#define X6502_Reset X6502_Reset_d
#define X6502_Power X6502_Power_d
#define X6502_AddCycles X6502_AddCycles_d
#define X6502_IRQBegin X6502_IRQBegin_d
#define X6502_IRQEnd X6502_IRQEnd_d
#define X6502_Rebase X6502_Rebase_d
#define X6502_GetCycleCount() g_cnt
#define X6502_C
#define X6502_A
#define X6502_D

#elif defined(ASM_6502)
#define TriggerIRQ TriggerIRQ_a
#define TriggerNMI TriggerNMI_a
#define X6502_Reset X6502_Reset_a
#define X6502_Power X6502_Power_a
#define X6502_AddCycles X6502_AddCycles_a
//#define X6502_IRQBegin X6502_IRQBegin_a
//#define X6502_IRQEnd X6502_IRQEnd_a
#define X6502_IRQBegin(w) nes_registers[4]|=w<<8
#define X6502_IRQEnd(w) nes_registers[4]&=~(w<<8)
#define X6502_Rebase X6502_Rebase_a
#define X6502_GetCycleCount() ((int32)nes_registers[7]>>16)
#define X6502_A

#define X6502_Run(c) \
{ \
 int32 cycles = (c) << 4; /* *16 */ \
 if (PAL) cycles -= (c);  /* *15 */ \
 nes_registers[7]+=cycles<<16; \
 cycles=(int32)nes_registers[7]>>16; \
 if (cycles > 0) { \
   X6502_Run_a(); \
   cycles -= (int32)nes_registers[7]>>16; \
   asmcpu_update(cycles); \
 } \
}

#else
#define TriggerIRQ TriggerIRQ_c
#define TriggerNMI TriggerNMI_c
#define X6502_Reset X6502_Reset_c
#define X6502_Power X6502_Power_c
#define X6502_AddCycles X6502_AddCycles_c
#define X6502_IRQBegin X6502_IRQBegin_c
#define X6502_IRQEnd X6502_IRQEnd_c
#define X6502_Rebase(...)
#define X6502_GetCycleCount() X.count
#define X6502_C

#define X6502_Run(c) \
{ \
 int32 cycles = (c) << 4; /* *16 */ \
 if (PAL) cycles -= (c);  /* *15 */ \
 X.count+=cycles; \
 if (X.count > 0) X6502_Run_c(); \
}
#define X6502_C
#endif

// c
#ifdef X6502_C
extern int32 g_cnt;
void TriggerIRQ_c(void);
void TriggerNMI_c(void);
void X6502_Run_c(void);
void X6502_Reset_c(void);
void X6502_Power_c(void);
void FASTAPASS(1) X6502_AddCycles_c(int x);
void FASTAPASS(1) X6502_IRQBegin_c(int w);
void FASTAPASS(1) X6502_IRQEnd_c(int w);
#endif

// asm
#ifdef X6502_A
extern uint32 nes_registers[0x10];
extern uint32 pc_base;
void TriggerIRQ_a(void);
void TriggerNMI_a(void);
void X6502_Run_a(void);
void X6502_Reset_a(void);
void X6502_Power_a(void);
void X6502_AddCycles_a(int x);
void X6502_IRQBegin_a(int w);
void X6502_IRQEnd_a(int w);
void X6502_Rebase_a(void);
#endif

// debug
#ifdef X6502_D
void TriggerIRQ_d(void);
void TriggerNMI_d(void);
void X6502_Run_d(int32 c);
void X6502_Reset_d(void);
void X6502_Power_d(void);
void X6502_AddCycles_d(int x);
void X6502_IRQBegin_d(int w);
void X6502_IRQEnd_d(int w);
void X6502_Rebase_d(void);
#endif

