/*************************************
  Little John GP32
  File : ncpu.S
  Authors : FCA author
            modified and adapted by Yoyo.
            timing fixed, missing opcodes added
	    and adapted for fceu by notaz, 2007.
**************************************/

#include "ncpu.h"

/* emulate (some) dummy reads (may need that because they affect open bus) */
#define DO_DUMMY_READS 1

@@@
@@@ Offets from REG_OP_TABLE
@@@
#define OTOFFS_NES_RAM   (nes_internal_ram - cpu_exec_table)
#define OTOFFS_NES_STACK (nes_stack        - cpu_exec_table)
#define OTOFFS_NES_REGS  (nes_registers    - cpu_exec_table)
#define OTOFFS_PC_BASE   (pc_base          - cpu_exec_table)
#define OTOFFS_IRQ_HOOK  (MapIRQHook       - cpu_exec_table)
#define OTOFFS_TIMESTAMP (timestamp        - cpu_exec_table)
#define OTOFFS_X         (X_               - cpu_exec_table)

@ fceu
#define FCEU_IQNMI      0x08
#define FCEU_IQTEMP     0x80


@@@@@@@@@@@@@@@@@@@@@@@@@@


@	SECTION_FAST
	SECTION_TEXT
	ALIGN

/*
bbbb:
.ascii "ab_a: %04x"
.byte 0x0a,0
.align 4
stmfd sp!,{r0-r3,r12,lr}
mov r1,r0
ldr r0,=bbbb
bl printf
ldmfd sp!,{r0-r3,r12,lr}
*/

@@@
@@@ r0	=  Address (unbased)
@@@ uses REG_OP_TABLE; sets REG_PC; trashes r1,r2; keeps r0
@@@
.macro	REBASE_PC
	@ FIXME: do something with mem not in Page[].
@	stmfd	sp!, {r0-r3,r12,lr}
@	mov	r1, r0
@	ldr     r0, =bbbb
@	bl      printf
@	ldmfd	sp!, {r0-r3,r12,lr}

	cmp     r0, #0x2000
	ldrge	r1, =Page
	movge	r2, r0, lsr #11
	ldrge	r2, [r1, r2, lsl #2]
	andlt	r2, r0, #0xf800
	addlt	r1, REG_OP_TABLE, #OTOFFS_NES_RAM
	sublt	r2, r1, r2
	str	r2, [REG_OP_TABLE, #OTOFFS_PC_BASE]
	add	REG_PC, r2, r0
.endm


@ updates fceu "timestamp" variable
@ loads cycles to reg, reg!=r1, trashes r1, kills flags
.macro	FLUSH_TIMESTAMP reg
	ands	\reg, REG_CYCLE, #0xff
	beq	1f
	ldr	r1, [REG_OP_TABLE, #OTOFFS_TIMESTAMP]
	orr	REG_CYCLE, REG_CYCLE, \reg, lsl #8	@ put cycles for do_irq_hook
	add	r1, r1, \reg
	bic	REG_CYCLE, REG_CYCLE, #0xff
	str	r1, [REG_OP_TABLE, #OTOFFS_TIMESTAMP]
1:
.endm


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ CYCLE_NEXT
@@@
@@@ 残りサイクルをチェックして残っていれば次の命令を実行
@@@ ただし割り込みをチェックしない
@@@
@@@ なんか名前が変だな(どーでもいーけどー
@@@
.macro	CYCLE_NEXT	n, unused=0, do_cyc_add=1
	@@DEBUG_INFO

.if \do_cyc_add
	add	REG_CYCLE, REG_CYCLE, #\n
.endif
	subs	REG_CYCLE, REG_CYCLE, #\n*48<<16
	ble	cpu_exec_end
	tst	REG_P_REST, #1<<16
	bne	do_irq_hook

	ldrb	r0, [REG_PC], #1
	tst	REG_P_REST, #0xff<<8
	ldreq	pc, [REG_OP_TABLE, r0, lsl #2]

	@ do some messing to find out which IRQ is pending..
	@ assumption: NMI can be set only on very first run, because it is only set once before vblank..
@	tst	REG_P_REST, #FCEU_IQNMI<<8
@	bne	do_int
	tst	REG_P_REST, #P_REST_I_FLAG
	@@ if I_FLAG=1, continue execution, don't trigger IRQ
	bicne	REG_P_REST, REG_P_REST, #FCEU_IQTEMP<<8
	ldrne	pc, [REG_OP_TABLE, r0, lsl #2]
	@@ I_FLAG=0 and REST is checked, we have a IRQ
	b	do_int
.endm

@ fceu needs timestamp cycles to be inremented before doing actual opcode.
@ this is only needed for ops which do memory i/o
.macro	CYCLE_PRE	n
	add	REG_CYCLE, REG_CYCLE, #\n
.endm


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ CYCLE_NEXT_INT
@@@
@@@ 残りサイクルをチェックして残っていれば次の命令を実行
@@@ CLI・PHPなどフラグを変更した場合はこれ
@@@
.macro	CYCLE_NEXT_INT	n
	CYCLE_NEXT	\n
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ ゼロページから読む
@@@
.macro	ZP_READ
	add	REG_ADDR, REG_ADDR, #OTOFFS_NES_RAM
	ldrb	r0, [REG_ADDR, REG_OP_TABLE]!
.endm

@@@
@@@ 読んだ後，書く場合はこれを使う(シフトなど)。でも，上と同じ
@@@
.macro	ZP_READ_W
	ZP_READ
.endm

@@@
@@@ アドレスを読む
@@@
.macro	ZP_READ_ADDR update_db=0
	ZP_READ
	ldrb	REG_ADDR, [REG_ADDR, #1]
	orr	REG_ADDR, r0, REG_ADDR, lsl #8
.if \update_db
	strb    r0, [REG_OP_TABLE, #(OTOFFS_X + 0x10)]		@ X.DB
.endif
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ ゼロページに書く
@@@
.macro	ZP_WRITE	reg=r0
	add	REG_ADDR, REG_ADDR, #OTOFFS_NES_RAM
	strb	\reg, [REG_ADDR, REG_OP_TABLE]
.endm

@@@
@@@ ZP_READ_W で読んだ後，同じアドレスに書く
@@@
.macro	ZP_WRITE_W
	strb	r0, [REG_ADDR]
.endm


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ 16ビットアドレスの読み書き
@@@
@@@ REG_ADDR	=> 16ビットアドレス
@@@ r0		=> 書くバイト(write)
@@@ r0		<= 読んだバイト(read)・書いたバイト(write)
@@@

@@@
@@@ 3種類の命令がある
@@@
@@@ 一番多いのが
@@@ 16ビットアドレスから8ビットロードするだけの命令
@@@ 次に多いのが
@@@ ロード，計算，ストアをする命令
@@@ 残りは
@@@ ストアするだけの命令

@@@
@@@ Read byte
@@@

.macro	READ unused_param
	tst	REG_ADDR, #0xe000
	@ RAM
	biceq	r0, REG_ADDR, #0x1800
	addeq	r0, r0, #OTOFFS_NES_RAM
	ldreqb	r0, [r0, REG_OP_TABLE]
	blne	read_byte
.endm

@@@
@@@ 読んだあと書く場合
@@@
@@@ READ_WRITE_1
@@@ OP
@@@ READ_WRITE_2
@@@
@@@ READ_WRITE_3
@@@ OP
@@@ READ_WRITE_4
@@@ のように使う
@@@ OPではr3を保存しなければならない

.macro	READ_WRITE_1
	tst	REG_ADDR, #0xe000
	adrne	lr, 9999f
	bne	read_byte
	@ RAM
	bic	REG_ADDR, REG_ADDR, #0x1800
	add	REG_ADDR, REG_ADDR, #OTOFFS_NES_RAM
	ldrb	r0, [REG_ADDR, REG_OP_TABLE]!
.endm

.macro	READ_WRITE_2
	strb	r0, [REG_ADDR]
.endm

.macro	READ_WRITE_3
9999:
	bl	write_byte	@ rmw first writes unmodified data
.endm

.macro	READ_WRITE_4
	bl	write_byte	@ and only then modified (Blaster Master)
.endm

@@@
@@@ Write r0 to [addr]
@@@

.macro	WRITE_1
	@@DEBUG_INFO

	tst	REG_ADDR, #0xe000
	biceq	REG_ADDR, REG_ADDR, #0x1800
	addeq	REG_ADDR, REG_ADDR, #OTOFFS_NES_RAM

	streqb	r0, [REG_ADDR, REG_OP_TABLE]
	blne	write_byte
.endm

@@@
@@@ そして，これを必要とするのはジャンプ命令・割り込み・リセットのみである
@@@ あまり重要でないので適当でよい。
@@@ テンポラリに使えるレジスタが足りないので REG_PC を使うが気にするな
@@@ すぐ後で上書きされる。
@@@ REG_ADDRを変更するが気にするな
@@@
.macro	READ_WORD
	READ    0
	mov	REG_PC, r0
	add	REG_ADDR, REG_ADDR, #1
	READ	0
	orr	r0, REG_PC, r0, lsl #8
.endm


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ スタックのプッシュ・ポップ
@@@
@@@

@@  Push byte <- r0
.macro	PUSH_BYTE
	add	r1, REG_OP_TABLE, #OTOFFS_NES_STACK
	strb	r0, [r1, REG_S, lsr #24]
	sub	REG_S, REG_S, #1 << 24
.endm

@@  Push word <- r0
.macro	PUSH_WORD
	add	r1, REG_OP_TABLE, #OTOFFS_NES_STACK
	mov	r2, r0, lsr #8
	strb	r2, [r1, REG_S, lsr #24]
	@@ いちおう REG_S のオーバーフローを気にしてみる
	sub	REG_S, REG_S, #1 << 24
	strb	r0, [r1, REG_S, lsr #24]
	sub	REG_S, REG_S, #1 << 24
.endm

@@  Pop byte -> r0
.macro	POP_BYTE
	add	r0, REG_OP_TABLE, #OTOFFS_NES_STACK
	add	REG_S, REG_S, #1 << 24
	ldrb	r0, [r0, REG_S, lsr #24]
.endm

@@  Pop word -> r0
.macro	POP_WORD
	add	r0, REG_OP_TABLE, #OTOFFS_NES_STACK
	add	REG_S, REG_S, #1 << 24
	ldrb	r1, [r0, REG_S, lsr #24]
	add	REG_S, REG_S, #1 << 24
	ldrb	r0, [r0, REG_S, lsr #24]
	orr	r0, r1, r0, lsl #8
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ アドレッシングモード
@@@

.macro	IMPLIED
	@@ なにも必要ない
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ オペランドが1バイトの場合
@@@

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ #$nn
.macro	IMM_VALUE
	ldrb	r0, [REG_PC], #1
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ $nn
.macro	ZERO_ADDR
	ldrb	REG_ADDR, [REG_PC], #1
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ $nn, X
.macro	ZEROX_ADDR
	ZERO_ADDR
	add	REG_ADDR, REG_ADDR, REG_X
	and	REG_ADDR, REG_ADDR, #0xFF
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ $nn, Y
.macro	ZEROY_ADDR
	ZERO_ADDR
	add	REG_ADDR, REG_ADDR, REG_Y
	and	REG_ADDR, REG_ADDR, #0xFF
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ ($nn, X)
.macro	INDX_ADDR
	ZEROX_ADDR
	ZP_READ_ADDR
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ ($nn), Y
.macro	INDY_ADDR
	ZERO_ADDR
	ZP_READ_ADDR 1                            @ SMB3 relies on open bus here
	add	REG_ADDR, REG_ADDR, REG_Y
	bic	REG_ADDR, REG_ADDR, #0x10000
	and	r0,REG_ADDR,#0xff
	cmp	REG_Y,r0
	addgt	REG_CYCLE, REG_CYCLE, #1
	subgt	REG_CYCLE, REG_CYCLE, #1*48<<16
.endm

@ Indirect Indexed (for writes and rmws)
.macro	INDY_ADDR_W
	ZERO_ADDR
	ZP_READ_ADDR
	add	REG_ADDR, REG_ADDR, REG_Y
	bic	REG_ADDR, REG_ADDR, #0x10000
.endm


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ オペランドが2バイトの場合
@@@

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ $nnnn

.macro	ABS_ADDR update_db=0
.if \update_db
	ldrb	r0, [REG_PC, #1]
	ldrb	REG_ADDR, [REG_PC], #2
	strb    r0, [REG_OP_TABLE, #(OTOFFS_X + 0x10)]		@ X.DB
	orr	REG_ADDR, REG_ADDR, r0, lsl #8
.else
	tst	REG_PC, #1
	ldrneb	REG_ADDR, [REG_PC], #1
	ldrneb	r0, [REG_PC], #1
	ldreqh	REG_ADDR, [REG_PC], #2
	orrne	REG_ADDR, REG_ADDR, r0, lsl #8
.endif
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ $nnnn, X
.macro	ABSX_ADDR
	ABS_ADDR DO_DUMMY_READS
	add	REG_ADDR, REG_ADDR, REG_X
	bic	REG_ADDR, REG_ADDR, #0x10000
	and	r0,REG_ADDR,#0xff
	cmp	REG_X,r0
	addgt	REG_CYCLE, REG_CYCLE, #1
	subgt	REG_CYCLE, REG_CYCLE, #1*48<<16
.endm

@ Absolute Indexed (for writes and rmws)
.macro	ABSX_ADDR_W
	ABS_ADDR
	add	REG_ADDR, REG_ADDR, REG_X
	bic	REG_ADDR, REG_ADDR, #0x10000
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@ $nnnn, Y
.macro	ABSY_ADDR
	ABS_ADDR DO_DUMMY_READS				@ a hack needed for Paperboy, Dirty Harry controls to work
	add	REG_ADDR, REG_ADDR, REG_Y
	bic	REG_ADDR, REG_ADDR, #0x10000
	and	r0,REG_ADDR,#0xff
	cmp	REG_Y,r0
	addgt	REG_CYCLE, REG_CYCLE, #1
	subgt	REG_CYCLE, REG_CYCLE, #1*48<<16
.endm

@ Absolute Indexed (for writes and rmws)
.macro	ABSY_ADDR_W
	ABS_ADDR
	add	REG_ADDR, REG_ADDR, REG_Y
	bic	REG_ADDR, REG_ADDR, #0x10000
.endm

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@@@ P を復元する
@@@ r0		=> 6502 の P レジスタ
@@@ REG_NZ	<= 復元された REG_NZ
@@@ REG_P_REST	<= 復元された REG_P_REST
.macro	LOAD_P
	mov	REG_NZ, #0
	tst	r0, #P_Z_FLAG
	moveq	REG_NZ, #0x01
	tst	r0, #P_N_FLAG
	orrne	REG_NZ, REG_NZ, #0x80 << 24

	bic	REG_P_REST, REG_P_REST, #P_REST_FLAGS
	and	r0, r0, #P_REST_FLAGS
	orr	REG_P_REST, REG_P_REST, r0
.endm

@@@ P を保存する
@@@
@@@ REG_NZ	=> 元にする REG_NZ
@@@ REG_P_REST	=> 元にする REG_P_REST
@@@ r0		<= 6502 の P
.macro	SAVE_P
	and	r0, REG_P_REST, #P_REST_FLAGS
	tst	REG_NZ, #0x80 << 24
	orrne	r0, r0, #P_N_FLAG
	tst	REG_NZ, #0xFF
	orreq	r0, r0, #P_Z_FLAG
	orr	r0, r0, #P_R_FLAG
.endm

@	SECTION_FAST
	SECTION_TEXT
	ALIGN
@@@
@@@ Op-codes
@@@


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ NOP
@@@
@@@ なにもしない
opEA:	@ NOP
	IMPLIED
	CYCLE_NEXT	2

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ CLC/SEC/CLD/SED/CLI/SEI/CLV
@@@
@@@ フラグ操作命令
@@@ すべて1バイト
@@@
@@@ これで変更できるフラグは全てREG_P_RESTにある
@@@ Iを変更する場合は割り込みをチェックしなればならない
@@@
op18:	@ CLC
	IMPLIED
	bic	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	CYCLE_NEXT	2

op38:	@ SEC
	IMPLIED
	orr	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	CYCLE_NEXT	2

opD8:	@ CLD
	IMPLIED
	bic	REG_P_REST, REG_P_REST, #P_REST_D_FLAG
	CYCLE_NEXT	2

opF8:	@ SED
	IMPLIED
	orr	REG_P_REST, REG_P_REST, #P_REST_D_FLAG
	CYCLE_NEXT	2

op58:	@ CLI
	IMPLIED
	bic	REG_P_REST, REG_P_REST, #P_REST_I_FLAG
	CYCLE_NEXT_INT	2

op78:	@ SEI
	IMPLIED
	orr	REG_P_REST, REG_P_REST, #P_REST_I_FLAG
	CYCLE_NEXT	2

opB8:	@ CLV
	IMPLIED
	bic	REG_P_REST, REG_P_REST, #P_REST_V_FLAG
	CYCLE_NEXT	2

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ TAX/TXA/TAY/TYA/TSX/TXS
@@@
@@@ レジスタ間転送命令
@@@ すべて1バイト
@@@
@@@ TXS以外はNZを変更する
@@@
opAA:	@ TAX
	IMPLIED
	mov	REG_X, REG_A, lsr #24
	mov	REG_NZ, REG_A, asr #24
	CYCLE_NEXT	2

op8A:	@ TXA
	IMPLIED
	mov	REG_A, REG_X, lsl #24
	mov	REG_NZ, REG_A, asr #24
	CYCLE_NEXT	2

opA8:	@ TAY
	IMPLIED
	mov	REG_Y, REG_A, lsr #24
	mov	REG_NZ, REG_A, asr #24
	CYCLE_NEXT	2

op98:	@ TYA
	IMPLIED
	mov	REG_A, REG_Y, lsl #24
	mov	REG_NZ, REG_A, asr #24
	CYCLE_NEXT	2

opBA:	@ TSX
	IMPLIED
	mov	REG_X, REG_S, lsr #24
	orr	REG_NZ, REG_X, REG_X, lsl #24
	CYCLE_NEXT	2

op9A:	@ TXS
	IMPLIED
	bic	REG_S, REG_S, #0xFF << 24
	orr	REG_S, REG_S, REG_X, lsl #24
	CYCLE_NEXT	2


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ PHA/PHP/PLA/PLP
@@@
@@@ スタック操作命令
@@@ PLAはNZをPLPは全てのフラグを変更する
@@@
op48:	@ PHA
	IMPLIED
	mov	r0, REG_A, lsr #24
	PUSH_BYTE
	CYCLE_NEXT	3

op08:	@ PHP
	IMPLIED
	SAVE_P
	orr	r0, r0, #P_B_FLAG
	PUSH_BYTE
	CYCLE_NEXT	3

op68:	@ PLA
	IMPLIED
	POP_BYTE
	mov	REG_A, r0, lsl #24
	mov	REG_NZ, REG_A, asr #24
	CYCLE_NEXT	4

op28:	@ PLP
	IMPLIED
	POP_BYTE
	LOAD_P
	CYCLE_NEXT	4

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ LDA/LDX/LDY
@@@
@@@ レジスタにロード
@@@
@@@ NZVC
@@@ ----
@@@ **--
.macro	OP_LDA
	mov	REG_A, r0, lsl #24
	mov	REG_NZ, REG_A, asr #24
.endm

.macro	OP_LDXY	reg
	mov	\reg, r0
	orr	REG_NZ, r0, r0, lsl #24
.endm

.macro	OP_LDX
	OP_LDXY	REG_X
.endm
.macro	OP_LDY
	OP_LDXY REG_Y
.endm

opA9:	@ LDA #$nn
	IMM_VALUE
	OP_LDA
	CYCLE_NEXT	2

opA5:	@ LDA $nn
	ZERO_ADDR
	ZP_READ
	OP_LDA
	CYCLE_NEXT	3

opB5:	@ LDA $nn, X
	ZEROX_ADDR
	ZP_READ
	OP_LDA
	CYCLE_NEXT	4

opAD:	@ LDA $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_LDA
	CYCLE_NEXT	4,1,0

opBD:	@ LDA $nnnn, X
	CYCLE_PRE	4
	ABSX_ADDR
	READ
	OP_LDA
	CYCLE_NEXT	4,1,0

opB9:	@ LDA $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_LDA
	CYCLE_NEXT	4,1,0

opA1:	@ LDA ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	READ
	OP_LDA
	CYCLE_NEXT	6,1,0

opB1:	@ LDA ($nn), Y
	CYCLE_PRE	5
	INDY_ADDR
	READ
	OP_LDA
	CYCLE_NEXT	5,1,0

opA2:	@ LDX #$nn
	IMM_VALUE
	OP_LDX
	CYCLE_NEXT	2

opA6:	@ LDX $nn
	ZERO_ADDR
	ZP_READ
	OP_LDX
	CYCLE_NEXT	3

opB6:	@ LDX $nn, Y
	ZEROY_ADDR
	ZP_READ
	OP_LDX
	CYCLE_NEXT	4

opAE:	@ LDX $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_LDX
	CYCLE_NEXT	4,1,0

opBE:	@ LDX $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_LDX
	CYCLE_NEXT	4,1,0



opA0:	@ LDY #$nn
	IMM_VALUE
	OP_LDY
	CYCLE_NEXT	2

opA4:	@ LDY $nn
	ZERO_ADDR
	ZP_READ
	OP_LDY
	CYCLE_NEXT	3

opB4:	@ LDY $nn, X
	ZEROX_ADDR
	ZP_READ
	OP_LDY
	CYCLE_NEXT	4



opAC:	@ LDY $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_LDY
	CYCLE_NEXT	4,1,0

opBC:	@ LDY $nnnn, X
	CYCLE_PRE	4
	ABSX_ADDR
	READ
	OP_LDY
	CYCLE_NEXT	4,1,0

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ STA/STX/STY
@@@
@@@ レジスタのストア
@@@
@@@ NZVC
@@@ ----
@@@ ----
.macro	OP_STA
	mov	r0, REG_A, lsr #24
.endm

op85:	@ STA $nn
	ZERO_ADDR
	OP_STA
	ZP_WRITE
	CYCLE_NEXT	3

op95:	@ STA $nn, X
	ZEROX_ADDR
	OP_STA
	ZP_WRITE
	CYCLE_NEXT	4

op8D:	@ STA $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	OP_STA
	WRITE_1
	CYCLE_NEXT	4,1,0

op9D:	@ STA $nnnn, X
	CYCLE_PRE	5
	ABSX_ADDR_W
	OP_STA
	WRITE_1
	CYCLE_NEXT	5,1,0

op99:	@ STA $nnnn, Y
	CYCLE_PRE	5
	ABSY_ADDR_W
	OP_STA
	WRITE_1
	CYCLE_NEXT	5,1,0

op81:	@ STA ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	OP_STA
	WRITE_1
	CYCLE_NEXT	6,1,0

op91:	@ STA ($nn), Y
	CYCLE_PRE	6
	INDY_ADDR_W
	OP_STA
	WRITE_1
	CYCLE_NEXT	6,1,0


op86:	@ STX $nn
	ZERO_ADDR
	ZP_WRITE	REG_X
	CYCLE_NEXT	3

op96:	@ STX $nn, Y
	ZEROY_ADDR
	ZP_WRITE	REG_X
	CYCLE_NEXT	4

op8E:	@ STX $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	mov	r0, REG_X
	WRITE_1
	CYCLE_NEXT	4,1,0


op84:	@ STY $nn
	ZERO_ADDR
	ZP_WRITE	REG_Y
	CYCLE_NEXT	3

op94:	@ STY $nn, X
	ZEROX_ADDR
	ZP_WRITE	REG_Y
	CYCLE_NEXT	4

op8C:	@ STY $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	mov	r0, REG_Y
	WRITE_1
	CYCLE_NEXT	4,1,0


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ INC/INX/INY/DEC/DEX/DEY
@@@ インクリメント，デクリメント
@@@ すべてNZを変更する。Cは変更しない
@@@
@@@ NZVC
@@@ ----
@@@ **--

.macro	OP_INC
	add	r0, r0, #1
	and	r0, r0, #0xFF
	orr	REG_NZ, r0, r0, lsl #24
.endm

.macro	OP_INXY	reg
	add	\reg, \reg, #1
	and	\reg, \reg, #0xFF
	orr	REG_NZ, \reg, \reg, lsl #24
.endm

.macro	OP_DEC
	sub	r0, r0, #1
	and	r0, r0, #0xFF
	orr	REG_NZ, r0, r0, lsl #24
.endm

.macro	OP_DEXY	reg
	sub	\reg, \reg, #1
	and	\reg, \reg, #0xFF
	orr	REG_NZ, \reg, \reg, lsl #24
.endm

opE6:	@ INC $nn
	ZERO_ADDR
	ZP_READ_W
	OP_INC
	ZP_WRITE_W
	CYCLE_NEXT	5

opF6:	@ INC $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_INC
	ZP_WRITE_W
	CYCLE_NEXT	6

opEE:	@ INC $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_INC
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_INC
	READ_WRITE_4
	CYCLE_NEXT	6,1,0

opFE:	@ INC $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_INC
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_INC
	READ_WRITE_4
	CYCLE_NEXT	7,1,0

opE8:	@ INX
	IMPLIED
	OP_INXY	REG_X
	CYCLE_NEXT	2

opC8:	@ INY
	IMPLIED
	OP_INXY	REG_Y
	CYCLE_NEXT	2

opC6:	@ DEC $nn
	ZERO_ADDR
	ZP_READ_W
	OP_DEC
	ZP_WRITE_W
	CYCLE_NEXT	5

opD6:	@ DEC $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_DEC
	ZP_WRITE_W
	CYCLE_NEXT	6

opCE:	@ DEC $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_DEC
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_DEC
	READ_WRITE_4
	CYCLE_NEXT	6,1,0

opDE:	@ DEC $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_DEC
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_DEC
	READ_WRITE_4
	CYCLE_NEXT	7,1,0

opCA:	@ DEX
	IMPLIED
	OP_DEXY	REG_X
	CYCLE_NEXT	2

op88:	@ DEY
	IMPLIED
	OP_DEXY	REG_Y
	CYCLE_NEXT	2


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ ADC/SBC
@@@
@@@ キャリー/ボローつきの加算/減算

.macro	OP_ADC
	@@ キャリーをセットして
	movs	r1, REG_P_REST, lsr #1
	@@ 上24ビットを全て1に
	subcs	r0, r0, #0x100
	@@ キャリーつきの加算
	adcs	REG_A, REG_A, r0, ror #8
	mov	REG_NZ, REG_A, asr #24
	adc	REG_P_REST, r1, r1
	orrvs	REG_P_REST, REG_P_REST, #P_REST_V_FLAG
	bicvc	REG_P_REST, REG_P_REST, #P_REST_V_FLAG
.endm

.macro	OP_SBC
	@@ キャリーをセットして
	movs	r1, REG_P_REST, lsr #1
	@@ キャリーつきの減算
	sbcs	REG_A, REG_A, r0, lsl #24
	and	REG_A, REG_A, #0xFF << 24
	mov	REG_NZ, REG_A, asr #24
	adc	REG_P_REST, r1, r1
	orrvs	REG_P_REST, REG_P_REST, #P_REST_V_FLAG
	bicvc	REG_P_REST, REG_P_REST, #P_REST_V_FLAG
.endm

op69:	@ ADC #$nn
	IMM_VALUE
	OP_ADC
	CYCLE_NEXT	2

op65:	@ ADC $nn
	ZERO_ADDR
	ZP_READ
	OP_ADC
	CYCLE_NEXT	3

op75:	@ ADC $nn, X
	ZEROX_ADDR
	ZP_READ
	OP_ADC
	CYCLE_NEXT	4

op6D:	@ ADC $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_ADC
	CYCLE_NEXT	4,1,0

op7D:	@ ADC $nnnn, X
	CYCLE_PRE	4
	ABSX_ADDR
	READ
	OP_ADC
	CYCLE_NEXT	4,1,0

op79:	@ ADC $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_ADC
	CYCLE_NEXT	4,1,0

op61:	@ ADC ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	READ
	OP_ADC
	CYCLE_NEXT	6,1,0

op71:	@ ADC ($nn), Y
	CYCLE_PRE	5
	INDY_ADDR
	READ
	OP_ADC
	CYCLE_NEXT	5,1,0

opEB:	@ USBC #$nn
opE9:	@ SBC #$nn
	IMM_VALUE
	OP_SBC
	CYCLE_NEXT	2

opE5:	@ SBC $nn
	ZERO_ADDR
	ZP_READ
	OP_SBC
	CYCLE_NEXT	3

opF5:	@ SBC $nn, X
	ZEROX_ADDR
	ZP_READ
	OP_SBC
	CYCLE_NEXT	4

opED:	@ SBC $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_SBC
	CYCLE_NEXT	4,1,0

opFD:	@ SBC $nnnn, X
	CYCLE_PRE	4
	ABSX_ADDR
	READ
	OP_SBC
	CYCLE_NEXT	4,1,0

opF9:	@ SBC $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_SBC
	CYCLE_NEXT	4,1,0

opE1:	@ SBC ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	READ
	OP_SBC
	CYCLE_NEXT	6,1,0

opF1:	@ SBC ($nn), Y
	CYCLE_PRE	5
	INDY_ADDR
	READ
	OP_SBC
	CYCLE_NEXT	5,1,0


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ AND/EOR/ORA
@@@
@@@ ビット演算。すべてNZを変更する
@@@
@@@ NZVC
@@@ ----
@@@ **--
.macro	OP_AND
	and	REG_A, REG_A, r0, lsl #24
	mov	REG_NZ, REG_A, asr #24
.endm

.macro	OP_EOR
	eor	REG_A, REG_A, r0, lsl #24
	mov	REG_NZ, REG_A, asr #24
.endm

.macro	OP_ORA
	orr	REG_A, REG_A, r0, lsl #24
	mov	REG_NZ, REG_A, asr #24
.endm

op29:	@ AND #$nn
	IMM_VALUE
	OP_AND
	CYCLE_NEXT	2

op25:	@ AND $nn
	ZERO_ADDR
	ZP_READ
	OP_AND
	CYCLE_NEXT	3

op35:	@ AND $nn, X
	ZEROX_ADDR
	ZP_READ
	OP_AND
	CYCLE_NEXT	4

op2D:	@ AND $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_AND
	CYCLE_NEXT	4,1,0

op3D:	@ AND $nnnn, X
	CYCLE_PRE	4
	ABSX_ADDR
	READ
	OP_AND
	CYCLE_NEXT	4,1,0

op39:	@ AND $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_AND
	CYCLE_NEXT	4,1,0

op21:	@ AND ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	READ
	OP_AND
	CYCLE_NEXT	6,1,0

op31:	@ AND ($nn), Y
	CYCLE_PRE	5
	INDY_ADDR
	READ
	OP_AND
	CYCLE_NEXT	5,1,0


op49:	@ EOR #$nn
	IMM_VALUE
	OP_EOR
	CYCLE_NEXT	2

op45:	@ EOR $nn
	ZERO_ADDR
	ZP_READ
	OP_EOR
	CYCLE_NEXT	3

op55:	@ EOR $nn, X
	ZEROX_ADDR
	ZP_READ
	OP_EOR
	CYCLE_NEXT	4

op4D:	@ EOR $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_EOR
	CYCLE_NEXT	4,1,0

op5D:	@ EOR $nnnn, X
	CYCLE_PRE	4
	ABSX_ADDR
	READ
	OP_EOR
	CYCLE_NEXT	4,1,0

op59:	@ EOR $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_EOR
	CYCLE_NEXT	4,1,0

op41:	@ EOR ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	READ
	OP_EOR
	CYCLE_NEXT	6,1,0

op51:	@ EOR ($nn), Y
	CYCLE_PRE	5
	INDY_ADDR
	READ
	OP_EOR
	CYCLE_NEXT	5,1,0


op09:	@ ORA #$nn
	IMM_VALUE
	OP_ORA
	CYCLE_NEXT	2

op05:	@ ORA $nn
	ZERO_ADDR
	ZP_READ
	OP_ORA
	CYCLE_NEXT	3

op15:	@ ORA $nn, X
	ZEROX_ADDR
	ZP_READ
	OP_ORA
	CYCLE_NEXT	4

op0D:	@ ORA $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_ORA
	CYCLE_NEXT	4,1,0

op1D:	@ ORA $nnnn, X
	CYCLE_PRE	4
	ABSX_ADDR
	READ
	OP_ORA
	CYCLE_NEXT	4,1,0

op19:	@ ORA $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_ORA
	CYCLE_NEXT	4,1,0

op01:	@ ORA ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	READ
	OP_ORA
	CYCLE_NEXT	6,1,0

op11:	@ ORA ($nn), Y
	CYCLE_PRE	5
	INDY_ADDR
	READ
	OP_ORA
	CYCLE_NEXT	5,1,0



@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ CMP/CPX/CPY
@@@
@@@ 比較。すべてNZCを変更する
@@@
@@@ NZVC
@@@ ----
@@@ **-*
.macro	OP_CMP
	subs	REG_NZ, REG_A, r0, lsl #24
	mov	REG_NZ, REG_NZ, asr #24
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
.endm

.macro	OP_CPXY	reg
	mov	r1, \reg, lsl #24
	subs	REG_NZ, r1, r0, lsl #24
	mov	REG_NZ, REG_NZ, asr #24
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
.endm

.macro	OP_CPX
	OP_CPXY	REG_X
.endm

.macro	OP_CPY
	OP_CPXY	REG_Y
.endm

opC9:	@ CMP #$nn
	IMM_VALUE
	OP_CMP
	CYCLE_NEXT	2

opC5:	@ CMP $nn
	ZERO_ADDR
	ZP_READ
	OP_CMP
	CYCLE_NEXT	3

opD5:	@ CMP $nn, X
	ZEROX_ADDR
	ZP_READ
	OP_CMP
	CYCLE_NEXT	4

opCD:	@ CMP $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_CMP
	CYCLE_NEXT	4,1,0

opDD:	@ CMP $nnnn, X
	CYCLE_PRE	4
	ABSX_ADDR
	READ
	OP_CMP
	CYCLE_NEXT	4,1,0

opD9:	@ CMP $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_CMP
	CYCLE_NEXT	4,1,0

opC1:	@ CMP ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	READ
	OP_CMP
	CYCLE_NEXT	6,1,0

opD1:	@ CMP ($nn), Y
	CYCLE_PRE	5
	INDY_ADDR
	READ
	OP_CMP
	CYCLE_NEXT	5,1,0


opE0:	@ CPX #$nn
	IMM_VALUE
	OP_CPX
	CYCLE_NEXT	2

opE4:	@ CPX $nn
	ZERO_ADDR
	ZP_READ
	OP_CPX
	CYCLE_NEXT	3

opEC:	@ CPX $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_CPX
	CYCLE_NEXT	4,1,0


opC0:	@ CPY #$nn
	IMM_VALUE
	OP_CPY
	CYCLE_NEXT	2

opC4:	@ CPY $nn
	ZERO_ADDR
	ZP_READ
	OP_CPY
	CYCLE_NEXT	3

opCC:	@ CPY $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_CPY
	CYCLE_NEXT	4,1,0


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ BIT
@@@
@@@ Z <= A ∧ M == 0
@@@ N <= M[7]
@@@ V <= M[6]
@@@
@@@ NZVC
@@@ ----
@@@ ***-
.macro	OP_BIT
	and	REG_NZ, r0, REG_A, lsr #24
	@@ R0[7] => C, R0[6] => N
	movs	r0, r0, lsl #25
	orrcs	REG_NZ, REG_NZ, #0x80 << 24
	orrmi	REG_P_REST, REG_P_REST, #P_REST_V_FLAG
	bicpl	REG_P_REST, REG_P_REST, #P_REST_V_FLAG
.endm

op24:	@ BIT $nn
	ZERO_ADDR
	ZP_READ
	OP_BIT
	CYCLE_NEXT	3

op2C:	@ BIT $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_BIT
	CYCLE_NEXT	4,1,0


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ ASL/LSR
@@@
@@@ 右シフト・左シフト
@@@
@@@ NZCを変更する
@@@
.macro	OP_ASL
	movs	REG_NZ, r0, lsl #25
	mov	r0, REG_NZ, lsr #24
	mov	REG_NZ, REG_NZ, asr #24
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
.endm

.macro	OP_ASL_A
	movs	REG_A, REG_A, lsl #1
	mov	REG_NZ, REG_A, asr #24
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
.endm

.macro	OP_LSR
	movs	r0, r0, lsr #1
	@@ Nは立たない
	mov	REG_NZ, r0
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
.endm

.macro	OP_LSR_A
	@@ Nは立たない
	movs	REG_NZ, REG_A, lsr #25
	mov	REG_A, REG_NZ, lsl #24
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
.endm

op0A:	@ ASL A
	IMPLIED
	OP_ASL_A
	CYCLE_NEXT	2

op06:	@ ASL $nn
	ZERO_ADDR
	ZP_READ_W
	OP_ASL
	ZP_WRITE_W
	CYCLE_NEXT	5

op16:	@ ASL $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_ASL
	ZP_WRITE_W
	CYCLE_NEXT	6

op0E:	@ ASL $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_ASL
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_ASL
	READ_WRITE_4
	CYCLE_NEXT	6,1,0

op1E:	@ ASL $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_ASL
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_ASL
	READ_WRITE_4
	CYCLE_NEXT	7,1,0


op4A:	@ LSR A
	IMPLIED
	OP_LSR_A
	CYCLE_NEXT	2

op46:	@ LSR $nn
	ZERO_ADDR
	ZP_READ_W
	OP_LSR
	ZP_WRITE_W
	CYCLE_NEXT	5

op56:	@ LSR $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_LSR
	ZP_WRITE_W
	CYCLE_NEXT	6

op4E:	@ LSR $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_LSR
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_LSR
	READ_WRITE_4
	CYCLE_NEXT	6,1,0

op5E:	@ LSR $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_LSR
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_LSR
	READ_WRITE_4
	CYCLE_NEXT	7,1,0


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ ROL/ROR
@@@
@@@ 左回転/右回転
@@@
@@@ NZCを変更する
@@@
.macro	OP_ROL
	@@ キャリーをセット
	movs	r1, REG_P_REST, lsr #1
	@@ シフト
	adc	r0, r0, r0
	@@ NZとキャリーをセット
	orrs	REG_NZ, r0, r0, lsl #24
	@@ キャリーを保存
	adc	REG_P_REST, r1, r1
.endm

.macro	OP_ROL_A
	@@ キャリーをセット
	movs	r1, REG_P_REST, lsr #1
	orrcs	REG_A, REG_A, #0x80 << 16
	movs	REG_A, REG_A, lsl #1
	mov	REG_NZ, REG_A, asr #24
	adc	REG_P_REST, r1, r1
.endm

.macro	OP_ROR
	movs	r1, REG_P_REST, lsr #1
	orrcs	r0, r0, #0x100
	movs	r0, r0, lsr #1
	orr	REG_NZ, r0, r0, lsl #24
	adc	REG_P_REST, r1, r1
.endm

.macro	OP_ROR_A
	movs	r1, REG_P_REST, lsr #1
	mov	REG_NZ, REG_A, rrx
	movs	REG_NZ, REG_NZ, asr #24
	mov	REG_A, REG_NZ, lsl #24
	adc	REG_P_REST, r1, r1
.endm

op2A:	@ ROL A
	IMPLIED
	OP_ROL_A
	CYCLE_NEXT	2

op26:	@ ROL $nn
	ZERO_ADDR
	ZP_READ_W
	OP_ROL
	ZP_WRITE_W
	CYCLE_NEXT	5

op36:	@ ROL $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_ROL
	ZP_WRITE_W
	CYCLE_NEXT	6

op2E:	@ ROL $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_ROL
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_ROL
	READ_WRITE_4
	CYCLE_NEXT	6,1,0


op3E:	@ ROL $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_ROL
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_ROL
	READ_WRITE_4
	CYCLE_NEXT	7,1,0


op6A:	@ ROR A
	IMPLIED
	OP_ROR_A
	CYCLE_NEXT	2

op66:	@ ROR $nn
	ZERO_ADDR
	ZP_READ_W
	OP_ROR
	ZP_WRITE_W
	CYCLE_NEXT	5

op76:	@ ROR $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_ROR
	ZP_WRITE_W
	CYCLE_NEXT	6

op6E:	@ ROR $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_ROR
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_ROR
	READ_WRITE_4
	CYCLE_NEXT	6,1,0

op7E:	@ ROR $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_ROR
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_ROR
	READ_WRITE_4
	CYCLE_NEXT	7,1,0


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ BCC/BCS/BNE/BEQ/BPL/BMI/BVC/BVS
@@@
@@@ 分岐
@@@

.macro HAD_BRANCH
	@@ R0 <- PC
	ldr	r0, [REG_OP_TABLE, #OTOFFS_PC_BASE]
	sub	r0, REG_PC, r0
	mov	r3, r1		@ preserve immediate
	REBASE_PC

	sub	r1,r0,r3
	@@ R1 <- (PC&0xFF)+ReadValue
	and	r1,r1,#0xff
	add	r1,r3,r1
	tst	r1,#0x100
	addne	REG_CYCLE, REG_CYCLE, #1
	subne	REG_CYCLE, REG_CYCLE, #1*48<<16
.endm

.macro	BRANCH_EQ
	ldreqsb	r1, [REG_PC], #1
	movne	r1, #1
	add	REG_PC, REG_PC, r1
	addeq	REG_CYCLE, REG_CYCLE, #1
	subeq	REG_CYCLE, REG_CYCLE, #1*48<<16
	bne	1f
	HAD_BRANCH
1:
.endm

.macro	BRANCH_NE
	ldrnesb	r1, [REG_PC], #1
	moveq	r1, #1
	add	REG_PC, REG_PC, r1
	addne	REG_CYCLE, REG_CYCLE, #1
	subne	REG_CYCLE, REG_CYCLE, #1*48<<16
	beq	1f
	HAD_BRANCH
1:
.endm

op90:	@ BCC $nn
	tst	REG_P_REST, #P_REST_C_FLAG
	@@ Z が立てば
	BRANCH_EQ
	CYCLE_NEXT	2
	.pool

opB0:	@ BCS $nn
	tst	REG_P_REST, #P_REST_C_FLAG
	@@ Z が下りてれば
	BRANCH_NE
	CYCLE_NEXT	2

opD0:	@ BNE $nn
	tst	REG_NZ, #0xFF
	BRANCH_NE
	CYCLE_NEXT	2

opF0:	@ BEQ $nn
	tst	REG_NZ, #0xFF
	BRANCH_EQ
	CYCLE_NEXT	2

op30:	@ BMI $nn
	tst	REG_NZ, #0x80 << 24
	@@ 結果が0でないなら
	BRANCH_NE
	CYCLE_NEXT	2

op10:	@ BPL $nn
	tst	REG_NZ, #0x80 << 24
	@@ 結果が0なら
	BRANCH_EQ
	CYCLE_NEXT	2

op50:	@ BVC $nn
	tst	REG_P_REST, #P_REST_V_FLAG
	@@ 結果が0なら
	BRANCH_EQ
	CYCLE_NEXT	2


op70:	@ BVS $nn
	tst	REG_P_REST, #P_REST_V_FLAG
	@@ 結果0でないなら
	BRANCH_NE
	CYCLE_NEXT	2
	.pool

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@  UNDOC
@@  LAX
@@
.macro	OP_LAX
	mov	REG_A, r0, lsl #24
	mov	REG_X, r0
	mov	REG_NZ, REG_A, asr #24
.endm

.macro  OP_SLO
	movs	r0, r0, lsl #25
 @@set C flag
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG

	orr	REG_A,REG_A,r0
 @@set NZ flag
	mov	REG_NZ, REG_A, asr #24
 @@restore value to write
	mov	r0, r0, lsr #24
.endm

.macro	OP_ISB
	add	r0, r0, #1
	and	r0, r0, #0xFF
.endm

.macro  OP_SRE
	movs	r0, r0, lsr #1
 @@set C flag
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG

	eor	REG_A,REG_A,r0, lsl #24
 @@set NZ flag
	mov	REG_NZ, REG_A, asr #24
.endm

.macro  OP_SBX
	and	r1,REG_X,REG_A, lsr #24
	subs	r0,r1,r0
	orrcs	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	biccc	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	and	REG_X,r0,#0xFF
	orr	REG_NZ, REG_X, REG_X, lsl #24
.endm

.macro  OP_DCP
	sub	r0,r0,#1
	and	r0,r0,#0xFF
.endm

.macro	OP_RLA
	@@ キャリーをセット
	movs	r1, REG_P_REST, lsr #1
	@@ シフト
	adc	r0, r0, r0
	@@ NZとキャリーをセット
	orrs	REG_NZ, r0, r0, lsl #24
	@@ キャリーを保存
	adc	REG_P_REST, r1, r1
	and	REG_A,REG_A,r0,lsl #24
	orr	REG_NZ, REG_A, REG_A, lsr #24		@ according to fceu
.endm


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@
@@  UNDOCUMENTED
@@

op47:	@ SRE $nn
	ZERO_ADDR
	ZP_READ_W
	OP_SRE
	ZP_WRITE_W
	CYCLE_NEXT	5

op57:	@ SRE $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_SRE
	ZP_WRITE_W
	CYCLE_NEXT	6

op4F:	@ SRE $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_SRE
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_SRE
	READ_WRITE_4
	CYCLE_NEXT	6,1,0

op5F:	@ SRE $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_SRE
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_SRE
	READ_WRITE_4
	CYCLE_NEXT	7,1,0

op5B:	@ SRE $nnnn, Y
	CYCLE_PRE	7
	ABSY_ADDR_W
	READ_WRITE_1
	OP_SRE
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_SRE
	READ_WRITE_4
	CYCLE_NEXT	7,1,0

op43:	@ SRE ($nn, X)
	CYCLE_PRE	8
	INDX_ADDR
	READ_WRITE_1
	OP_SRE
	READ_WRITE_2
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_SRE
	READ_WRITE_4
	CYCLE_NEXT	8,1,0

op53:	@ SRE ($nn), Y
	CYCLE_PRE	8
	INDY_ADDR_W
	READ_WRITE_1
	OP_SRE
	READ_WRITE_2
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_SRE
	READ_WRITE_4
	CYCLE_NEXT	8,1,0


opE7:	@ ISB $nn
	ZERO_ADDR
	ZP_READ_W
	OP_ISB
	ZP_WRITE_W
	OP_SBC
	CYCLE_NEXT	5

opF7:	@ ISB $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_ISB
	ZP_WRITE_W
	OP_SBC
	CYCLE_NEXT	6

opEF:   @ ISB $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_ISB
	READ_WRITE_2
	OP_SBC
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_ISB
	READ_WRITE_4
	OP_SBC
	CYCLE_NEXT	6,1,0


opFF:   @ ISB $nnnn,X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_ISB
	READ_WRITE_2
	OP_SBC
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_ISB
	READ_WRITE_4
	OP_SBC
	CYCLE_NEXT	7,1,0

opFB:	@ ISB $nnnn, Y
	CYCLE_PRE	7
	ABSY_ADDR_W
	READ_WRITE_1
	OP_ISB
	READ_WRITE_2
	OP_SBC
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_ISB
	READ_WRITE_4
	OP_SBC
	CYCLE_NEXT	7,1,0

opE3:	@ ISB ($nn, X)
	CYCLE_PRE	8
	INDX_ADDR
	READ_WRITE_1
	OP_ISB
	READ_WRITE_2
	OP_SBC
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_ISB
	READ_WRITE_4
	OP_SBC
	CYCLE_NEXT	8,1,0

opF3:	@ ISB ($nn), Y
	CYCLE_PRE	8
	INDY_ADDR
	READ_WRITE_1
	OP_ISB
	READ_WRITE_2
	OP_SBC
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_ISB
	READ_WRITE_4
	OP_SBC
	CYCLE_NEXT	8,1,0

opA7:	@ LAX $nn
	ZERO_ADDR
	ZP_READ
	OP_LAX
	CYCLE_NEXT	3

opB7:	@ LAX $nn, Y
	ZEROY_ADDR
	ZP_READ
	OP_LAX
	CYCLE_NEXT	4

opAF:	@ LAX $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	READ
	OP_LAX
	CYCLE_NEXT	4,1,0

opBF:	@ LAX $nnnn, Y
	CYCLE_PRE	4
	ABSY_ADDR
	READ
	OP_LAX
	CYCLE_NEXT	4,1,0

opA3:	@ LAX ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	READ
	OP_LAX
	CYCLE_NEXT	6,1,0

opB3:	@ LAX ($nn), Y
	CYCLE_PRE	5
	INDY_ADDR
	READ
	OP_LAX
	CYCLE_NEXT	5,1,0

op07:	@ SLO $nn
	CYCLE_PRE	5
	ZERO_ADDR
	ZP_READ_W
	OP_SLO
	ZP_WRITE_W
	CYCLE_NEXT	5,1,0

op17:	@ SLO $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_SLO
	ZP_WRITE_W
	CYCLE_NEXT	6

op0F:	@ SLO $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_SLO
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_SLO
	READ_WRITE_4
	CYCLE_NEXT	6,1,0

op1F:	@ SLO $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_SLO
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_SLO
	READ_WRITE_4
	CYCLE_NEXT	7,1,0

op1B:	@ SLO $nnnn, Y
	CYCLE_PRE	7
	ABSY_ADDR_W
	READ_WRITE_1
	OP_SLO
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_SLO
	READ_WRITE_4
	CYCLE_NEXT	7,1,0

op03:	@ SLO ($nn, X)
	CYCLE_PRE	8
	INDX_ADDR
	READ_WRITE_1
	OP_SLO
	READ_WRITE_2
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_SLO
	READ_WRITE_4
	CYCLE_NEXT	8,1,0

op13:	@ SLO ($nn), Y
	CYCLE_PRE	8
	INDY_ADDR_W
	READ_WRITE_1
	OP_SLO
	READ_WRITE_2
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_SLO
	READ_WRITE_4
	CYCLE_NEXT	8,1,0

opCB:	@ SBX #$nn
	IMM_VALUE
	OP_SBX
	CYCLE_NEXT	2

opC7:	@ DCP $nn
	ZERO_ADDR
	ZP_READ_W
	OP_DCP
	ZP_WRITE_W
	OP_CMP
	CYCLE_NEXT	5

opD7:	@ DCP $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_DCP
	ZP_WRITE_W
	OP_CMP
	CYCLE_NEXT	6

opCF:	@ DCP $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_DCP
	READ_WRITE_2
	OP_CMP
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_DCP
	READ_WRITE_4
	OP_CMP
	CYCLE_NEXT	6,1,0

opDF:	@ DCP $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_DCP
	READ_WRITE_2
	OP_CMP
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_DCP
	READ_WRITE_4
	OP_CMP
	CYCLE_NEXT	7,1,0

opDB:	@ DCP $nnnn, Y
	CYCLE_PRE	7
	ABSY_ADDR_W
	READ_WRITE_1
	OP_DCP
	READ_WRITE_2
	OP_CMP
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_DCP
	READ_WRITE_4
	OP_CMP
	CYCLE_NEXT	7,1,0


opC3:	@ DCP ($nn, X)
	CYCLE_PRE	8
	INDX_ADDR
	READ_WRITE_1
	OP_DCP
	READ_WRITE_2
	OP_CMP
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_DCP
	READ_WRITE_4
	OP_CMP
	CYCLE_NEXT	8,1,0

opD3:	@ DCP ($nn), Y
	CYCLE_PRE	8
	INDY_ADDR_W
	READ_WRITE_1
	OP_DCP
	READ_WRITE_2
	OP_CMP
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_DCP
	READ_WRITE_4
	OP_CMP
	CYCLE_NEXT	8,1,0

op27:	@ RLA $nn
	ZERO_ADDR
	ZP_READ_W
	OP_RLA
	ZP_WRITE_W
	CYCLE_NEXT	5

op37:	@ RLA $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_RLA
	ZP_WRITE_W
	CYCLE_NEXT	6

op2F:	@ RLA $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_RLA
	READ_WRITE_2
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_RLA
	READ_WRITE_4
	CYCLE_NEXT	6,1,0

op3F:	@ RLA $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_RLA
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_RLA
	READ_WRITE_4
	CYCLE_NEXT	7,1,0

op3B:	@ RLA $nnnn, Y
	CYCLE_PRE	7
	ABSY_ADDR_W
	READ_WRITE_1
	OP_RLA
	READ_WRITE_2
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_RLA
	READ_WRITE_4
	CYCLE_NEXT	7,1,0

op23:	@ RLA ($nn, X)
	CYCLE_PRE	8
	INDX_ADDR
	READ_WRITE_1
	OP_RLA
	READ_WRITE_2
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_RLA
	READ_WRITE_4
	CYCLE_NEXT	8,1,0

op33:	@ RLA ($nn), Y
	CYCLE_PRE	8
	INDY_ADDR_W
	READ_WRITE_1
	OP_RLA
	READ_WRITE_2
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_RLA
	READ_WRITE_4
	CYCLE_NEXT	8,1,0

op67:	@ RRA $nn
	ZERO_ADDR
	ZP_READ_W
	OP_ROR
	ZP_WRITE_W
	OP_ADC
	CYCLE_NEXT	5

op77:	@ RRA $nn, X
	ZEROX_ADDR
	ZP_READ_W
	OP_ROR
	ZP_WRITE_W
	OP_ADC
	CYCLE_NEXT	6

op6F:	@ RRA $nnnn
	CYCLE_PRE	6
	ABS_ADDR
	READ_WRITE_1
	OP_ROR
	READ_WRITE_2
	OP_ADC
	CYCLE_NEXT	6,1,0
	READ_WRITE_3
	OP_ROR
	READ_WRITE_4
	OP_ADC
	CYCLE_NEXT	6,1,0

op7F:	@ RRA $nnnn, X
	CYCLE_PRE	7
	ABSX_ADDR_W
	READ_WRITE_1
	OP_ROR
	READ_WRITE_2
	OP_ADC
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_ROR
	READ_WRITE_4
	OP_ADC
	CYCLE_NEXT	7,1,0

op7B:	@ RRA $nnnn, Y
	CYCLE_PRE	7
	ABSY_ADDR_W
	READ_WRITE_1
	OP_ROR
	READ_WRITE_2
	OP_ADC
	CYCLE_NEXT	7,1,0
	READ_WRITE_3
	OP_ROR
	READ_WRITE_4
	OP_ADC
	CYCLE_NEXT	7,1,0

op63:	@ RRA ($nn, X)
	CYCLE_PRE	8
	INDX_ADDR
	READ_WRITE_1
	OP_ROR
	READ_WRITE_2
	OP_ADC
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_ROR
	READ_WRITE_4
	OP_ADC
	CYCLE_NEXT	8,1,0

op73:	@ RRA ($nn), Y
	CYCLE_PRE	8
	INDY_ADDR_W
	READ_WRITE_1
	OP_ROR
	READ_WRITE_2
	OP_ADC
	CYCLE_NEXT	8,1,0
	READ_WRITE_3
	OP_ROR
	READ_WRITE_4
	OP_ADC
	CYCLE_NEXT	8,1,0


op04:	@ NOP $nn
op44:
op64:
	add     REG_PC,REG_PC,#1
	CYCLE_NEXT  3

op0C:	@ NOP $nnnn
	CYCLE_PRE	4
#if DO_DUMMY_READS
	ABS_ADDR
	READ
#else
	add     REG_PC,REG_PC,#2
#endif
	CYCLE_NEXT	4,1,0

op14:	@ NOP $nn, X
op34:
op54:
op74:
opD4:
opF4:
	add     REG_PC,REG_PC,#1
	CYCLE_NEXT  4

op1A:	@ NOP
op3A:
op5A:
op7A:
opDA:
opFA:
       CYCLE_NEXT   2

op1C:	@ NOP $nnnn, X
op3C:
op5C:
op7C:
opDC:
opFC:
	CYCLE_PRE	4
#if DO_DUMMY_READS
	ABSX_ADDR
	READ
#else
	add     REG_PC,REG_PC,#2
#endif
	CYCLE_NEXT	4,1,0

op80:	@ NOP #$nn
op82:
op89:
opC2:
	add     REG_PC,REG_PC,#1
	CYCLE_NEXT   2

opE2:
	add     REG_PC,REG_PC,#1
	CYCLE_NEXT   3

op02:	@ JAM: try to be compatible with fceu
op12:
op22:
op32:
op42:
op52:
op62:
op72:
op92:
opB2:
opD2:
opF2:
	sub     REG_PC,REG_PC,#1
	add	REG_CYCLE, REG_CYCLE, #0x10
	FLUSH_TIMESTAMP r0
	add	REG_CYCLE, REG_CYCLE, #0xef
	sub	REG_CYCLE, REG_CYCLE, #0x00d00000 @ 0xff*48<<16
	sub	REG_CYCLE, REG_CYCLE, #0x2f000000
	CYCLE_NEXT   2

op0B:	@ ANC #$nn
op2B:	@ ANC #$nn
	IMM_VALUE
	OP_AND
	bic	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	orr	REG_P_REST, REG_P_REST, REG_A, lsr #31
	CYCLE_NEXT   2

op4B:	@ ASR #$nn
	IMM_VALUE
	OP_AND
	OP_LSR_A
	CYCLE_NEXT   2

op6B:	@ ARR #$nn
	IMM_VALUE
	OP_AND
	bic	REG_P_REST, REG_P_REST, #P_REST_C_FLAG
	eor	r0, REG_A, REG_A, lsr #1
	and	r0, r0, #0x40000000
	orr	REG_P_REST, REG_P_REST, r0, lsr #24
	OP_ROR_A
	CYCLE_NEXT   2

op83:	@ SAX/AAX ($nn, X)
	CYCLE_PRE	6
	INDX_ADDR
	and	r0, REG_X, REG_A, lsr #24
	WRITE_1
	CYCLE_NEXT	6,1,0

op87:	@ SAX/AAX $nn
	ZERO_ADDR
	and	r0, REG_X, REG_A, lsr #24
	ZP_WRITE
	CYCLE_NEXT	3

op8B:	@ ANE/XAA #$nn
	orr	REG_A, REG_A, #0xee000000
	and	REG_A, REG_A, REG_X, lsl #24
	IMM_VALUE
	OP_AND
	CYCLE_NEXT	2

op8F:	@ SAX/AAX $nnnn
	CYCLE_PRE	4
	ABS_ADDR
	and	r0, REG_X, REG_A, lsr #24
	WRITE_1
	CYCLE_NEXT	4,1,0

.macro	AMY_MSB
	sub	r0, REG_ADDR, REG_Y
	mov	r0, r0, lsr #8
	add	r0, r0, #1
.endm

op93:	@ SHA/AXA ($nn), Y
	CYCLE_PRE	6
	INDY_ADDR_W
	AMY_MSB
	and	r0, r0, REG_X
	and	r0, r0, REG_A, lsr #24
	WRITE_1
	CYCLE_NEXT	6,1,0

op97:	@ SAX,AAX $nn, Y
	ZEROY_ADDR
	and	r0, REG_X, REG_A, lsr #24
	ZP_WRITE
	CYCLE_NEXT	4

op9B:	@ SHS/XAS $nnnn, Y
	and	r0, REG_X, REG_A, lsr #24
	bic	REG_S, REG_S, #0xFF << 24
	orr	REG_S, REG_S, r0, lsl #24
	ABSY_ADDR_W
	AMY_MSB
	and	r0, r0, REG_S, lsr #24
	WRITE_1
	CYCLE_NEXT	5

op9C:	@ SHY/SYA $nnnn, X
	CYCLE_PRE	5
	ABSX_ADDR_W
	sub	r0, REG_ADDR, REG_X
	mov	r0, r0, lsr #8
	add	r0, r0, #1
	and	r0, r0, REG_Y
	WRITE_1
	CYCLE_NEXT	5,1,0

op9E:	@ SHX/SXA $nnnn, Y
	ABSY_ADDR_W
	AMY_MSB
	and	r0, r0, REG_X
	WRITE_1
	CYCLE_NEXT	5

op9F:	@ SHA/AXA $nnnn, Y
	CYCLE_PRE	5
	ABSY_ADDR_W
	AMY_MSB
	and	r0, r0, REG_X
	and	r0, r0, REG_A, lsr #24
	WRITE_1
	CYCLE_NEXT	5,1,0

opAB:	@ LXA/ATX/OAL #$nn
	IMM_VALUE
	orr	REG_A, REG_A, #0xee000000
	OP_AND
	mov	REG_X, REG_A, lsr #24
	CYCLE_NEXT	2

opBB:	@ LAS/LAR $nnnn,Y
	CYCLE_PRE	4
	ABSY_ADDR_W
	READ_WRITE_1
	READ_WRITE_2
	b	opBB_
	READ_WRITE_3
	READ_WRITE_4
opBB_:	and	REG_X, r0, REG_S, lsr #24
	bic	REG_S, REG_S, #0xff<<24
	orr	REG_S, REG_S, REG_X, lsl #24
	mov	REG_A, REG_X, lsl #24
	orr	REG_NZ, REG_A, REG_A, lsr #24
	CYCLE_NEXT	4,1,0




@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ JMP_INDIRECT
@@@
@@@ NZVC
@@@ ----
@@@ ----
@ JMP ($nnnn)
op6C:
	CYCLE_PRE	5
	ABS_ADDR
	and	r0, REG_ADDR, #0xFF
	teq	r0, #0xFF
	beq	jmp_indirect_bug
	READ_WORD
	REBASE_PC
	CYCLE_NEXT	5,1,0
jmp_indirect_bug:
@@
@@ BUG is : to not read word at REG_ADDR, because it loops
@@ but read low part at REG_ADDR and high part at REG_ADDR&0xFF00 instead of REG_ADDR+1
@@
	READ	0
	mov	REG_PC, r0
	and	REG_ADDR, REG_ADDR, #0xff00
	READ	0
	orr	r0, REG_PC, r0, lsl #8
        REBASE_PC

	CYCLE_NEXT	5,1,0


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ JMP_ABSOLUTE
@@@
@@@ NZVC
@@@ ----
@@@ ----
@ JMP $nnnn
op4C:
	ABS_ADDR
	mov	r0, REG_ADDR
	@sub	r3, REG_PC, #3
	REBASE_PC
	CYCLE_NEXT	3

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ JSR
@@@
@@@ NZVC
@@@ ----
@@@ ----
@ JSR $nnnn
op20:
	ABS_ADDR
	ldr	r0, [REG_OP_TABLE, #OTOFFS_PC_BASE]
	sub	r0, REG_PC, r0
	sub	r0, r0, #1
	PUSH_WORD
	mov	r0, REG_ADDR
	REBASE_PC
	CYCLE_NEXT	6



@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ BRK
@@@
@@@ NZVC BI
@@@ ---- --
@@@ ---- 11
op00:	@ BRK
	IMPLIED
	add	REG_PC, REG_PC, #1
	ldr	r0, [REG_OP_TABLE, #OTOFFS_PC_BASE]
	sub	r0, REG_PC, r0
	PUSH_WORD
	orr	REG_P_REST, REG_P_REST, #P_REST_B_FLAG
	SAVE_P
	PUSH_BYTE
	orr	REG_P_REST, REG_P_REST, #P_REST_I_FLAG
	mov	REG_ADDR, #0x10000
	sub	REG_ADDR, REG_ADDR, #IRQ_VECTOR
	READ_WORD
	REBASE_PC
	CYCLE_NEXT	7

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ RTI
@@@
@@@ NZVC
@@@ ----
@@@ ----
@ RTI
op40:
	IMPLIED
	POP_BYTE
	LOAD_P
	POP_WORD
	REBASE_PC
	CYCLE_NEXT_INT	6

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ RTS
@@@
@@@ NZVC
@@@ ----
@@@ ----
@ RTS
op60:
	IMPLIED
	POP_WORD
	add	r0, r0, #1
	REBASE_PC
	CYCLE_NEXT	6

@@@
@@@ 割り込みの処理
@@@ WARNING: decrements REG_PC
@@@
do_int:
	add	REG_CYCLE, REG_CYCLE, #7
	ldr	r0, [REG_OP_TABLE, #OTOFFS_PC_BASE]
	sub	REG_PC, REG_PC, #1
	sub	r0, REG_PC, r0
	PUSH_WORD
	bic	REG_P_REST, REG_P_REST, #P_REST_B_FLAG
	SAVE_P
	PUSH_BYTE
	tst	REG_P_REST, #FCEU_IQNMI<<8
	orreq	REG_P_REST, REG_P_REST, #P_REST_I_FLAG
	bic     REG_P_REST, REG_P_REST, #((FCEU_IQNMI|FCEU_IQTEMP)<<8)
	mov	REG_ADDR, #0x10000
	subeq	REG_ADDR, REG_ADDR, #IRQ_VECTOR
	subne	REG_ADDR, REG_ADDR, #NMI_VECTOR
	READ_WORD
	REBASE_PC
@	CYCLE_NEXT	7

	subs	REG_CYCLE, REG_CYCLE, #7*48<<16
	ble	cpu_exec_end
	ldrb	r0, [REG_PC], #1
	tst	REG_P_REST, #0xff<<8
	ldreq	pc, [REG_OP_TABLE, r0, lsl #2]

	tst	REG_P_REST, #P_REST_I_FLAG
	ldrne	pc, [REG_OP_TABLE, r0, lsl #2]
	b	do_int


@@@
@@@ リセットの処理
@@@
	.globl	reset_cpu

reset_cpu:
	stmfd   r13!,{r4-r11,lr}

	ldr     r0, =nes_registers
	ldmia	r0, {r4-r12}

	@@  Set Z, clear N
	mov	REG_NZ, #0
	@@REG_P_REST = 0, don't touch REG_S
	bic	REG_P_REST, REG_P_REST, #0xff

	@ fceu: set MapIRQHook present flag
	ldr	r0, [REG_OP_TABLE, #OTOFFS_IRQ_HOOK]
	tst	r0, r0
	orrne	REG_P_REST, REG_P_REST, #1<<16
	biceq	REG_P_REST, REG_P_REST, #1<<16

	@@ R bit is always 1
	orr	REG_NZ, REG_NZ, #P_R_FLAG

	@@ INTERRUPT
	orr	REG_P_REST, REG_P_REST, #P_REST_I_FLAG

	mov	REG_ADDR, #0x10000
	sub	REG_ADDR, REG_ADDR, #RESET_VECTOR
	READ_WORD
	REBASE_PC

	ldr     r0, =nes_registers
	stmia	r0, {r4-r12}

	ldmfd   r13!,{r4-r11,lr}
	bx      lr

@@@
@@@ low-level memhandlers
@@@

/*
@ disabled because no improvements noticed, only causes trouble (with gg for example)
read_rom_byte:
@ try to avoid lookup of every address at least for ROM and RAM areas
@ I've verified that if ARead[0xfff0] points to CartBR, it is always normal ROM read.
	ldr	r0, =CartBR
	ldr	r2, =ARead
	mov	r1, #0xff00
	orr	r1, r1, r1, lsr #4
	ldr	r1, [r2, r1, lsl #2]		@ if (ARead[0xfff0] == CartBR)
	cmp	r0, r1
	bne	read_byte
	ldr	r2, =Page
	mov	r1, REG_ADDR, lsr #11
	ldr	r2, [r2, r1, lsl #2]
	ldrb	r0, [r2, REG_ADDR]
	bx	lr
*/


read_byte:
	@ must preserve r3 for the callers too
	@ TODO: check if all of saves are needed, _DB (is full needed?)
	FLUSH_TIMESTAMP r2			@ needed for TryFixit1
	str     REG_PC,     [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x0c)]	@ might get rebased
	str     REG_P_REST, [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x10)]	@ might set irq
	str     REG_CYCLE,  [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x1c)]	@ might get used
	mov     REG_PC, lr			@ r7
	mov	REG_P_REST, r3			@ r8

#ifndef DEBUG_ASM_6502
	ldr	r2, =ARead
	bic     r0, REG_ADDR, #0x00ff0000
	mov	lr, pc
	ldr	pc, [r2, r0, lsl #2]
#else
	ldr	r2, =dread_count_a
	ldr	r0, =dreads
	ldr	r1, [r2]
	ldrb	r0, [r0, r1]
	add	r1, r1, #1
	str	r1, [r2]
#endif

	ldr	REG_OP_TABLE, =cpu_exec_table	@ got trashed because was in r12
	mov	lr, REG_PC
	mov	r3, REG_P_REST
	ldr     REG_PC,     [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x0c)]	@ might get rebased
	ldr     REG_P_REST, [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x10)]	@ might set irq
	ldr     REG_CYCLE,  [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x1c)]	@ might get used
	strb    r0,         [REG_OP_TABLE, #(OTOFFS_X + 0x10)]		@ X.DB
	bx	lr


write_byte:
	FLUSH_TIMESTAMP r2			@ Blaster Master, more...
#ifndef DEBUG_ASM_6502
	@ must preserve r0 (data) and r3 for the callers
	str     REG_PC,     [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x0c)]	@ might get rebased
	str     REG_P_REST, [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x10)]	@ might set irq
	str     REG_CYCLE,  [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x1c)]	@ might get used
	mov     REG_PC, lr			@ r7
	mov	REG_P_REST, r3			@ r8
	mov	REG_CYCLE, r0			@ r11

	ldr	r2, =BWrite
	mov	r1, r0
	bic     r0, REG_ADDR, #0x00ff0000
	mov	lr, pc
	ldr	pc, [r2, r0, lsl #2]

	ldr	REG_OP_TABLE, =cpu_exec_table	@ got trashed because was in r12
	mov	lr, REG_PC
	mov	r3, REG_P_REST
	mov	r0, REG_CYCLE
	ldr     REG_PC,     [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x0c)]	@ might get rebased
	ldr     REG_P_REST, [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x10)]	@ might set irq
	ldr     REG_CYCLE,  [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x1c)]	@ might get used
#else
	ldr	r1, =dwrite_count_a
	ldr	r2, =dwrites_a
	ldr	r1, [r1]
	and	r0, r0, #0xff
	orr	r0, r0, REG_ADDR, lsl #8
	str	r0, [r2, r1, lsl #2]
	ldr	r2, =dwrite_count_a
	add	r1, r1, #1
	str	r1, [r2]
	and	r0, r0, #0xff
#endif
	bx	lr


.pool



@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@@@
@@@ 6502 をエミュレートする
@@@
@@@ REG_CYCLE	=> 実行するサイクル
@@@ REG_CYCLE	<= 残っているサイクル(零または負)
	.globl	cpu_exec
cpu_exec:
	stmfd   r13!,{r4-r11,lr}

	ldr     r0, =nes_registers
	ldmia	r0, {r4-r12}

@	ldr   REG_OP_TABLE, = cpu_exec_table @ set on init

	ldrb	r0, [REG_PC], #1
	tst	REG_P_REST, #0xff<<8
	ldreq	pc, [REG_OP_TABLE, r0, lsl #2]

	@ assumption: NMI can be set only on very first run, because it is only set once before vblank..
	tst	REG_P_REST, #FCEU_IQNMI<<8
	bne	do_int
	tst	REG_P_REST, #P_REST_I_FLAG
	@@ if I_FLAG=1, continue execution, don't trigger IRQ
	bicne	REG_P_REST, REG_P_REST, #FCEU_IQTEMP<<8
	ldrne	pc, [REG_OP_TABLE, r0, lsl #2]
	@@ I_FLAG=0 and REST is checked, we have a IRQ
	b	do_int


cpu_exec_end:
	FLUSH_TIMESTAMP r0

	tst	REG_P_REST, #1<<16
	bne	do_irq_hook_final

	ldr     r0, =nes_registers
	stmia	r0, {r4-r12}

	ldmfd   r13!,{r4-r11,lr}
	bx      lr

@@@
@@@ みればわかるようにオペコードをインデックスにしたジャンプテーブル
@@@ 0x400バイト
@@@

@ the code does RAM accesses through cpu_exec_table pointer + offset, so
@ they must be in the same section

	SECTION_DATA
	ALIGN

cpu_exec_table:
	.long	op00, op01, op02, op03, op04, op05, op06, op07
	.long	op08, op09, op0A, op0B, op0C, op0D, op0E, op0F

	.long	op10, op11, op12, op13, op14, op15, op16, op17
	.long	op18, op19, op1A, op1B, op1C, op1D, op1E, op1F

	.long	op20, op21, op22, op23, op24, op25, op26, op27
	.long	op28, op29, op2A, op2B, op2C, op2D, op2E, op2F

	.long	op30, op31, op32, op33, op34, op35, op36, op37
	.long	op38, op39, op3A, op3B, op3C, op3D, op3E, op3F

	.long	op40, op41, op42, op43, op44, op45, op46, op47
	.long	op48, op49, op4A, op4B, op4C, op4D, op4E, op4F

	.long	op50, op51, op52, op53, op54, op55, op56, op57
	.long	op58, op59, op5A, op5B, op5C, op5D, op5E, op5F

	.long	op60, op61, op62, op63, op64, op65, op66, op67
	.long	op68, op69, op6A, op6B, op6C, op6D, op6E, op6F

	.long	op70, op71, op72, op73, op74, op75, op76, op77
	.long	op78, op79, op7A, op7B, op7C, op7D, op7E, op7F

	.long	op80, op81, op82, op83, op84, op85, op86, op87
	.long	op88, op89, op8A, op8B, op8C, op8D, op8E, op8F

	.long	op90, op91, op92, op93, op94, op95, op96, op97
	.long	op98, op99, op9A, op9B, op9C, op9D, op9E, op9F

	.long	opA0, opA1, opA2, opA3, opA4, opA5, opA6, opA7
	.long	opA8, opA9, opAA, opAB, opAC, opAD, opAE, opAF

	.long	opB0, opB1, opB2, opB3, opB4, opB5, opB6, opB7
	.long	opB8, opB9, opBA, opBB, opBC, opBD, opBE, opBF

	.long	opC0, opC1, opC2, opC3, opC4, opC5, opC6, opC7
	.long	opC8, opC9, opCA, opCB, opCC, opCD, opCE, opCF

	.long	opD0, opD1, opD2, opD3, opD4, opD5, opD6, opD7
	.long	opD8, opD9, opDA, opDB, opDC, opDD, opDE, opDF

	.long	opE0, opE1, opE2, opE3, opE4, opE5, opE6, opE7
	.long	opE8, opE9, opEA, opEB, opEC, opED, opEE, opEF

	.long	opF0, opF1, opF2, opF3, opF4, opF5, opF6, opF7
	.long	opF8, opF9, opFA, opFB, opFC, opFD, opFE, opFF

@@@
@@@ オペコードテーブルの直後にこれを置くREG_OP_TABLE相対で
@@@ 利用できる
@@@
nes_registers:
	.fill   0x40, 1, 0
RAM:
nes_internal_ram:
	.fill	0x100, 1, 0
nes_stack:
	.fill	0x700, 1, 0
pc_base:
	.long	0
MapIRQHook:
	.long	0
timestamp:
timestamp_a:
	.long	0
#ifndef DEBUG_ASM_6502
X:
#endif
X_:	.fill   0x20, 1, 0

.pool

@	SECTION_SLOW
	SECTION_TEXT
	ALIGN



@@@@@@@@@@@@@@@@@@@@@@@@@@
@ fceu stuff...
@@@@@@@@@@@@@@@@@@@@@@@@@@

@ for reference
.extern Page
.extern ARead
.extern BWrite
.extern MapIRQHook


	SECTION_DATA
	ALIGN
	.globl	nes_registers @ TODO: hide?
	.globl	pc_base
	.globl	MapIRQHook @ (int a)
#ifndef DEBUG_ASM_6502
	.globl	X
	.globl	RAM
	.globl	timestamp
#else
	.globl	X_
	.globl	nes_internal_ram
	.globl	timestamp_a
#endif
	.globl	X6502_Reset_a @ (void);
	.globl	X6502_Power_a @ (void);
	.globl	X6502_Run_a @ (int32 cycles);
	.globl	TriggerIRQ_a @ (void);
	.globl	TriggerNMI_a @ (void);
	.globl	X6502_AddCycles_a @ (int x);
	.globl	X6502_IRQBegin_a @ (int w);
	.globl	X6502_IRQEnd_a @ (int w);
	.globl	X6502_Rebase_a @ (void);

	SECTION_TEXT
	ALIGN

.equiv X6502_Reset_a, reset_cpu
.equiv X6502_Run_a, cpu_exec


X6502_Power_a:
	ldr	r1, =timestamp
	mov	r0, #0
	str	r0, [r1]
	ldr	r1, =nes_registers
	mov     r2, #(0x40/4)
X6502_Power_loop1:
	subs	r2, r2, #1
	str	r0, [r1], #4
	bne	X6502_Power_loop1
	ldr	r0, =cpu_exec_table
	sub	r1, r1, #0x40
	str	r0, [r1, #0x20]
	b	X6502_Reset_a


TriggerIRQ_a:
	mov     r0, #FCEU_IQTEMP

X6502_IRQBegin_a:
	ldr	r2, =nes_registers
	and	r0, r0, #0xff
	ldr     r1, [r2, #0x10] @ REG_P_REST
	orr     r1, r1, r0, lsl #8
	str	r1, [r2, #0x10]
	bx	lr


X6502_IRQEnd_a:
	ldr	r2, =nes_registers
	and	r0, r0, #0xff
	ldr     r1, [r2, #0x10]
	bic     r1, r1, r0, lsl #8
	str	r1, [r2, #0x10]
	bx	lr


TriggerNMI_a:
	mov     r0, #FCEU_IQNMI
	b	X6502_IRQBegin_a


X6502_AddCycles_a:
	ldr	r3, =timestamp
	ldr	r2, =nes_registers
	ldr	r1, [r3]
	add	r1, r1, r0
	str	r1, [r3]
	ldrsh	r1, [r2, #0x1e]
	mvn	r3, #47			@ r3=-48
	mla	r3, r0, r3, r1
	ldr	r1, =MapIRQHook		@ hack..
	strh	r3, [r2, #0x1e]
	ldr	r1, [r1]
	tst	r1, r1
	bxeq	lr
	bx	r1


@ rebase PC when not executing or in memhandlers
X6502_Rebase_a:
	stmfd	sp!,{REG_PC,REG_OP_TABLE}
	ldr	REG_OP_TABLE, =cpu_exec_table
	ldr	r0, [REG_OP_TABLE, #(OTOFFS_NES_REGS+0x0c)] @ PC
	ldr	r1, [REG_OP_TABLE, #OTOFFS_PC_BASE]
	sub	r0, r0, r1
	REBASE_PC
	str	REG_PC, [REG_OP_TABLE, #(OTOFFS_NES_REGS+0x0c)] @ PC
	ldmfd	sp!,{REG_PC,REG_OP_TABLE}
	bx	lr
.pool


@ the nasty MapIRQHook thing from FCE..
@ test Gradius 2 (J) if you change this
do_irq_hook:
	FLUSH_TIMESTAMP r0

	@ get irqhook cycles
	and	r0, REG_CYCLE, #0xff00
	mov	r0, r0, lsr #8
#ifndef DEBUG_ASM_6502
	@ I have reviewed all MapIRQHook functions, they only seem to cause IRQs, not messing cycles or something
	str     REG_P_REST, [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x10)]	@ might set irq
	mov	REG_P_REST, REG_OP_TABLE				@ r8

	@ if somebody modifies MapIRQHook without calling reset, we are doomed
	mov	lr, pc
	ldr	pc, [REG_OP_TABLE, #OTOFFS_IRQ_HOOK]

	mov	REG_OP_TABLE, REG_P_REST				@ got trashed because was in r12
	ldr     REG_P_REST, [REG_OP_TABLE, #(OTOFFS_NES_REGS + 0x10)]	@ might set irq
#else
	ldr	r1, =mapirq_cyc_a
	str	r0, [r1]
	mov	r1, r0
#endif

	ldrb	r0, [REG_PC], #1
	bic	REG_CYCLE, REG_CYCLE, #0xff00
	tst	REG_P_REST, #0xff<<8
	ldreq	pc, [REG_OP_TABLE, r0, lsl #2]

	@ do some messing to find out which IRQ is pending..
	tst	REG_P_REST, #P_REST_I_FLAG
	@@ if I_FLAG=1, continue execution, don't trigger IRQ
	bicne	REG_P_REST, REG_P_REST, #FCEU_IQTEMP<<8
	ldrne	pc, [REG_OP_TABLE, r0, lsl #2]
	@@ I_FLAG=0 and REST is checked, we have a IRQ
	b	do_int


do_irq_hook_final:
	ldr     r1, =nes_registers

	@ get irqhook cycles
	and	r0, REG_CYCLE, #0xff00
	bic	REG_CYCLE, REG_CYCLE, #0xff00
	mov	r0, r0, lsr #8

	stmia	r1, {r4-r12}

	ldmfd   r13!,{r4-r11,lr}

#ifndef DEBUG_ASM_6502
	ldr	pc, [REG_OP_TABLE, #OTOFFS_IRQ_HOOK]
#else
	ldr	r1, =mapirq_cyc_a
	str	r0, [r1]
	mov	r1, r0
	bx	lr
#endif


@ vim:filetype=armasm

