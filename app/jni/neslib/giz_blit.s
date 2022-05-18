
	  .global Blit8To16Asm


Blit8To16Asm:
	;@ r0 = pixFrom
	;@ r1 = pixTo
	;@ r2 = pal lookup
	stmfd sp!,{r4-r9}
	tst r1,#3
    bne aligned_16bit
aligned_32bit:
	mov r9,r3,lsr#4
1:
	ldmia r0!,{r3-r6}
	and r7,r3,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	and r8,r3,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	orr r7,r7,r8,lsl#16
	str r7,[r1],#4
	and r7,r3,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	and r8,r3,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	orr r7,r7,r8,lsl#16
	str r7,[r1],#4
	
	and r7,r4,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	and r8,r4,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	orr r7,r7,r8,lsl#16
	str r7,[r1],#4
	and r7,r4,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	and r8,r4,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	orr r7,r7,r8,lsl#16
	str r7,[r1],#4
	
	and r7,r5,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	and r8,r5,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	orr r7,r7,r8,lsl#16
	str r7,[r1],#4
	and r7,r5,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	and r8,r5,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	orr r7,r7,r8,lsl#16
	str r7,[r1],#4
	
	and r7,r6,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	and r8,r6,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	orr r7,r7,r8,lsl#16
	str r7,[r1],#4
	and r7,r6,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	and r8,r6,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	orr r7,r7,r8,lsl#16
	str r7,[r1],#4
	
	subs r9,r9,#1
	bne 1b
	
	ldmfd sp!,{r4-r9}
	mov pc,lr

aligned_16bit:
	;@ r0 = pixFrom
	;@ r1 = pixTo
	;@ r2 = pal lookup
	mov r9,r3,lsr#4
	sub r9,r9,#1
    ;@ first section handles initial 16bit write, then the rest of the writes are all 32bit
	ldmia r0!,{r3-r6}
	and r7,r3,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	and r8,r3,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	strh r7,[r1],#2
	
	and r7,r3,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r3,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
    
	and r7,r4,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r4,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	and r7,r4,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r4,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	
	and r7,r5,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r5,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	and r7,r5,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r5,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	
	and r7,r6,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r6,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	and r7,r6,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r6,#0xFF000000
	ldr r8,[r2,r8,lsr#22]

1:	
	ldmia r0!,{r3-r6}
	and r7,r3,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r3,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	and r7,r3,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r3,#0xFF000000
	ldr r8,[r2,r8,lsr#22]

	and r7,r4,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r4,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	and r7,r4,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r4,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	
	and r7,r5,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r5,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	and r7,r5,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r5,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	
	and r7,r6,#0x000000FF
	ldr r7,[r2,r7,lsl#2]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r6,#0x0000FF00
	ldr r8,[r2,r8,lsr#6]
	and r7,r6,#0x00FF0000
	ldr r7,[r2,r7,lsr#14]
	orr r7,r8,r7,lsl#16
	str r7,[r1],#4
	and r8,r6,#0xFF000000
	ldr r8,[r2,r8,lsr#22]
	
	subs r9,r9,#1
	bne 1b
	
	strh r8,[r1]
	ldmfd sp!,{r4-r9}
	mov pc,lr
	