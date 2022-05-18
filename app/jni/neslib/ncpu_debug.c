#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "x6502.h"
#include "fce.h"

// asm core state
extern uint32 nes_registers[0x10];
extern uint32 pc_base;
extern uint8  nes_internal_ram[0x800];
extern uint32 timestamp_a;
extern uint32 framecount;
extern X6502 X_;
static uint32 framecount_d;
uint32 PC_prev = 0xcccccc, OP_prev = 0xcccccc;
int32  g_cnt = 0;

static int pending_add_cycles = 0, pending_rebase = 0, pending_irq = 0;

uint8  dreads[4];
uint32 dwrites_c[2], dwrites_a[2];
int dread_count_c, dread_count_a, dwrite_count_c, dwrite_count_a;
int mapirq_cyc_c, mapirq_cyc_a;

extern void DumpEmptyCartMapping(void);

static void leave(void)
{
	printf("\nA: %02x, X: %02x, Y: %02x, S: %02x\n", X.A, X.X, X.Y, X.S);
	printf("PC = %04x, OP=%02X\n", PC_prev, OP_prev);
	printf("rest = %08x\n", nes_registers[4]);
	DumpEmptyCartMapping();
	exit(1);
}

static void compare_state(void)
{
	uint8 nes_flags;
	int i, fail = 0;

	if ((nes_registers[0] >> 24) != X.A) {
		printf("A: %02x vs %02x\n", nes_registers[0] >> 24, X.A);
		fail = 1;
	}

	if ((nes_registers[1] & 0xff) != X.X) {
		printf("X: %02x vs %02x\n", nes_registers[1] & 0xff, X.X);
		fail = 1;
	}

	if ((nes_registers[2] & 0xff) != X.Y) {
		printf("Y: %02x vs %02x\n", nes_registers[2] & 0xff, X.Y);
		fail = 1;
	}

	if (nes_registers[3] - pc_base != X.PC) {
		printf("PC: %04x vs %04x\n", nes_registers[3] - pc_base, X.PC);
		fail = 1;
	}

	if ((nes_registers[4] >> 24) != X.S) {
		printf("S: %02x vs %02x\n", nes_registers[4] >> 24, X.S);
		fail = 1;
	}

	if (((nes_registers[4]>>8)&0xff) != X.IRQlow) {
		printf("IRQlow: %02x vs %02x\n", ((nes_registers[4]>>8)&0xff), X.IRQlow);
		fail = 1;
	}

	// NVUB DIZC
	nes_flags = nes_registers[4] & 0x5d;
	if (  nes_registers[5]&0x80000000)  nes_flags |= 0x80; // N
	if (!(nes_registers[5]&0x000000ff)) nes_flags |= 0x02; // Z
	// nes_flags |= 0x20; // U, not set in C core (set only when pushing)

	if (nes_flags != (X.P&~0x20)) {
		printf("flags: %02x vs %02x\n", nes_flags, (X.P&~0x20));
		fail = 1;
	}

	if (((int32)nes_registers[7] >> 16) != X.count) {
		printf("cycles: %i vs %i\n", (int32)nes_registers[7] >> 16, X.count);
		fail = 1;
	}

	if (dread_count_a != dread_count_c) {
		printf("dread_count: %i vs %i\n", dread_count_a, dread_count_c);
		fail = 1;
	}

	if (dwrite_count_a != dwrite_count_c) {
		printf("dwrite_count: %i vs %i\n", dwrite_count_a, dwrite_count_c);
		fail = 1;
	}

	for (i = dwrite_count_a - 1; !fail && i >= 0; i--)
		if (dwrites_a[i] != dwrites_c[i]) {
			printf("dwrites[%i]: %06x vs %06x\n", dwrite_count_a, dwrites_a[i], dwrites_c[i]);
			fail = 1;
		}

	if (mapirq_cyc_a != mapirq_cyc_c) {
		printf("mapirq_cyc: %i vs %i\n", mapirq_cyc_a, mapirq_cyc_c);
		fail = 1;
	}

	if (timestamp_a != timestamp) {
		printf("timestamp: %u vs %u\n", timestamp_a, timestamp);
		fail = 1;
	}

/*
	if (X_.DB != X.DB) {
		printf("DB: %02x vs %02x\n", X_.DB, X.DB);
		fail = 1;
	}
*/
	if (fail) leave();
}

static void compare_ram(void)
{
#if 1
	int i, fail = 0;
	for (i = 0; i < 0x800/4; i++)
	{
		if (((int *)nes_internal_ram)[i] != ((int32 *)RAM)[i]) {
			int u;
			fail = 1;
			for (u = i*4; u < i*4+4; u++)
				if (nes_internal_ram[u] != RAM[u])
					printf("RAM[%03x]: %02x vs %02x\n", u, nes_internal_ram[u], RAM[u]);
		}
	}

	if (fail) leave();
#endif
}

void TriggerIRQ_d(void)
{
	printf("-- irq\n");
	pending_irq |= 0x100;
}

void TriggerNMI_d(void)
{
	printf("-- nmi\n");
	TriggerNMI_c();
	TriggerNMI_a();
	compare_state();
}

void X6502_Run_d(int32 c)
{
	int32 cycles = c << 4; /* *16 */
	if (PAL) cycles -= c;  /* *15 */

	//printf("-- %06i: run(%i)\n", (int)g_cnt, (int)c);
	g_cnt += cycles;

	if (framecount != framecount_d) {
		compare_ram();
		framecount_d = framecount;
	}

	timestamp_a = timestamp;

	while (g_cnt > 0)
	{
		if (pending_irq) {
			if (pending_irq & 0x100) {
				TriggerIRQ_c();
				TriggerIRQ_a();
			}
			if (pending_irq & 0xff) {
				X6502_IRQBegin_c(pending_irq & 0xff);
				X6502_IRQBegin_a(pending_irq & 0xff);
			}
			pending_irq = 0;
		}

		//printf("%04x: %02x\n", nes_registers[3] - pc_base, *(unsigned char *)nes_registers[3]);

		nes_registers[7]=1<<16;
		X.count=1;

		dread_count_c = dread_count_a = dwrite_count_c = dwrite_count_a = 0;
		mapirq_cyc_a = mapirq_cyc_c = 0;
		//timestamp_a = timestamp;

		X6502_Run_c();

		X6502_Run_a();

		compare_state();
		g_cnt -= 1 - X.count;
		if (pending_add_cycles) {
			g_cnt -= pending_add_cycles*48;
			//X6502_AddCycles_c(pending_add_cycles);
			//X6502_AddCycles_a(pending_add_cycles);
			timestamp   += pending_add_cycles;
			timestamp_a += pending_add_cycles;
			pending_add_cycles = 0;
		}
		if (pending_rebase) {
			X6502_Rebase_a();
			pending_rebase = 0;
		}
	}

	//printf("-- run_end\n");
}

void X6502_Reset_d(void)
{
	printf("-- reset\n");

	X6502_Reset_c();
	X6502_Reset_a();
	compare_state();
}

void X6502_Power_d(void)
{
	printf("-- power\n");
	if (nes_internal_ram == RAM) printf("nes_internal_ram == RAM!!\n");
	dread_count_c = dread_count_a = dwrite_count_c = dwrite_count_a = 0;
	mapirq_cyc_c = mapirq_cyc_a = 0;

	X6502_Power_c();
	X6502_Power_a();
	compare_state();

#if 0
	{
		unsigned char *p = (void *) nes_registers[3];
		int i, u, nop = 0xea;

		for (i = 0; i < 256; i++)
		{
			if (i == 0 || i == 0x20 || i == 0x40 || i == 0x60 || i == 0x4c || i == 0x6c) continue; /* BRK, JSR, RET, etc. */
			if ((i & 0x1f) == 0x10) continue; /* Bxx */
			switch (i)
			{
				case 0x02: /* JAM */
				case 0x12:
				case 0x22:
				case 0x32:
				case 0x42:
				case 0x52:
				case 0x62:
				case 0x72:
				case 0x92:
				case 0xB2:
				case 0xD2:
				case 0xF2: continue;
			}

			*p++ = i;
			for (u = 0; u < 3; u++)
				*p++ = nop;
		}
	}
#endif
}

void X6502_AddCycles_d(int x)
{
	printf("-- AddCycles(%i|%i)\n", x, x*48);

	pending_add_cycles = x; // *48;
//	printf("can't use this in debug\n");
//	exit(1);
	//X6502_AddCycles_c(x);
	//X6502_AddCycles_a(x);
	//compare_state();
}

void X6502_IRQBegin_d(int w)
{
	printf("-- IRQBegin(%02x)\n", w);

	// X6502_IRQBegin_c(w);
	// X6502_IRQBegin_a(w);
	pending_irq |= w;
}

void X6502_IRQEnd_d(int w)
{
	printf("-- IRQEnd(%02x)\n", w);

	X6502_IRQEnd_c(w);
	X6502_IRQEnd_a(w);
}


void X6502_Rebase_d(void)
{
	pending_rebase = 1;
}


