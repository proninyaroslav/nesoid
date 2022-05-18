@ vim:filetype=armasm

@ Assembly optimized routines for gpfce - FCE Ultra port 
@ (c) Copyright 2007, Grazvydas "notaz" Ignotas

@ test
.global flushcache @ beginning_addr, end_addr, flags

flushcache:
    swi #0x9f0002
    mov pc, lr


.global block_or @ void *src, size_t n, int pat

block_or:
    stmfd   sp!, {r4-r5}
    orr     r2, r2, r2, lsl #8
    orr     r2, r2, r2, lsl #16
    mov     r1, r1, lsr #4
block_loop_or:
    ldmia   r0, {r3-r5,r12}
    subs    r1, r1, #1
    orr     r3, r3, r2
    orr     r4, r4, r2
    orr     r5, r5, r2
    orr     r12,r12,r2
    stmia   r0!, {r3-r5,r12}
    bne     block_loop_or
    ldmfd   sp!, {r4-r5}
    bx      lr


.global block_and @ void *src, size_t n, int andpat

block_and:
    stmfd   sp!, {r4-r5}
    orr     r2, r2, r2, lsl #8
    orr     r2, r2, r2, lsl #16
    mov     r1, r1, lsr #4
block_loop_and:
    ldmia   r0, {r3-r5,r12}
    subs    r1, r1, #1
    and     r3, r3, r2
    and     r4, r4, r2
    and     r5, r5, r2
    and     r12,r12,r2
    stmia   r0!, {r3-r5,r12}
    bne     block_loop_and
    ldmfd   sp!, {r4-r5}
    bx      lr


.global block_andor @ void *src, size_t n, int andpat, int orpat

block_andor:
    stmfd   sp!, {r4-r6}
    orr     r2, r2, r2, lsl #8
    orr     r2, r2, r2, lsl #16
    orr     r3, r3, r3, lsl #8
    orr     r3, r3, r3, lsl #16
    mov     r1, r1, lsr #4
block_loop_andor:
    ldmia   r0, {r4-r6,r12}
    subs    r1, r1, #1
    and     r4, r4, r2
    orr     r4, r4, r3
    and     r5, r5, r2
    orr     r5, r5, r3
    and     r6, r6, r2
    orr     r6, r6, r3
    and     r12,r12,r2
    orr     r12,r12,r3
    stmia   r0!, {r4-r6,r12}
    bne     block_loop_andor
    ldmfd   sp!, {r4-r6}
    bx      lr


.global spend_cycles @ c

spend_cycles:
    mov     r0, r0, lsr #2  @ 4 cycles/iteration
    sub     r0, r0, #2      @ entry/exit/init
.sc_loop:
    subs    r0, r0, #1
    bpl     .sc_loop

    bx      lr


.global memset32 @ int *dest, int c, int count

memset32:
    stmfd   sp!, {lr}

    mov     r3, r1
    subs    r2, r2, #4
    bmi     mst32_fin

    mov     r12,r1
    mov     lr, r1

mst32_loop:
    subs    r2, r2, #4
    stmia   r0!, {r1,r3,r12,lr}
    bpl     mst32_loop

mst32_fin:
    tst     r2, #1
    strne   r1, [r0], #4

    tst     r2, #2
    stmneia r0!, {r1,r3}

    ldmfd   sp!, {lr}
    bx      lr


@ warning: this code relies on palette being strictly RGB555, i.e. bit5=0
.global soft_scale @ void *dst, unsigned short *pal, int line_offs, int lines

soft_scale:
    stmfd   sp!,{r4-r11,lr}
    mov     lr, #0xff
    mov     lr, lr, lsl #1
    mov     r9, #0x3900        @ f800 07e0 001f | e000 0780 001c | 3800 01e0 0007
    orr     r9, r9, #0x00e7

    mov     r11,r3             @ r11= line counter
    mov     r3, r1             @ r3 = pal base

    mov     r12,#320
    mul     r2, r12,r2
    add     r4, r0, r2, lsl #1 @ r4 = dst_start
    add     r5, r0, r2         @ r5 = src_start
    mul     r12,r11,r12
    add     r0, r4, r12,lsl #1 @ r0 = dst_end
    add     r1, r5, r12        @ r1 = src_end

    mov     r2, r11

soft_scale_loop:
    sub     r1, r1, #64        @ skip borders
    orr     r2, r2, #(256/8-1)<<24

soft_scale_loop_line:
    ldr     r12, [r1, #-8]!
    ldr     r7,  [r1, #4]

    and     r4, lr, r12,lsl #1
    ldrh    r4, [r3, r4]
    and     r5, lr, r12,lsr #7
    ldrh    r5, [r3, r5]
    and     r11,r4, r9, lsl #2
    orr     r4, r4, r11,lsl #14       @ r4[31:16] = 1/4 pix_s 0
    and     r11,r5, r9, lsl #2
    sub     r6, r5, r11,lsr #2        @ r6 = 3/4 pix_s 1
    add     r4, r4, r6, lsl #16       @ pix_d 0, 1
    and     r6, lr, r12,lsr #15
    ldrh    r6, [r3, r6]
    and     r12,lr, r12,lsr #23
    ldrh    r12,[r3, r12]

    mov     r11,r6, ror #11
    adds    r5, r11,r5, ror #11
    mov     r5, r5, ror #22
    bic     r5, r5, #0xff000000
    bic     r5, r5, #0x0420           @ set the green bits as they should be
    orrcs   r5, r5, #0x0400

    and     r11,r6, r9, lsl #2
    sub     r6, r6, r11,lsr #2        @ r6 = 3/4 pix_s 2
    orr     r5, r5, r6, lsl #16

    and     r6, lr, r7, lsl #1
    ldrh    r6, [r3, r6]
    and     r11,r12,r9, lsl #2
    add     r5, r5, r11,lsl #14       @ pix_d 2, 3
    orr     r6, r12,r6, lsl #16       @ pix_d 4, 5

    and     r12,lr, r7, lsr #7
    ldrh    r12,[r3, r12]
    and     r10,lr, r7, lsr #15
    ldrh    r10,[r3, r10]
    and     r11,r12,r9, lsl #2
    sub     r8, r12,r11,lsr #2        @ r8 = 3/4 pix_s 1
    and     r11,r6, r9, lsl #18
    add     r8, r8, r11,lsr #18
    and     r7, lr, r7, lsr #23
    ldrh    r7, [r3, r7]

    mov     r11,r10,ror #11
    adds    r12,r11,r12,ror #11
    mov     r12,r12,ror #22
    bic     r12,r12,#0x0420
    orrcs   r12,r12,#0x0400
    orr     r8, r8, r12,lsl #16       @ pix_d 6, 7

    and     r11,r10,r9, lsl #2
    sub     r10,r10,r11,lsr #2        @ r10= 3/4 pix_s 2
    and     r11,r7, r9, lsl #2
    add     r10,r10,r11,lsr #2        @ += 1/4 pix_s 3
    orr     r10,r10,r7, lsl #16       @ pix_d 8, 9

    subs    r2, r2, #1<<24

    stmdb   r0!, {r4,r5,r6,r8,r10}
    bpl     soft_scale_loop_line

    add     r2, r2, #1<<24
    subs    r2, r2, #1
    bne     soft_scale_loop

    ldmfd   sp!,{r4-r11,lr}
    bx      lr


@ void convert2RGB555(unsigned short *dst, unsigned char *src, unsigned short *pal, int count);

.global convert2RGB555

convert2RGB555:
    stmfd   sp!,{r4-r8,lr}
    mov     lr, #0xff
    mov     lr, lr, lsl #1

    mov     r3, r3, lsr #3

convert2RGB555_loop:
    ldmia   r1!,{r4,r5}

    and     r6, lr, r4, lsl #1
    ldrh    r6, [r2, r6]
    and     r7, lr, r4, lsr #7
    ldrh    r7, [r2, r7]
    and     r8, lr, r4, lsr #15
    ldrh    r8, [r2, r8]
    and     r4, lr, r4, lsr #23
    ldrh    r4, [r2, r4]

    orr     r6, r6, r7, lsl #16
    and     r12,lr, r5, lsl #1
    ldrh    r12, [r2, r12]
    orr     r7, r8, r4, lsl #16
    and     r8, lr, r5, lsr #7
    ldrh    r8, [r2, r8]
    and     r4, lr, r5, lsr #15
    ldrh    r4, [r2, r4]
    and     r5, lr, r5, lsr #23
    ldrh    r5, [r2, r5]
    orr     r8, r12,r8, lsl #16
    orr     r12,r4, r5, lsl #16

    stmia   r0!,{r6,r7,r8,r12}
    subs    r3, r3, #1
    bne     convert2RGB555_loop

    ldmfd   sp!,{r4-r8,lr}
    bx      lr

