
build/fib.o:     file format elf32-littlearm


Disassembly of section .text._Z6FibIntl:

00000000 <_Z6FibIntl>:
   0:	b538      	push	{r3, r4, r5, lr}
   2:	4604      	mov	r4, r0
   4:	2500      	movs	r5, #0
   6:	2c02      	cmp	r4, #2
   8:	dd05      	ble.n	16 <_Z6FibIntl+0x16>
   a:	1e60      	subs	r0, r4, #1
   c:	f7ff fffe 	bl	0 <_Z6FibIntl>
  10:	3c02      	subs	r4, #2
  12:	4405      	add	r5, r0
  14:	e7f7      	b.n	6 <_Z6FibIntl+0x6>
  16:	1c68      	adds	r0, r5, #1
  18:	bd38      	pop	{r3, r4, r5, pc}

Disassembly of section .text._Z8FibFloatl:

00000000 <_Z8FibFloatl>:
   0:	2802      	cmp	r0, #2
   2:	b5d0      	push	{r4, r6, r7, lr}
   4:	4604      	mov	r4, r0
   6:	dd10      	ble.n	2a <_Z8FibFloatl+0x2a>
   8:	3801      	subs	r0, #1
   a:	f7ff fffe 	bl	0 <_Z8FibFloatl>
   e:	1ea0      	subs	r0, r4, #2
  10:	ec57 6b10 	vmov	r6, r7, d0
  14:	f7ff fffe 	bl	0 <_Z8FibFloatl>
  18:	4630      	mov	r0, r6
  1a:	ec53 2b10 	vmov	r2, r3, d0
  1e:	4639      	mov	r1, r7
  20:	f7ff fffe 	bl	0 <__aeabi_dadd>
  24:	ec41 0b10 	vmov	d0, r0, r1
  28:	bdd0      	pop	{r4, r6, r7, pc}
  2a:	ed9f 0b01 	vldr	d0, [pc, #4]	; 30 <_Z8FibFloatl+0x30>
  2e:	e7fb      	b.n	28 <_Z8FibFloatl+0x28>
  30:	00000000 	.word	0x00000000
  34:	3ff00000 	.word	0x3ff00000
