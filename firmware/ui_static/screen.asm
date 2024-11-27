
build/screen.o:     file format elf32-littlearm


Disassembly of section .text._ZL7GetFont5Fonts:

00000000 <_ZL7GetFont5Fonts>:
   0:	b480      	push	{r7}
   2:	b083      	sub	sp, #12
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	2b03      	cmp	r3, #3
   c:	d812      	bhi.n	34 <_ZL7GetFont5Fonts+0x34>
   e:	a201      	add	r2, pc, #4	; (adr r2, 14 <_ZL7GetFont5Fonts+0x14>)
  10:	f852 f023 	ldr.w	pc, [r2, r3, lsl #2]
  14:	00000025 	.word	0x00000025
  18:	00000029 	.word	0x00000029
  1c:	0000002d 	.word	0x0000002d
  20:	00000031 	.word	0x00000031
  24:	4b07      	ldr	r3, [pc, #28]	; (44 <_ZL7GetFont5Fonts+0x44>)
  26:	e007      	b.n	38 <_ZL7GetFont5Fonts+0x38>
  28:	4b07      	ldr	r3, [pc, #28]	; (48 <_ZL7GetFont5Fonts+0x48>)
  2a:	e005      	b.n	38 <_ZL7GetFont5Fonts+0x38>
  2c:	4b07      	ldr	r3, [pc, #28]	; (4c <_ZL7GetFont5Fonts+0x4c>)
  2e:	e003      	b.n	38 <_ZL7GetFont5Fonts+0x38>
  30:	4b07      	ldr	r3, [pc, #28]	; (50 <_ZL7GetFont5Fonts+0x50>)
  32:	e001      	b.n	38 <_ZL7GetFont5Fonts+0x38>
  34:	bf00      	nop
  36:	4b03      	ldr	r3, [pc, #12]	; (44 <_ZL7GetFont5Fonts+0x44>)
  38:	4618      	mov	r0, r3
  3a:	370c      	adds	r7, #12
  3c:	46bd      	mov	sp, r7
  3e:	f85d 7b04 	ldr.w	r7, [sp], #4
  42:	4770      	bx	lr
	...

Disassembly of section .text._ZN6ScreenC2ER19__SPI_HandleTypeDefP12GPIO_TypeDeftS3_tS3_tS3_tNS_11OrientationE:

00000000 <_ZN6ScreenC1ER19__SPI_HandleTypeDefP12GPIO_TypeDeftS3_tS3_tS3_tNS_11OrientationE>:
   0:	b580      	push	{r7, lr}
   2:	b084      	sub	sp, #16
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	607a      	str	r2, [r7, #4]
   c:	807b      	strh	r3, [r7, #2]
   e:	68fb      	ldr	r3, [r7, #12]
  10:	68ba      	ldr	r2, [r7, #8]
  12:	601a      	str	r2, [r3, #0]
  14:	68fb      	ldr	r3, [r7, #12]
  16:	687a      	ldr	r2, [r7, #4]
  18:	605a      	str	r2, [r3, #4]
  1a:	68fb      	ldr	r3, [r7, #12]
  1c:	887a      	ldrh	r2, [r7, #2]
  1e:	811a      	strh	r2, [r3, #8]
  20:	68fb      	ldr	r3, [r7, #12]
  22:	69ba      	ldr	r2, [r7, #24]
  24:	60da      	str	r2, [r3, #12]
  26:	68fb      	ldr	r3, [r7, #12]
  28:	8bba      	ldrh	r2, [r7, #28]
  2a:	821a      	strh	r2, [r3, #16]
  2c:	68fb      	ldr	r3, [r7, #12]
  2e:	6a3a      	ldr	r2, [r7, #32]
  30:	615a      	str	r2, [r3, #20]
  32:	68fb      	ldr	r3, [r7, #12]
  34:	8cba      	ldrh	r2, [r7, #36]	; 0x24
  36:	831a      	strh	r2, [r3, #24]
  38:	68fb      	ldr	r3, [r7, #12]
  3a:	6aba      	ldr	r2, [r7, #40]	; 0x28
  3c:	61da      	str	r2, [r3, #28]
  3e:	68fb      	ldr	r3, [r7, #12]
  40:	8dba      	ldrh	r2, [r7, #44]	; 0x2c
  42:	841a      	strh	r2, [r3, #32]
  44:	68fb      	ldr	r3, [r7, #12]
  46:	f897 2030 	ldrb.w	r2, [r7, #48]	; 0x30
  4a:	f883 2022 	strb.w	r2, [r3, #34]	; 0x22
  4e:	68fb      	ldr	r3, [r7, #12]
  50:	f44f 72a0 	mov.w	r2, #320	; 0x140
  54:	849a      	strh	r2, [r3, #36]	; 0x24
  56:	68fb      	ldr	r3, [r7, #12]
  58:	22f0      	movs	r2, #240	; 0xf0
  5a:	84da      	strh	r2, [r3, #38]	; 0x26
  5c:	68fb      	ldr	r3, [r7, #12]
  5e:	332c      	adds	r3, #44	; 0x2c
  60:	f640 2228 	movw	r2, #2600	; 0xa28
  64:	2100      	movs	r1, #0
  66:	4618      	mov	r0, r3
  68:	f7ff fffe 	bl	0 <memset>
  6c:	68fb      	ldr	r3, [r7, #12]
  6e:	2200      	movs	r2, #0
  70:	f8c3 2a54 	str.w	r2, [r3, #2644]	; 0xa54
  74:	68fb      	ldr	r3, [r7, #12]
  76:	2200      	movs	r2, #0
  78:	f8c3 2a58 	str.w	r2, [r3, #2648]	; 0xa58
  7c:	68fb      	ldr	r3, [r7, #12]
  7e:	2200      	movs	r2, #0
  80:	f8a3 2a5c 	strh.w	r2, [r3, #2652]	; 0xa5c
  84:	68fb      	ldr	r3, [r7, #12]
  86:	2200      	movs	r2, #0
  88:	f883 2a5e 	strb.w	r2, [r3, #2654]	; 0xa5e
  8c:	68fb      	ldr	r3, [r7, #12]
  8e:	2200      	movs	r2, #0
  90:	f883 2a5f 	strb.w	r2, [r3, #2655]	; 0xa5f
  94:	68fb      	ldr	r3, [r7, #12]
  96:	f503 6326 	add.w	r3, r3, #2656	; 0xa60
  9a:	f44f 5296 	mov.w	r2, #4800	; 0x12c0
  9e:	2100      	movs	r1, #0
  a0:	4618      	mov	r0, r3
  a2:	f7ff fffe 	bl	0 <memset>
  a6:	68fb      	ldr	r3, [r7, #12]
  a8:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  ac:	f503 6352 	add.w	r3, r3, #3360	; 0xd20
  b0:	2200      	movs	r2, #0
  b2:	601a      	str	r2, [r3, #0]
  b4:	605a      	str	r2, [r3, #4]
  b6:	609a      	str	r2, [r3, #8]
  b8:	60da      	str	r2, [r3, #12]
  ba:	821a      	strh	r2, [r3, #16]
  bc:	68fb      	ldr	r3, [r7, #12]
  be:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  c2:	461a      	mov	r2, r3
  c4:	2300      	movs	r3, #0
  c6:	f8c2 3d34 	str.w	r3, [r2, #3380]	; 0xd34
  ca:	68fb      	ldr	r3, [r7, #12]
  cc:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  d0:	461a      	mov	r2, r3
  d2:	2300      	movs	r3, #0
  d4:	f8c2 3d38 	str.w	r3, [r2, #3384]	; 0xd38
  d8:	68fb      	ldr	r3, [r7, #12]
  da:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  de:	461a      	mov	r2, r3
  e0:	2300      	movs	r3, #0
  e2:	f8c2 3d3c 	str.w	r3, [r2, #3388]	; 0xd3c
  e6:	68fb      	ldr	r3, [r7, #12]
  e8:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  ec:	f503 6354 	add.w	r3, r3, #3392	; 0xd40
  f0:	f44f 62d2 	mov.w	r2, #1680	; 0x690
  f4:	2100      	movs	r1, #0
  f6:	4618      	mov	r0, r3
  f8:	f7ff fffe 	bl	0 <memset>
  fc:	68fb      	ldr	r3, [r7, #12]
  fe:	f503 5300 	add.w	r3, r3, #8192	; 0x2000
 102:	f203 33f3 	addw	r3, r3, #1011	; 0x3f3
 106:	2230      	movs	r2, #48	; 0x30
 108:	2100      	movs	r1, #0
 10a:	4618      	mov	r0, r3
 10c:	f7ff fffe 	bl	0 <memset>
 110:	68fb      	ldr	r3, [r7, #12]
 112:	4618      	mov	r0, r3
 114:	3710      	adds	r7, #16
 116:	46bd      	mov	sp, r7
 118:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen4InitEv:

00000000 <_ZN6Screen4InitEv>:
   0:	b590      	push	{r4, r7, lr}
   2:	b095      	sub	sp, #84	; 0x54
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	6878      	ldr	r0, [r7, #4]
   a:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
   e:	6878      	ldr	r0, [r7, #4]
  10:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  14:	2101      	movs	r1, #1
  16:	6878      	ldr	r0, [r7, #4]
  18:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  1c:	2005      	movs	r0, #5
  1e:	f7ff fffe 	bl	0 <HAL_Delay>
  22:	21cb      	movs	r1, #203	; 0xcb
  24:	6878      	ldr	r0, [r7, #4]
  26:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  2a:	4a81      	ldr	r2, [pc, #516]	; (230 <_ZN6Screen4InitEv+0x230>)
  2c:	f107 0348 	add.w	r3, r7, #72	; 0x48
  30:	e892 0003 	ldmia.w	r2, {r0, r1}
  34:	6018      	str	r0, [r3, #0]
  36:	3304      	adds	r3, #4
  38:	7019      	strb	r1, [r3, #0]
  3a:	f107 0348 	add.w	r3, r7, #72	; 0x48
  3e:	2205      	movs	r2, #5
  40:	4619      	mov	r1, r3
  42:	6878      	ldr	r0, [r7, #4]
  44:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  48:	21cf      	movs	r1, #207	; 0xcf
  4a:	6878      	ldr	r0, [r7, #4]
  4c:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  50:	4a78      	ldr	r2, [pc, #480]	; (234 <_ZN6Screen4InitEv+0x234>)
  52:	f107 0344 	add.w	r3, r7, #68	; 0x44
  56:	6812      	ldr	r2, [r2, #0]
  58:	4611      	mov	r1, r2
  5a:	8019      	strh	r1, [r3, #0]
  5c:	3302      	adds	r3, #2
  5e:	0c12      	lsrs	r2, r2, #16
  60:	701a      	strb	r2, [r3, #0]
  62:	f107 0344 	add.w	r3, r7, #68	; 0x44
  66:	2203      	movs	r2, #3
  68:	4619      	mov	r1, r3
  6a:	6878      	ldr	r0, [r7, #4]
  6c:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  70:	21e8      	movs	r1, #232	; 0xe8
  72:	6878      	ldr	r0, [r7, #4]
  74:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  78:	4a6f      	ldr	r2, [pc, #444]	; (238 <_ZN6Screen4InitEv+0x238>)
  7a:	f107 0340 	add.w	r3, r7, #64	; 0x40
  7e:	6812      	ldr	r2, [r2, #0]
  80:	4611      	mov	r1, r2
  82:	8019      	strh	r1, [r3, #0]
  84:	3302      	adds	r3, #2
  86:	0c12      	lsrs	r2, r2, #16
  88:	701a      	strb	r2, [r3, #0]
  8a:	f107 0340 	add.w	r3, r7, #64	; 0x40
  8e:	2203      	movs	r2, #3
  90:	4619      	mov	r1, r3
  92:	6878      	ldr	r0, [r7, #4]
  94:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  98:	21ea      	movs	r1, #234	; 0xea
  9a:	6878      	ldr	r0, [r7, #4]
  9c:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  a0:	2300      	movs	r3, #0
  a2:	87bb      	strh	r3, [r7, #60]	; 0x3c
  a4:	f107 033c 	add.w	r3, r7, #60	; 0x3c
  a8:	2202      	movs	r2, #2
  aa:	4619      	mov	r1, r3
  ac:	6878      	ldr	r0, [r7, #4]
  ae:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  b2:	21ed      	movs	r1, #237	; 0xed
  b4:	6878      	ldr	r0, [r7, #4]
  b6:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  ba:	4b60      	ldr	r3, [pc, #384]	; (23c <_ZN6Screen4InitEv+0x23c>)
  bc:	63bb      	str	r3, [r7, #56]	; 0x38
  be:	f107 0338 	add.w	r3, r7, #56	; 0x38
  c2:	2204      	movs	r2, #4
  c4:	4619      	mov	r1, r3
  c6:	6878      	ldr	r0, [r7, #4]
  c8:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  cc:	21f7      	movs	r1, #247	; 0xf7
  ce:	6878      	ldr	r0, [r7, #4]
  d0:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  d4:	2120      	movs	r1, #32
  d6:	6878      	ldr	r0, [r7, #4]
  d8:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  dc:	21c0      	movs	r1, #192	; 0xc0
  de:	6878      	ldr	r0, [r7, #4]
  e0:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  e4:	2123      	movs	r1, #35	; 0x23
  e6:	6878      	ldr	r0, [r7, #4]
  e8:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  ec:	21c1      	movs	r1, #193	; 0xc1
  ee:	6878      	ldr	r0, [r7, #4]
  f0:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  f4:	2110      	movs	r1, #16
  f6:	6878      	ldr	r0, [r7, #4]
  f8:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
  fc:	21c5      	movs	r1, #197	; 0xc5
  fe:	6878      	ldr	r0, [r7, #4]
 100:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 104:	f642 033e 	movw	r3, #10302	; 0x283e
 108:	86bb      	strh	r3, [r7, #52]	; 0x34
 10a:	f107 0334 	add.w	r3, r7, #52	; 0x34
 10e:	2202      	movs	r2, #2
 110:	4619      	mov	r1, r3
 112:	6878      	ldr	r0, [r7, #4]
 114:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 118:	21c7      	movs	r1, #199	; 0xc7
 11a:	6878      	ldr	r0, [r7, #4]
 11c:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 120:	2186      	movs	r1, #134	; 0x86
 122:	6878      	ldr	r0, [r7, #4]
 124:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 128:	2136      	movs	r1, #54	; 0x36
 12a:	6878      	ldr	r0, [r7, #4]
 12c:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 130:	2148      	movs	r1, #72	; 0x48
 132:	6878      	ldr	r0, [r7, #4]
 134:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 138:	213a      	movs	r1, #58	; 0x3a
 13a:	6878      	ldr	r0, [r7, #4]
 13c:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 140:	2155      	movs	r1, #85	; 0x55
 142:	6878      	ldr	r0, [r7, #4]
 144:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 148:	21b1      	movs	r1, #177	; 0xb1
 14a:	6878      	ldr	r0, [r7, #4]
 14c:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 150:	f44f 53c0 	mov.w	r3, #6144	; 0x1800
 154:	863b      	strh	r3, [r7, #48]	; 0x30
 156:	f107 0330 	add.w	r3, r7, #48	; 0x30
 15a:	2202      	movs	r2, #2
 15c:	4619      	mov	r1, r3
 15e:	6878      	ldr	r0, [r7, #4]
 160:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 164:	21b6      	movs	r1, #182	; 0xb6
 166:	6878      	ldr	r0, [r7, #4]
 168:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 16c:	4a34      	ldr	r2, [pc, #208]	; (240 <_ZN6Screen4InitEv+0x240>)
 16e:	f107 032c 	add.w	r3, r7, #44	; 0x2c
 172:	6812      	ldr	r2, [r2, #0]
 174:	4611      	mov	r1, r2
 176:	8019      	strh	r1, [r3, #0]
 178:	3302      	adds	r3, #2
 17a:	0c12      	lsrs	r2, r2, #16
 17c:	701a      	strb	r2, [r3, #0]
 17e:	f107 032c 	add.w	r3, r7, #44	; 0x2c
 182:	2203      	movs	r2, #3
 184:	4619      	mov	r1, r3
 186:	6878      	ldr	r0, [r7, #4]
 188:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 18c:	21f2      	movs	r1, #242	; 0xf2
 18e:	6878      	ldr	r0, [r7, #4]
 190:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 194:	2100      	movs	r1, #0
 196:	6878      	ldr	r0, [r7, #4]
 198:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 19c:	2126      	movs	r1, #38	; 0x26
 19e:	6878      	ldr	r0, [r7, #4]
 1a0:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 1a4:	2101      	movs	r1, #1
 1a6:	6878      	ldr	r0, [r7, #4]
 1a8:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 1ac:	21e0      	movs	r1, #224	; 0xe0
 1ae:	6878      	ldr	r0, [r7, #4]
 1b0:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 1b4:	4b23      	ldr	r3, [pc, #140]	; (244 <_ZN6Screen4InitEv+0x244>)
 1b6:	f107 041c 	add.w	r4, r7, #28
 1ba:	cb0f      	ldmia	r3, {r0, r1, r2, r3}
 1bc:	c407      	stmia	r4!, {r0, r1, r2}
 1be:	8023      	strh	r3, [r4, #0]
 1c0:	3402      	adds	r4, #2
 1c2:	0c1b      	lsrs	r3, r3, #16
 1c4:	7023      	strb	r3, [r4, #0]
 1c6:	f107 031c 	add.w	r3, r7, #28
 1ca:	220f      	movs	r2, #15
 1cc:	4619      	mov	r1, r3
 1ce:	6878      	ldr	r0, [r7, #4]
 1d0:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 1d4:	21e1      	movs	r1, #225	; 0xe1
 1d6:	6878      	ldr	r0, [r7, #4]
 1d8:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 1dc:	4b1a      	ldr	r3, [pc, #104]	; (248 <_ZN6Screen4InitEv+0x248>)
 1de:	f107 040c 	add.w	r4, r7, #12
 1e2:	cb0f      	ldmia	r3, {r0, r1, r2, r3}
 1e4:	c407      	stmia	r4!, {r0, r1, r2}
 1e6:	8023      	strh	r3, [r4, #0]
 1e8:	3402      	adds	r4, #2
 1ea:	0c1b      	lsrs	r3, r3, #16
 1ec:	7023      	strb	r3, [r4, #0]
 1ee:	f107 030c 	add.w	r3, r7, #12
 1f2:	220f      	movs	r2, #15
 1f4:	4619      	mov	r1, r3
 1f6:	6878      	ldr	r0, [r7, #4]
 1f8:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 1fc:	2113      	movs	r1, #19
 1fe:	6878      	ldr	r0, [r7, #4]
 200:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 204:	2111      	movs	r1, #17
 206:	6878      	ldr	r0, [r7, #4]
 208:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 20c:	2078      	movs	r0, #120	; 0x78
 20e:	f7ff fffe 	bl	0 <HAL_Delay>
 212:	2129      	movs	r1, #41	; 0x29
 214:	6878      	ldr	r0, [r7, #4]
 216:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 21a:	687b      	ldr	r3, [r7, #4]
 21c:	f893 3022 	ldrb.w	r3, [r3, #34]	; 0x22
 220:	4619      	mov	r1, r3
 222:	6878      	ldr	r0, [r7, #4]
 224:	f7ff fffe 	bl	0 <_ZN6Screen4InitEv>
 228:	bf00      	nop
 22a:	3754      	adds	r7, #84	; 0x54
 22c:	46bd      	mov	sp, r7
 22e:	bd90      	pop	{r4, r7, pc}
 230:	00000000 	.word	0x00000000
 234:	00000008 	.word	0x00000008
 238:	0000000c 	.word	0x0000000c
 23c:	81120364 	.word	0x81120364
 240:	00000010 	.word	0x00000010
 244:	00000014 	.word	0x00000014
 248:	00000024 	.word	0x00000024

Disassembly of section .text._ZN6Screen4DrawEm:

00000000 <_ZN6Screen4DrawEm>:
   0:	b590      	push	{r4, r7, lr}
   2:	b08b      	sub	sp, #44	; 0x2c
   4:	af02      	add	r7, sp, #8
   6:	6078      	str	r0, [r7, #4]
   8:	6039      	str	r1, [r7, #0]
   a:	687b      	ldr	r3, [r7, #4]
   c:	f893 3a5f 	ldrb.w	r3, [r3, #2655]	; 0xa5f
  10:	2b00      	cmp	r3, #0
  12:	d01b      	beq.n	4c <_ZN6Screen4DrawEm+0x4c>
  14:	687b      	ldr	r3, [r7, #4]
  16:	2200      	movs	r2, #0
  18:	f8a3 2a5c 	strh.w	r2, [r3, #2652]	; 0xa5c
  1c:	687b      	ldr	r3, [r7, #4]
  1e:	2200      	movs	r2, #0
  20:	f883 2a5f 	strb.w	r2, [r3, #2655]	; 0xa5f
  24:	687b      	ldr	r3, [r7, #4]
  26:	2201      	movs	r2, #1
  28:	f883 2a5e 	strb.w	r2, [r3, #2654]	; 0xa5e
  2c:	687b      	ldr	r3, [r7, #4]
  2e:	8cdb      	ldrh	r3, [r3, #38]	; 0x26
  30:	3b01      	subs	r3, #1
  32:	b29b      	uxth	r3, r3
  34:	b21a      	sxth	r2, r3
  36:	687b      	ldr	r3, [r7, #4]
  38:	8c9b      	ldrh	r3, [r3, #36]	; 0x24
  3a:	3b01      	subs	r3, #1
  3c:	b29b      	uxth	r3, r3
  3e:	b21b      	sxth	r3, r3
  40:	9300      	str	r3, [sp, #0]
  42:	2300      	movs	r3, #0
  44:	2100      	movs	r1, #0
  46:	6878      	ldr	r0, [r7, #4]
  48:	f7ff fffe 	bl	0 <_ZN6Screen4DrawEm>
  4c:	687b      	ldr	r3, [r7, #4]
  4e:	f893 3a5e 	ldrb.w	r3, [r3, #2654]	; 0xa5e
  52:	f083 0301 	eor.w	r3, r3, #1
  56:	b2db      	uxtb	r3, r3
  58:	2b00      	cmp	r3, #0
  5a:	d17e      	bne.n	15a <_ZN6Screen4DrawEm+0x15a>
  5c:	6878      	ldr	r0, [r7, #4]
  5e:	f7ff fffe 	bl	0 <_ZN6Screen4DrawEm>
  62:	687b      	ldr	r3, [r7, #4]
  64:	f503 6326 	add.w	r3, r3, #2656	; 0xa60
  68:	617b      	str	r3, [r7, #20]
  6a:	2300      	movs	r3, #0
  6c:	61fb      	str	r3, [r7, #28]
  6e:	69fb      	ldr	r3, [r7, #28]
  70:	f5b3 5f96 	cmp.w	r3, #4800	; 0x12c0
  74:	d208      	bcs.n	88 <_ZN6Screen4DrawEm+0x88>
  76:	697a      	ldr	r2, [r7, #20]
  78:	69fb      	ldr	r3, [r7, #28]
  7a:	4413      	add	r3, r2
  7c:	2200      	movs	r2, #0
  7e:	701a      	strb	r2, [r3, #0]
  80:	69fb      	ldr	r3, [r7, #28]
  82:	3301      	adds	r3, #1
  84:	61fb      	str	r3, [r7, #28]
  86:	e7f2      	b.n	6e <_ZN6Screen4DrawEm+0x6e>
  88:	2300      	movs	r3, #0
  8a:	617b      	str	r3, [r7, #20]
  8c:	687b      	ldr	r3, [r7, #4]
  8e:	f8b3 3a5c 	ldrh.w	r3, [r3, #2652]	; 0xa5c
  92:	330a      	adds	r3, #10
  94:	827b      	strh	r3, [r7, #18]
  96:	2300      	movs	r3, #0
  98:	837b      	strh	r3, [r7, #26]
  9a:	8b7b      	ldrh	r3, [r7, #26]
  9c:	2b31      	cmp	r3, #49	; 0x31
  9e:	d83b      	bhi.n	118 <_ZN6Screen4DrawEm+0x118>
  a0:	8b7b      	ldrh	r3, [r7, #26]
  a2:	2234      	movs	r2, #52	; 0x34
  a4:	fb02 f303 	mul.w	r3, r2, r3
  a8:	3328      	adds	r3, #40	; 0x28
  aa:	687a      	ldr	r2, [r7, #4]
  ac:	4413      	add	r3, r2
  ae:	3304      	adds	r3, #4
  b0:	60fb      	str	r3, [r7, #12]
  b2:	68fb      	ldr	r3, [r7, #12]
  b4:	791b      	ldrb	r3, [r3, #4]
  b6:	2b00      	cmp	r3, #0
  b8:	d027      	beq.n	10a <_ZN6Screen4DrawEm+0x10a>
  ba:	2b01      	cmp	r3, #1
  bc:	d122      	bne.n	104 <_ZN6Screen4DrawEm+0x104>
  be:	68fb      	ldr	r3, [r7, #12]
  c0:	681c      	ldr	r4, [r3, #0]
  c2:	687b      	ldr	r3, [r7, #4]
  c4:	f503 6126 	add.w	r1, r3, #2656	; 0xa60
  c8:	687b      	ldr	r3, [r7, #4]
  ca:	f8b3 3a5c 	ldrh.w	r3, [r3, #2652]	; 0xa5c
  ce:	b21a      	sxth	r2, r3
  d0:	f9b7 3012 	ldrsh.w	r3, [r7, #18]
  d4:	68f8      	ldr	r0, [r7, #12]
  d6:	47a0      	blx	r4
  d8:	68fb      	ldr	r3, [r7, #12]
  da:	899b      	ldrh	r3, [r3, #12]
  dc:	8a7a      	ldrh	r2, [r7, #18]
  de:	429a      	cmp	r2, r3
  e0:	d315      	bcc.n	10e <_ZN6Screen4DrawEm+0x10e>
  e2:	68fb      	ldr	r3, [r7, #12]
  e4:	2200      	movs	r2, #0
  e6:	711a      	strb	r2, [r3, #4]
  e8:	68fb      	ldr	r3, [r7, #12]
  ea:	2200      	movs	r2, #0
  ec:	745a      	strb	r2, [r3, #17]
  ee:	68fb      	ldr	r3, [r7, #12]
  f0:	2200      	movs	r2, #0
  f2:	741a      	strb	r2, [r3, #16]
  f4:	687b      	ldr	r3, [r7, #4]
  f6:	f8d3 3a54 	ldr.w	r3, [r3, #2644]	; 0xa54
  fa:	1e5a      	subs	r2, r3, #1
  fc:	687b      	ldr	r3, [r7, #4]
  fe:	f8c3 2a54 	str.w	r2, [r3, #2644]	; 0xa54
 102:	e004      	b.n	10e <_ZN6Screen4DrawEm+0x10e>
 104:	f7ff fffe 	bl	0 <Error_Handler>
 108:	e002      	b.n	110 <_ZN6Screen4DrawEm+0x110>
 10a:	bf00      	nop
 10c:	e000      	b.n	110 <_ZN6Screen4DrawEm+0x110>
 10e:	bf00      	nop
 110:	8b7b      	ldrh	r3, [r7, #26]
 112:	3301      	adds	r3, #1
 114:	837b      	strh	r3, [r7, #26]
 116:	e7c0      	b.n	9a <_ZN6Screen4DrawEm+0x9a>
 118:	687b      	ldr	r3, [r7, #4]
 11a:	f503 6326 	add.w	r3, r3, #2656	; 0xa60
 11e:	f44f 5296 	mov.w	r2, #4800	; 0x12c0
 122:	4619      	mov	r1, r3
 124:	6878      	ldr	r0, [r7, #4]
 126:	f7ff fffe 	bl	0 <_ZN6Screen4DrawEm>
 12a:	687b      	ldr	r3, [r7, #4]
 12c:	f8b3 3a5c 	ldrh.w	r3, [r3, #2652]	; 0xa5c
 130:	330a      	adds	r3, #10
 132:	b29a      	uxth	r2, r3
 134:	687b      	ldr	r3, [r7, #4]
 136:	f8a3 2a5c 	strh.w	r2, [r3, #2652]	; 0xa5c
 13a:	687b      	ldr	r3, [r7, #4]
 13c:	f8b3 2a5c 	ldrh.w	r2, [r3, #2652]	; 0xa5c
 140:	687b      	ldr	r3, [r7, #4]
 142:	8c9b      	ldrh	r3, [r3, #36]	; 0x24
 144:	429a      	cmp	r2, r3
 146:	d309      	bcc.n	15c <_ZN6Screen4DrawEm+0x15c>
 148:	687b      	ldr	r3, [r7, #4]
 14a:	2200      	movs	r2, #0
 14c:	f8a3 2a5c 	strh.w	r2, [r3, #2652]	; 0xa5c
 150:	687b      	ldr	r3, [r7, #4]
 152:	2200      	movs	r2, #0
 154:	f883 2a5e 	strb.w	r2, [r3, #2654]	; 0xa5e
 158:	e000      	b.n	15c <_ZN6Screen4DrawEm+0x15c>
 15a:	bf00      	nop
 15c:	3724      	adds	r7, #36	; 0x24
 15e:	46bd      	mov	sp, r7
 160:	bd90      	pop	{r4, r7, pc}

Disassembly of section .text._ZN6Screen5SleepEv:

00000000 <_ZN6Screen5SleepEv>:
   0:	b480      	push	{r7}
   2:	b083      	sub	sp, #12
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	bf00      	nop
   a:	370c      	adds	r7, #12
   c:	46bd      	mov	sp, r7
   e:	f85d 7b04 	ldr.w	r7, [sp], #4
  12:	4770      	bx	lr

Disassembly of section .text._ZN6Screen4WakeEv:

00000000 <_ZN6Screen4WakeEv>:
   0:	b480      	push	{r7}
   2:	b083      	sub	sp, #12
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	bf00      	nop
   a:	370c      	adds	r7, #12
   c:	46bd      	mov	sp, r7
   e:	f85d 7b04 	ldr.w	r7, [sp], #4
  12:	4770      	bx	lr

Disassembly of section .text._ZN6Screen5ResetEv:

00000000 <_ZN6Screen5ResetEv>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	6958      	ldr	r0, [r3, #20]
   c:	687b      	ldr	r3, [r7, #4]
   e:	8b1b      	ldrh	r3, [r3, #24]
  10:	2200      	movs	r2, #0
  12:	4619      	mov	r1, r3
  14:	f7ff fffe 	bl	0 <HAL_GPIO_WritePin>
  18:	2032      	movs	r0, #50	; 0x32
  1a:	f7ff fffe 	bl	0 <HAL_Delay>
  1e:	687b      	ldr	r3, [r7, #4]
  20:	6958      	ldr	r0, [r3, #20]
  22:	687b      	ldr	r3, [r7, #4]
  24:	8b1b      	ldrh	r3, [r3, #24]
  26:	2201      	movs	r2, #1
  28:	4619      	mov	r1, r3
  2a:	f7ff fffe 	bl	0 <HAL_GPIO_WritePin>
  2e:	bf00      	nop
  30:	3708      	adds	r7, #8
  32:	46bd      	mov	sp, r7
  34:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen6SelectEv:

00000000 <_ZN6Screen6SelectEv>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	6858      	ldr	r0, [r3, #4]
   c:	687b      	ldr	r3, [r7, #4]
   e:	891b      	ldrh	r3, [r3, #8]
  10:	2200      	movs	r2, #0
  12:	4619      	mov	r1, r3
  14:	f7ff fffe 	bl	0 <HAL_GPIO_WritePin>
  18:	bf00      	nop
  1a:	3708      	adds	r7, #8
  1c:	46bd      	mov	sp, r7
  1e:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen15EnableBacklightEv:

00000000 <_ZN6Screen15EnableBacklightEv>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	69d8      	ldr	r0, [r3, #28]
   c:	687b      	ldr	r3, [r7, #4]
   e:	8c1b      	ldrh	r3, [r3, #32]
  10:	2201      	movs	r2, #1
  12:	4619      	mov	r1, r3
  14:	f7ff fffe 	bl	0 <HAL_GPIO_WritePin>
  18:	bf00      	nop
  1a:	3708      	adds	r7, #8
  1c:	46bd      	mov	sp, r7
  1e:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen16DisableBacklightEv:

00000000 <_ZN6Screen16DisableBacklightEv>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	69d8      	ldr	r0, [r3, #28]
   c:	687b      	ldr	r3, [r7, #4]
   e:	8c1b      	ldrh	r3, [r3, #32]
  10:	2200      	movs	r2, #0
  12:	4619      	mov	r1, r3
  14:	f7ff fffe 	bl	0 <HAL_GPIO_WritePin>
  18:	bf00      	nop
  1a:	3708      	adds	r7, #8
  1c:	46bd      	mov	sp, r7
  1e:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen18SetWriteablePixelsEssss:

00000000 <_ZN6Screen18SetWriteablePixelsEssss>:
   0:	b580      	push	{r7, lr}
   2:	b086      	sub	sp, #24
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	4608      	mov	r0, r1
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	4603      	mov	r3, r0
  10:	817b      	strh	r3, [r7, #10]
  12:	460b      	mov	r3, r1
  14:	813b      	strh	r3, [r7, #8]
  16:	4613      	mov	r3, r2
  18:	80fb      	strh	r3, [r7, #6]
  1a:	f9b7 300a 	ldrsh.w	r3, [r7, #10]
  1e:	121b      	asrs	r3, r3, #8
  20:	b21b      	sxth	r3, r3
  22:	b2db      	uxtb	r3, r3
  24:	753b      	strb	r3, [r7, #20]
  26:	897b      	ldrh	r3, [r7, #10]
  28:	b2db      	uxtb	r3, r3
  2a:	757b      	strb	r3, [r7, #21]
  2c:	f9b7 3008 	ldrsh.w	r3, [r7, #8]
  30:	121b      	asrs	r3, r3, #8
  32:	b21b      	sxth	r3, r3
  34:	b2db      	uxtb	r3, r3
  36:	75bb      	strb	r3, [r7, #22]
  38:	893b      	ldrh	r3, [r7, #8]
  3a:	b2db      	uxtb	r3, r3
  3c:	75fb      	strb	r3, [r7, #23]
  3e:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  42:	121b      	asrs	r3, r3, #8
  44:	b21b      	sxth	r3, r3
  46:	b2db      	uxtb	r3, r3
  48:	743b      	strb	r3, [r7, #16]
  4a:	88fb      	ldrh	r3, [r7, #6]
  4c:	b2db      	uxtb	r3, r3
  4e:	747b      	strb	r3, [r7, #17]
  50:	f9b7 3020 	ldrsh.w	r3, [r7, #32]
  54:	121b      	asrs	r3, r3, #8
  56:	b21b      	sxth	r3, r3
  58:	b2db      	uxtb	r3, r3
  5a:	74bb      	strb	r3, [r7, #18]
  5c:	8c3b      	ldrh	r3, [r7, #32]
  5e:	b2db      	uxtb	r3, r3
  60:	74fb      	strb	r3, [r7, #19]
  62:	212a      	movs	r1, #42	; 0x2a
  64:	68f8      	ldr	r0, [r7, #12]
  66:	f7ff fffe 	bl	0 <_ZN6Screen18SetWriteablePixelsEssss>
  6a:	f107 0314 	add.w	r3, r7, #20
  6e:	2204      	movs	r2, #4
  70:	4619      	mov	r1, r3
  72:	68f8      	ldr	r0, [r7, #12]
  74:	f7ff fffe 	bl	0 <_ZN6Screen18SetWriteablePixelsEssss>
  78:	212b      	movs	r1, #43	; 0x2b
  7a:	68f8      	ldr	r0, [r7, #12]
  7c:	f7ff fffe 	bl	0 <_ZN6Screen18SetWriteablePixelsEssss>
  80:	f107 0310 	add.w	r3, r7, #16
  84:	2204      	movs	r2, #4
  86:	4619      	mov	r1, r3
  88:	68f8      	ldr	r0, [r7, #12]
  8a:	f7ff fffe 	bl	0 <_ZN6Screen18SetWriteablePixelsEssss>
  8e:	212c      	movs	r1, #44	; 0x2c
  90:	68f8      	ldr	r0, [r7, #12]
  92:	f7ff fffe 	bl	0 <_ZN6Screen18SetWriteablePixelsEssss>
  96:	68f8      	ldr	r0, [r7, #12]
  98:	f7ff fffe 	bl	0 <_ZN6Screen18SetWriteablePixelsEssss>
  9c:	bf00      	nop
  9e:	3718      	adds	r7, #24
  a0:	46bd      	mov	sp, r7
  a2:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen14SetOrientationENS_11OrientationE:

00000000 <_ZN6Screen14SetOrientationENS_11OrientationE>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	460b      	mov	r3, r1
   a:	70fb      	strb	r3, [r7, #3]
   c:	78fb      	ldrb	r3, [r7, #3]
   e:	2b03      	cmp	r3, #3
  10:	d84a      	bhi.n	a8 <_ZN6Screen14SetOrientationENS_11OrientationE+0xa8>
  12:	a201      	add	r2, pc, #4	; (adr r2, 18 <_ZN6Screen14SetOrientationENS_11OrientationE+0x18>)
  14:	f852 f023 	ldr.w	pc, [r2, r3, lsl #2]
  18:	00000029 	.word	0x00000029
  1c:	00000049 	.word	0x00000049
  20:	00000069 	.word	0x00000069
  24:	00000089 	.word	0x00000089
  28:	687b      	ldr	r3, [r7, #4]
  2a:	22f0      	movs	r2, #240	; 0xf0
  2c:	84da      	strh	r2, [r3, #38]	; 0x26
  2e:	687b      	ldr	r3, [r7, #4]
  30:	f44f 72a0 	mov.w	r2, #320	; 0x140
  34:	849a      	strh	r2, [r3, #36]	; 0x24
  36:	2136      	movs	r1, #54	; 0x36
  38:	6878      	ldr	r0, [r7, #4]
  3a:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  3e:	2148      	movs	r1, #72	; 0x48
  40:	6878      	ldr	r0, [r7, #4]
  42:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  46:	e030      	b.n	aa <_ZN6Screen14SetOrientationENS_11OrientationE+0xaa>
  48:	687b      	ldr	r3, [r7, #4]
  4a:	22f0      	movs	r2, #240	; 0xf0
  4c:	84da      	strh	r2, [r3, #38]	; 0x26
  4e:	687b      	ldr	r3, [r7, #4]
  50:	f44f 72a0 	mov.w	r2, #320	; 0x140
  54:	849a      	strh	r2, [r3, #36]	; 0x24
  56:	2136      	movs	r1, #54	; 0x36
  58:	6878      	ldr	r0, [r7, #4]
  5a:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  5e:	2188      	movs	r1, #136	; 0x88
  60:	6878      	ldr	r0, [r7, #4]
  62:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  66:	e020      	b.n	aa <_ZN6Screen14SetOrientationENS_11OrientationE+0xaa>
  68:	687b      	ldr	r3, [r7, #4]
  6a:	f44f 72a0 	mov.w	r2, #320	; 0x140
  6e:	84da      	strh	r2, [r3, #38]	; 0x26
  70:	687b      	ldr	r3, [r7, #4]
  72:	22f0      	movs	r2, #240	; 0xf0
  74:	849a      	strh	r2, [r3, #36]	; 0x24
  76:	2136      	movs	r1, #54	; 0x36
  78:	6878      	ldr	r0, [r7, #4]
  7a:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  7e:	2128      	movs	r1, #40	; 0x28
  80:	6878      	ldr	r0, [r7, #4]
  82:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  86:	e010      	b.n	aa <_ZN6Screen14SetOrientationENS_11OrientationE+0xaa>
  88:	687b      	ldr	r3, [r7, #4]
  8a:	f44f 72a0 	mov.w	r2, #320	; 0x140
  8e:	84da      	strh	r2, [r3, #38]	; 0x26
  90:	687b      	ldr	r3, [r7, #4]
  92:	22f0      	movs	r2, #240	; 0xf0
  94:	849a      	strh	r2, [r3, #36]	; 0x24
  96:	2136      	movs	r1, #54	; 0x36
  98:	6878      	ldr	r0, [r7, #4]
  9a:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  9e:	21e8      	movs	r1, #232	; 0xe8
  a0:	6878      	ldr	r0, [r7, #4]
  a2:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  a6:	e000      	b.n	aa <_ZN6Screen14SetOrientationENS_11OrientationE+0xaa>
  a8:	bf00      	nop
  aa:	687b      	ldr	r3, [r7, #4]
  ac:	8cdb      	ldrh	r3, [r3, #38]	; 0x26
  ae:	005b      	lsls	r3, r3, #1
  b0:	b29a      	uxth	r2, r3
  b2:	687b      	ldr	r3, [r7, #4]
  b4:	851a      	strh	r2, [r3, #40]	; 0x28
  b6:	687b      	ldr	r3, [r7, #4]
  b8:	78fa      	ldrb	r2, [r7, #3]
  ba:	f883 2022 	strb.w	r2, [r3, #34]	; 0x22
  be:	2314      	movs	r3, #20
  c0:	f44f 728c 	mov.w	r2, #280	; 0x118
  c4:	2114      	movs	r1, #20
  c6:	6878      	ldr	r0, [r7, #4]
  c8:	f7ff fffe 	bl	0 <_ZN6Screen14SetOrientationENS_11OrientationE>
  cc:	bf00      	nop
  ce:	3708      	adds	r7, #8
  d0:	46bd      	mov	sp, r7
  d2:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen13FillRectangleEttttt:

00000000 <_ZN6Screen13FillRectangleEttttt>:
   0:	b580      	push	{r7, lr}
   2:	b088      	sub	sp, #32
   4:	af02      	add	r7, sp, #8
   6:	60f8      	str	r0, [r7, #12]
   8:	4608      	mov	r0, r1
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	4603      	mov	r3, r0
  10:	817b      	strh	r3, [r7, #10]
  12:	460b      	mov	r3, r1
  14:	813b      	strh	r3, [r7, #8]
  16:	4613      	mov	r3, r2
  18:	80fb      	strh	r3, [r7, #6]
  1a:	1dbb      	adds	r3, r7, #6
  1c:	f107 0208 	add.w	r2, r7, #8
  20:	f107 010a 	add.w	r1, r7, #10
  24:	f107 0020 	add.w	r0, r7, #32
  28:	9000      	str	r0, [sp, #0]
  2a:	68f8      	ldr	r0, [r7, #12]
  2c:	f7ff fffe 	bl	0 <_ZN6Screen13FillRectangleEttttt>
  30:	68f8      	ldr	r0, [r7, #12]
  32:	f7ff fffe 	bl	0 <_ZN6Screen13FillRectangleEttttt>
  36:	4603      	mov	r3, r0
  38:	617b      	str	r3, [r7, #20]
  3a:	697b      	ldr	r3, [r7, #20]
  3c:	4a0a      	ldr	r2, [pc, #40]	; (68 <_ZN6Screen13FillRectangleEttttt+0x68>)
  3e:	601a      	str	r2, [r3, #0]
  40:	897a      	ldrh	r2, [r7, #10]
  42:	697b      	ldr	r3, [r7, #20]
  44:	80da      	strh	r2, [r3, #6]
  46:	893a      	ldrh	r2, [r7, #8]
  48:	697b      	ldr	r3, [r7, #20]
  4a:	811a      	strh	r2, [r3, #8]
  4c:	88fa      	ldrh	r2, [r7, #6]
  4e:	697b      	ldr	r3, [r7, #20]
  50:	815a      	strh	r2, [r3, #10]
  52:	8c3a      	ldrh	r2, [r7, #32]
  54:	697b      	ldr	r3, [r7, #20]
  56:	819a      	strh	r2, [r3, #12]
  58:	697b      	ldr	r3, [r7, #20]
  5a:	8cba      	ldrh	r2, [r7, #36]	; 0x24
  5c:	81da      	strh	r2, [r3, #14]
  5e:	bf00      	nop
  60:	3718      	adds	r7, #24
  62:	46bd      	mov	sp, r7
  64:	bd80      	pop	{r7, pc}
  66:	bf00      	nop
  68:	00000000 	.word	0x00000000

Disassembly of section .text._ZN6Screen13DrawRectangleEtttttt:

00000000 <_ZN6Screen13DrawRectangleEtttttt>:
   0:	b580      	push	{r7, lr}
   2:	b088      	sub	sp, #32
   4:	af02      	add	r7, sp, #8
   6:	60f8      	str	r0, [r7, #12]
   8:	4608      	mov	r0, r1
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	4603      	mov	r3, r0
  10:	817b      	strh	r3, [r7, #10]
  12:	460b      	mov	r3, r1
  14:	813b      	strh	r3, [r7, #8]
  16:	4613      	mov	r3, r2
  18:	80fb      	strh	r3, [r7, #6]
  1a:	1dbb      	adds	r3, r7, #6
  1c:	f107 0208 	add.w	r2, r7, #8
  20:	f107 010a 	add.w	r1, r7, #10
  24:	f107 0020 	add.w	r0, r7, #32
  28:	9000      	str	r0, [sp, #0]
  2a:	68f8      	ldr	r0, [r7, #12]
  2c:	f7ff fffe 	bl	0 <_ZN6Screen13DrawRectangleEtttttt>
  30:	68f8      	ldr	r0, [r7, #12]
  32:	f7ff fffe 	bl	0 <_ZN6Screen13DrawRectangleEtttttt>
  36:	4603      	mov	r3, r0
  38:	617b      	str	r3, [r7, #20]
  3a:	697b      	ldr	r3, [r7, #20]
  3c:	4a0d      	ldr	r2, [pc, #52]	; (74 <_ZN6Screen13DrawRectangleEtttttt+0x74>)
  3e:	601a      	str	r2, [r3, #0]
  40:	897a      	ldrh	r2, [r7, #10]
  42:	697b      	ldr	r3, [r7, #20]
  44:	80da      	strh	r2, [r3, #6]
  46:	893a      	ldrh	r2, [r7, #8]
  48:	697b      	ldr	r3, [r7, #20]
  4a:	811a      	strh	r2, [r3, #8]
  4c:	88fa      	ldrh	r2, [r7, #6]
  4e:	697b      	ldr	r3, [r7, #20]
  50:	815a      	strh	r2, [r3, #10]
  52:	8c3a      	ldrh	r2, [r7, #32]
  54:	697b      	ldr	r3, [r7, #20]
  56:	819a      	strh	r2, [r3, #12]
  58:	697b      	ldr	r3, [r7, #20]
  5a:	8d3a      	ldrh	r2, [r7, #40]	; 0x28
  5c:	81da      	strh	r2, [r3, #14]
  5e:	8cbb      	ldrh	r3, [r7, #36]	; 0x24
  60:	2202      	movs	r2, #2
  62:	4619      	mov	r1, r3
  64:	6978      	ldr	r0, [r7, #20]
  66:	f7ff fffe 	bl	0 <_ZN6Screen13DrawRectangleEtttttt>
  6a:	bf00      	nop
  6c:	3718      	adds	r7, #24
  6e:	46bd      	mov	sp, r7
  70:	bd80      	pop	{r7, pc}
  72:	bf00      	nop
  74:	00000000 	.word	0x00000000

Disassembly of section .text._ZN6Screen13DrawCharacterEttcRK4Fonttt:

00000000 <_ZN6Screen13DrawCharacterEttcRK4Fonttt>:
   0:	b580      	push	{r7, lr}
   2:	b088      	sub	sp, #32
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	4608      	mov	r0, r1
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	4603      	mov	r3, r0
  10:	817b      	strh	r3, [r7, #10]
  12:	460b      	mov	r3, r1
  14:	813b      	strh	r3, [r7, #8]
  16:	4613      	mov	r3, r2
  18:	71fb      	strb	r3, [r7, #7]
  1a:	6abb      	ldr	r3, [r7, #40]	; 0x28
  1c:	781b      	ldrb	r3, [r3, #0]
  1e:	b29a      	uxth	r2, r3
  20:	897b      	ldrh	r3, [r7, #10]
  22:	4413      	add	r3, r2
  24:	83fb      	strh	r3, [r7, #30]
  26:	6abb      	ldr	r3, [r7, #40]	; 0x28
  28:	785b      	ldrb	r3, [r3, #1]
  2a:	b29a      	uxth	r2, r3
  2c:	893b      	ldrh	r3, [r7, #8]
  2e:	4413      	add	r3, r2
  30:	83bb      	strh	r3, [r7, #28]
  32:	79fb      	ldrb	r3, [r7, #7]
  34:	3b20      	subs	r3, #32
  36:	b29a      	uxth	r2, r3
  38:	6abb      	ldr	r3, [r7, #40]	; 0x28
  3a:	785b      	ldrb	r3, [r3, #1]
  3c:	b29b      	uxth	r3, r3
  3e:	fb12 f303 	smulbb	r3, r2, r3
  42:	b29a      	uxth	r2, r3
  44:	6abb      	ldr	r3, [r7, #40]	; 0x28
  46:	781b      	ldrb	r3, [r3, #0]
  48:	08db      	lsrs	r3, r3, #3
  4a:	b2db      	uxtb	r3, r3
  4c:	3301      	adds	r3, #1
  4e:	b29b      	uxth	r3, r3
  50:	fb12 f303 	smulbb	r3, r2, r3
  54:	837b      	strh	r3, [r7, #26]
  56:	6abb      	ldr	r3, [r7, #40]	; 0x28
  58:	685a      	ldr	r2, [r3, #4]
  5a:	8b7b      	ldrh	r3, [r7, #26]
  5c:	4413      	add	r3, r2
  5e:	617b      	str	r3, [r7, #20]
  60:	68f8      	ldr	r0, [r7, #12]
  62:	f7ff fffe 	bl	0 <_ZN6Screen13DrawCharacterEttcRK4Fonttt>
  66:	4603      	mov	r3, r0
  68:	613b      	str	r3, [r7, #16]
  6a:	693b      	ldr	r3, [r7, #16]
  6c:	4a0f      	ldr	r2, [pc, #60]	; (ac <_ZN6Screen13DrawCharacterEttcRK4Fonttt+0xac>)
  6e:	601a      	str	r2, [r3, #0]
  70:	693b      	ldr	r3, [r7, #16]
  72:	897a      	ldrh	r2, [r7, #10]
  74:	80da      	strh	r2, [r3, #6]
  76:	693b      	ldr	r3, [r7, #16]
  78:	893a      	ldrh	r2, [r7, #8]
  7a:	815a      	strh	r2, [r3, #10]
  7c:	693b      	ldr	r3, [r7, #16]
  7e:	8bfa      	ldrh	r2, [r7, #30]
  80:	811a      	strh	r2, [r3, #8]
  82:	693b      	ldr	r3, [r7, #16]
  84:	8bba      	ldrh	r2, [r7, #28]
  86:	819a      	strh	r2, [r3, #12]
  88:	693b      	ldr	r3, [r7, #16]
  8a:	8dba      	ldrh	r2, [r7, #44]	; 0x2c
  8c:	81da      	strh	r2, [r3, #14]
  8e:	8e3b      	ldrh	r3, [r7, #48]	; 0x30
  90:	2202      	movs	r2, #2
  92:	4619      	mov	r1, r3
  94:	6938      	ldr	r0, [r7, #16]
  96:	f7ff fffe 	bl	0 <_ZN6Screen13DrawCharacterEttcRK4Fonttt>
  9a:	2204      	movs	r2, #4
  9c:	6979      	ldr	r1, [r7, #20]
  9e:	6938      	ldr	r0, [r7, #16]
  a0:	f7ff fffe 	bl	0 <_ZN6Screen13DrawCharacterEttcRK4Fonttt>
  a4:	bf00      	nop
  a6:	3720      	adds	r7, #32
  a8:	46bd      	mov	sp, r7
  aa:	bd80      	pop	{r7, pc}
  ac:	00000000 	.word	0x00000000

Disassembly of section .text._ZN6Screen10DrawStringEttPKctRK4Fonttt:

00000000 <_ZN6Screen10DrawStringEttPKctRK4Fonttt>:
   0:	b580      	push	{r7, lr}
   2:	b088      	sub	sp, #32
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	607b      	str	r3, [r7, #4]
   a:	460b      	mov	r3, r1
   c:	817b      	strh	r3, [r7, #10]
   e:	4613      	mov	r3, r2
  10:	813b      	strh	r3, [r7, #8]
  12:	897a      	ldrh	r2, [r7, #10]
  14:	8d3b      	ldrh	r3, [r7, #40]	; 0x28
  16:	6af9      	ldr	r1, [r7, #44]	; 0x2c
  18:	7809      	ldrb	r1, [r1, #0]
  1a:	fb01 f303 	mul.w	r3, r1, r3
  1e:	4413      	add	r3, r2
  20:	2bf0      	cmp	r3, #240	; 0xf0
  22:	dd08      	ble.n	36 <_ZN6Screen10DrawStringEttPKctRK4Fonttt+0x36>
  24:	897b      	ldrh	r3, [r7, #10]
  26:	f1c3 03f0 	rsb	r3, r3, #240	; 0xf0
  2a:	6afa      	ldr	r2, [r7, #44]	; 0x2c
  2c:	7812      	ldrb	r2, [r2, #0]
  2e:	fb93 f3f2 	sdiv	r3, r3, r2
  32:	b29b      	uxth	r3, r3
  34:	e000      	b.n	38 <_ZN6Screen10DrawStringEttPKctRK4Fonttt+0x38>
  36:	8d3b      	ldrh	r3, [r7, #40]	; 0x28
  38:	83fb      	strh	r3, [r7, #30]
  3a:	6afb      	ldr	r3, [r7, #44]	; 0x2c
  3c:	781b      	ldrb	r3, [r3, #0]
  3e:	b29b      	uxth	r3, r3
  40:	8bfa      	ldrh	r2, [r7, #30]
  42:	fb12 f303 	smulbb	r3, r2, r3
  46:	83bb      	strh	r3, [r7, #28]
  48:	897a      	ldrh	r2, [r7, #10]
  4a:	8bbb      	ldrh	r3, [r7, #28]
  4c:	4413      	add	r3, r2
  4e:	837b      	strh	r3, [r7, #26]
  50:	6afb      	ldr	r3, [r7, #44]	; 0x2c
  52:	785b      	ldrb	r3, [r3, #1]
  54:	b29a      	uxth	r2, r3
  56:	893b      	ldrh	r3, [r7, #8]
  58:	4413      	add	r3, r2
  5a:	833b      	strh	r3, [r7, #24]
  5c:	68f8      	ldr	r0, [r7, #12]
  5e:	f7ff fffe 	bl	0 <_ZN6Screen10DrawStringEttPKctRK4Fonttt>
  62:	4603      	mov	r3, r0
  64:	617b      	str	r3, [r7, #20]
  66:	697b      	ldr	r3, [r7, #20]
  68:	4a1a      	ldr	r2, [pc, #104]	; (d4 <_ZN6Screen10DrawStringEttPKctRK4Fonttt+0xd4>)
  6a:	601a      	str	r2, [r3, #0]
  6c:	697b      	ldr	r3, [r7, #20]
  6e:	897a      	ldrh	r2, [r7, #10]
  70:	80da      	strh	r2, [r3, #6]
  72:	697b      	ldr	r3, [r7, #20]
  74:	8b7a      	ldrh	r2, [r7, #26]
  76:	811a      	strh	r2, [r3, #8]
  78:	697b      	ldr	r3, [r7, #20]
  7a:	893a      	ldrh	r2, [r7, #8]
  7c:	815a      	strh	r2, [r3, #10]
  7e:	697b      	ldr	r3, [r7, #20]
  80:	8b3a      	ldrh	r2, [r7, #24]
  82:	819a      	strh	r2, [r3, #12]
  84:	697b      	ldr	r3, [r7, #20]
  86:	8e3a      	ldrh	r2, [r7, #48]	; 0x30
  88:	81da      	strh	r2, [r3, #14]
  8a:	8ebb      	ldrh	r3, [r7, #52]	; 0x34
  8c:	2202      	movs	r2, #2
  8e:	4619      	mov	r1, r3
  90:	6978      	ldr	r0, [r7, #20]
  92:	f7ff fffe 	bl	0 <_ZN6Screen10DrawStringEttPKctRK4Fonttt>
  96:	687b      	ldr	r3, [r7, #4]
  98:	2204      	movs	r2, #4
  9a:	4619      	mov	r1, r3
  9c:	6978      	ldr	r0, [r7, #20]
  9e:	f7ff fffe 	bl	0 <_ZN6Screen10DrawStringEttPKctRK4Fonttt>
  a2:	6afb      	ldr	r3, [r7, #44]	; 0x2c
  a4:	781b      	ldrb	r3, [r3, #0]
  a6:	2201      	movs	r2, #1
  a8:	4619      	mov	r1, r3
  aa:	6978      	ldr	r0, [r7, #20]
  ac:	f7ff fffe 	bl	0 <_ZN6Screen10DrawStringEttPKctRK4Fonttt>
  b0:	6afb      	ldr	r3, [r7, #44]	; 0x2c
  b2:	785b      	ldrb	r3, [r3, #1]
  b4:	2201      	movs	r2, #1
  b6:	4619      	mov	r1, r3
  b8:	6978      	ldr	r0, [r7, #20]
  ba:	f7ff fffe 	bl	0 <_ZN6Screen10DrawStringEttPKctRK4Fonttt>
  be:	6afb      	ldr	r3, [r7, #44]	; 0x2c
  c0:	685b      	ldr	r3, [r3, #4]
  c2:	2204      	movs	r2, #4
  c4:	4619      	mov	r1, r3
  c6:	6978      	ldr	r0, [r7, #20]
  c8:	f7ff fffe 	bl	0 <_ZN6Screen10DrawStringEttPKctRK4Fonttt>
  cc:	bf00      	nop
  ce:	3720      	adds	r7, #32
  d0:	46bd      	mov	sp, r7
  d2:	bd80      	pop	{r7, pc}
  d4:	00000000 	.word	0x00000000

Disassembly of section .text._ZN6Screen16DefineScrollAreaEttt:

00000000 <_ZN6Screen16DefineScrollAreaEttt>:
   0:	b580      	push	{r7, lr}
   2:	b088      	sub	sp, #32
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	4608      	mov	r0, r1
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	4603      	mov	r3, r0
  10:	817b      	strh	r3, [r7, #10]
  12:	460b      	mov	r3, r1
  14:	813b      	strh	r3, [r7, #8]
  16:	4613      	mov	r3, r2
  18:	80fb      	strh	r3, [r7, #6]
  1a:	68fb      	ldr	r3, [r7, #12]
  1c:	f893 3022 	ldrb.w	r3, [r3, #34]	; 0x22
  20:	2b03      	cmp	r3, #3
  22:	d853      	bhi.n	cc <_ZN6Screen16DefineScrollAreaEttt+0xcc>
  24:	a201      	add	r2, pc, #4	; (adr r2, 2c <_ZN6Screen16DefineScrollAreaEttt+0x2c>)
  26:	f852 f023 	ldr.w	pc, [r2, r3, lsl #2]
  2a:	bf00      	nop
  2c:	0000003d 	.word	0x0000003d
  30:	00000085 	.word	0x00000085
  34:	0000003d 	.word	0x0000003d
  38:	00000085 	.word	0x00000085
  3c:	897b      	ldrh	r3, [r7, #10]
  3e:	0a1b      	lsrs	r3, r3, #8
  40:	b29b      	uxth	r3, r3
  42:	b2db      	uxtb	r3, r3
  44:	763b      	strb	r3, [r7, #24]
  46:	897b      	ldrh	r3, [r7, #10]
  48:	b2db      	uxtb	r3, r3
  4a:	767b      	strb	r3, [r7, #25]
  4c:	893b      	ldrh	r3, [r7, #8]
  4e:	0a1b      	lsrs	r3, r3, #8
  50:	b29b      	uxth	r3, r3
  52:	b2db      	uxtb	r3, r3
  54:	76bb      	strb	r3, [r7, #26]
  56:	893b      	ldrh	r3, [r7, #8]
  58:	b2db      	uxtb	r3, r3
  5a:	76fb      	strb	r3, [r7, #27]
  5c:	88fb      	ldrh	r3, [r7, #6]
  5e:	0a1b      	lsrs	r3, r3, #8
  60:	b29b      	uxth	r3, r3
  62:	b2db      	uxtb	r3, r3
  64:	773b      	strb	r3, [r7, #28]
  66:	88fb      	ldrh	r3, [r7, #6]
  68:	b2db      	uxtb	r3, r3
  6a:	777b      	strb	r3, [r7, #29]
  6c:	2133      	movs	r1, #51	; 0x33
  6e:	68f8      	ldr	r0, [r7, #12]
  70:	f7ff fffe 	bl	0 <_ZN6Screen16DefineScrollAreaEttt>
  74:	f107 0318 	add.w	r3, r7, #24
  78:	2206      	movs	r2, #6
  7a:	4619      	mov	r1, r3
  7c:	68f8      	ldr	r0, [r7, #12]
  7e:	f7ff fffe 	bl	0 <_ZN6Screen16DefineScrollAreaEttt>
  82:	e024      	b.n	ce <_ZN6Screen16DefineScrollAreaEttt+0xce>
  84:	88fb      	ldrh	r3, [r7, #6]
  86:	0a1b      	lsrs	r3, r3, #8
  88:	b29b      	uxth	r3, r3
  8a:	b2db      	uxtb	r3, r3
  8c:	743b      	strb	r3, [r7, #16]
  8e:	88fb      	ldrh	r3, [r7, #6]
  90:	b2db      	uxtb	r3, r3
  92:	747b      	strb	r3, [r7, #17]
  94:	893b      	ldrh	r3, [r7, #8]
  96:	0a1b      	lsrs	r3, r3, #8
  98:	b29b      	uxth	r3, r3
  9a:	b2db      	uxtb	r3, r3
  9c:	74bb      	strb	r3, [r7, #18]
  9e:	893b      	ldrh	r3, [r7, #8]
  a0:	b2db      	uxtb	r3, r3
  a2:	74fb      	strb	r3, [r7, #19]
  a4:	897b      	ldrh	r3, [r7, #10]
  a6:	0a1b      	lsrs	r3, r3, #8
  a8:	b29b      	uxth	r3, r3
  aa:	b2db      	uxtb	r3, r3
  ac:	753b      	strb	r3, [r7, #20]
  ae:	897b      	ldrh	r3, [r7, #10]
  b0:	b2db      	uxtb	r3, r3
  b2:	757b      	strb	r3, [r7, #21]
  b4:	2133      	movs	r1, #51	; 0x33
  b6:	68f8      	ldr	r0, [r7, #12]
  b8:	f7ff fffe 	bl	0 <_ZN6Screen16DefineScrollAreaEttt>
  bc:	f107 0310 	add.w	r3, r7, #16
  c0:	2206      	movs	r2, #6
  c2:	4619      	mov	r1, r3
  c4:	68f8      	ldr	r0, [r7, #12]
  c6:	f7ff fffe 	bl	0 <_ZN6Screen16DefineScrollAreaEttt>
  ca:	e000      	b.n	ce <_ZN6Screen16DefineScrollAreaEttt+0xce>
  cc:	bf00      	nop
  ce:	3720      	adds	r7, #32
  d0:	46bd      	mov	sp, r7
  d2:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen12ScrollScreenEtb:

00000000 <_ZN6Screen12ScrollScreenEtb>:
   0:	b580      	push	{r7, lr}
   2:	b084      	sub	sp, #16
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	460b      	mov	r3, r1
   a:	807b      	strh	r3, [r7, #2]
   c:	4613      	mov	r3, r2
   e:	707b      	strb	r3, [r7, #1]
  10:	687b      	ldr	r3, [r7, #4]
  12:	f893 3022 	ldrb.w	r3, [r3, #34]	; 0x22
  16:	2b03      	cmp	r3, #3
  18:	d828      	bhi.n	6c <_ZN6Screen12ScrollScreenEtb+0x6c>
  1a:	a201      	add	r2, pc, #4	; (adr r2, 20 <_ZN6Screen12ScrollScreenEtb+0x20>)
  1c:	f852 f023 	ldr.w	pc, [r2, r3, lsl #2]
  20:	00000031 	.word	0x00000031
  24:	0000004f 	.word	0x0000004f
  28:	00000031 	.word	0x00000031
  2c:	0000004f 	.word	0x0000004f
  30:	787b      	ldrb	r3, [r7, #1]
  32:	2b00      	cmp	r3, #0
  34:	d003      	beq.n	3e <_ZN6Screen12ScrollScreenEtb+0x3e>
  36:	4a1b      	ldr	r2, [pc, #108]	; (a4 <_ZN6Screen12ScrollScreenEtb+0xa4>)
  38:	887b      	ldrh	r3, [r7, #2]
  3a:	8013      	strh	r3, [r2, #0]
  3c:	e01a      	b.n	74 <_ZN6Screen12ScrollScreenEtb+0x74>
  3e:	687b      	ldr	r3, [r7, #4]
  40:	8c9a      	ldrh	r2, [r3, #36]	; 0x24
  42:	887b      	ldrh	r3, [r7, #2]
  44:	1ad3      	subs	r3, r2, r3
  46:	b29a      	uxth	r2, r3
  48:	4b16      	ldr	r3, [pc, #88]	; (a4 <_ZN6Screen12ScrollScreenEtb+0xa4>)
  4a:	801a      	strh	r2, [r3, #0]
  4c:	e012      	b.n	74 <_ZN6Screen12ScrollScreenEtb+0x74>
  4e:	787b      	ldrb	r3, [r7, #1]
  50:	2b00      	cmp	r3, #0
  52:	d007      	beq.n	64 <_ZN6Screen12ScrollScreenEtb+0x64>
  54:	687b      	ldr	r3, [r7, #4]
  56:	8c9a      	ldrh	r2, [r3, #36]	; 0x24
  58:	887b      	ldrh	r3, [r7, #2]
  5a:	1ad3      	subs	r3, r2, r3
  5c:	b29a      	uxth	r2, r3
  5e:	4b11      	ldr	r3, [pc, #68]	; (a4 <_ZN6Screen12ScrollScreenEtb+0xa4>)
  60:	801a      	strh	r2, [r3, #0]
  62:	e007      	b.n	74 <_ZN6Screen12ScrollScreenEtb+0x74>
  64:	4a0f      	ldr	r2, [pc, #60]	; (a4 <_ZN6Screen12ScrollScreenEtb+0xa4>)
  66:	887b      	ldrh	r3, [r7, #2]
  68:	8013      	strh	r3, [r2, #0]
  6a:	e003      	b.n	74 <_ZN6Screen12ScrollScreenEtb+0x74>
  6c:	4b0d      	ldr	r3, [pc, #52]	; (a4 <_ZN6Screen12ScrollScreenEtb+0xa4>)
  6e:	2200      	movs	r2, #0
  70:	801a      	strh	r2, [r3, #0]
  72:	e014      	b.n	9e <_ZN6Screen12ScrollScreenEtb+0x9e>
  74:	4b0b      	ldr	r3, [pc, #44]	; (a4 <_ZN6Screen12ScrollScreenEtb+0xa4>)
  76:	881b      	ldrh	r3, [r3, #0]
  78:	0a1b      	lsrs	r3, r3, #8
  7a:	b29b      	uxth	r3, r3
  7c:	b2db      	uxtb	r3, r3
  7e:	733b      	strb	r3, [r7, #12]
  80:	4b08      	ldr	r3, [pc, #32]	; (a4 <_ZN6Screen12ScrollScreenEtb+0xa4>)
  82:	881b      	ldrh	r3, [r3, #0]
  84:	b2db      	uxtb	r3, r3
  86:	737b      	strb	r3, [r7, #13]
  88:	2137      	movs	r1, #55	; 0x37
  8a:	6878      	ldr	r0, [r7, #4]
  8c:	f7ff fffe 	bl	0 <_ZN6Screen12ScrollScreenEtb>
  90:	f107 030c 	add.w	r3, r7, #12
  94:	2202      	movs	r2, #2
  96:	4619      	mov	r1, r3
  98:	6878      	ldr	r0, [r7, #4]
  9a:	f7ff fffe 	bl	0 <_ZN6Screen12ScrollScreenEtb>
  9e:	3710      	adds	r7, #16
  a0:	46bd      	mov	sp, r7
  a2:	bd80      	pop	{r7, pc}
  a4:	00000000 	.word	0x00000000

Disassembly of section .text._ZN6Screen10AppendTextEPKcm:

00000000 <_ZN6Screen10AppendTextEPKcm>:
   0:	b490      	push	{r4, r7}
   2:	b088      	sub	sp, #32
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	607a      	str	r2, [r7, #4]
   c:	687b      	ldr	r3, [r7, #4]
   e:	2b30      	cmp	r3, #48	; 0x30
  10:	bf28      	it	cs
  12:	2330      	movcs	r3, #48	; 0x30
  14:	61bb      	str	r3, [r7, #24]
  16:	68fb      	ldr	r3, [r7, #12]
  18:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  1c:	f8d3 2d34 	ldr.w	r2, [r3, #3380]	; 0xd34
  20:	4613      	mov	r3, r2
  22:	005b      	lsls	r3, r3, #1
  24:	4413      	add	r3, r2
  26:	011b      	lsls	r3, r3, #4
  28:	f503 53ea 	add.w	r3, r3, #7488	; 0x1d40
  2c:	68fa      	ldr	r2, [r7, #12]
  2e:	4413      	add	r3, r2
  30:	617b      	str	r3, [r7, #20]
  32:	68fb      	ldr	r3, [r7, #12]
  34:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  38:	f8d3 3d34 	ldr.w	r3, [r3, #3380]	; 0xd34
  3c:	f503 530f 	add.w	r3, r3, #9152	; 0x23c0
  40:	3310      	adds	r3, #16
  42:	68fa      	ldr	r2, [r7, #12]
  44:	4413      	add	r3, r2
  46:	613b      	str	r3, [r7, #16]
  48:	2300      	movs	r3, #0
  4a:	61fb      	str	r3, [r7, #28]
  4c:	693b      	ldr	r3, [r7, #16]
  4e:	781b      	ldrb	r3, [r3, #0]
  50:	2b2f      	cmp	r3, #47	; 0x2f
  52:	d814      	bhi.n	7e <_ZN6Screen10AppendTextEPKcm+0x7e>
  54:	69fa      	ldr	r2, [r7, #28]
  56:	69bb      	ldr	r3, [r7, #24]
  58:	429a      	cmp	r2, r3
  5a:	d210      	bcs.n	7e <_ZN6Screen10AppendTextEPKcm+0x7e>
  5c:	68ba      	ldr	r2, [r7, #8]
  5e:	69fb      	ldr	r3, [r7, #28]
  60:	1c59      	adds	r1, r3, #1
  62:	61f9      	str	r1, [r7, #28]
  64:	4413      	add	r3, r2
  66:	7818      	ldrb	r0, [r3, #0]
  68:	697a      	ldr	r2, [r7, #20]
  6a:	693b      	ldr	r3, [r7, #16]
  6c:	781b      	ldrb	r3, [r3, #0]
  6e:	1c59      	adds	r1, r3, #1
  70:	b2cc      	uxtb	r4, r1
  72:	6939      	ldr	r1, [r7, #16]
  74:	700c      	strb	r4, [r1, #0]
  76:	4413      	add	r3, r2
  78:	4602      	mov	r2, r0
  7a:	701a      	strb	r2, [r3, #0]
  7c:	e7e6      	b.n	4c <_ZN6Screen10AppendTextEPKcm+0x4c>
  7e:	bf00      	nop
  80:	3720      	adds	r7, #32
  82:	46bd      	mov	sp, r7
  84:	bc90      	pop	{r4, r7}
  86:	4770      	bx	lr

Disassembly of section .text._ZN6Screen10CommitTextEv:

00000000 <_ZN6Screen10CommitTextEv>:
   0:	b580      	push	{r7, lr}
   2:	b088      	sub	sp, #32
   4:	af04      	add	r7, sp, #16
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
   e:	f8d3 3d34 	ldr.w	r3, [r3, #3380]	; 0xd34
  12:	81fb      	strh	r3, [r7, #14]
  14:	687b      	ldr	r3, [r7, #4]
  16:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  1a:	f8d3 3d34 	ldr.w	r3, [r3, #3380]	; 0xd34
  1e:	3301      	adds	r3, #1
  20:	687a      	ldr	r2, [r7, #4]
  22:	f502 5280 	add.w	r2, r2, #4096	; 0x1000
  26:	f8c2 3d34 	str.w	r3, [r2, #3380]	; 0xd34
  2a:	687b      	ldr	r3, [r7, #4]
  2c:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  30:	f8d3 3d34 	ldr.w	r3, [r3, #3380]	; 0xd34
  34:	2b22      	cmp	r3, #34	; 0x22
  36:	bf8c      	ite	hi
  38:	2301      	movhi	r3, #1
  3a:	2300      	movls	r3, #0
  3c:	b2db      	uxtb	r3, r3
  3e:	2b00      	cmp	r3, #0
  40:	d006      	beq.n	50 <_ZN6Screen10CommitTextEv+0x50>
  42:	687b      	ldr	r3, [r7, #4]
  44:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  48:	461a      	mov	r2, r3
  4a:	2300      	movs	r3, #0
  4c:	f8c2 3d34 	str.w	r3, [r2, #3380]	; 0xd34
  50:	4b23      	ldr	r3, [pc, #140]	; (e0 <_ZN6Screen10CommitTextEv+0xe0>)
  52:	785b      	ldrb	r3, [r3, #1]
  54:	b29a      	uxth	r2, r3
  56:	687b      	ldr	r3, [r7, #4]
  58:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  5c:	f8d3 3d38 	ldr.w	r3, [r3, #3384]	; 0xd38
  60:	b29b      	uxth	r3, r3
  62:	fb12 f303 	smulbb	r3, r2, r3
  66:	b29b      	uxth	r3, r3
  68:	3314      	adds	r3, #20
  6a:	b29a      	uxth	r2, r3
  6c:	4b1d      	ldr	r3, [pc, #116]	; (e4 <_ZN6Screen10CommitTextEv+0xe4>)
  6e:	801a      	strh	r2, [r3, #0]
  70:	687b      	ldr	r3, [r7, #4]
  72:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  76:	f8d3 3d38 	ldr.w	r3, [r3, #3384]	; 0xd38
  7a:	2b22      	cmp	r3, #34	; 0x22
  7c:	d82c      	bhi.n	d8 <_ZN6Screen10CommitTextEv+0xd8>
  7e:	687b      	ldr	r3, [r7, #4]
  80:	f503 5380 	add.w	r3, r3, #4096	; 0x1000
  84:	f8d3 3d38 	ldr.w	r3, [r3, #3384]	; 0xd38
  88:	3301      	adds	r3, #1
  8a:	687a      	ldr	r2, [r7, #4]
  8c:	f502 5280 	add.w	r2, r2, #4096	; 0x1000
  90:	f8c2 3d38 	str.w	r3, [r2, #3384]	; 0xd38
  94:	4b13      	ldr	r3, [pc, #76]	; (e4 <_ZN6Screen10CommitTextEv+0xe4>)
  96:	8819      	ldrh	r1, [r3, #0]
  98:	89fa      	ldrh	r2, [r7, #14]
  9a:	4613      	mov	r3, r2
  9c:	005b      	lsls	r3, r3, #1
  9e:	4413      	add	r3, r2
  a0:	011b      	lsls	r3, r3, #4
  a2:	f503 53ea 	add.w	r3, r3, #7488	; 0x1d40
  a6:	687a      	ldr	r2, [r7, #4]
  a8:	18d0      	adds	r0, r2, r3
  aa:	89fb      	ldrh	r3, [r7, #14]
  ac:	687a      	ldr	r2, [r7, #4]
  ae:	4413      	add	r3, r2
  b0:	f503 530f 	add.w	r3, r3, #9152	; 0x23c0
  b4:	3310      	adds	r3, #16
  b6:	781b      	ldrb	r3, [r3, #0]
  b8:	b29b      	uxth	r3, r3
  ba:	2200      	movs	r2, #0
  bc:	9203      	str	r2, [sp, #12]
  be:	f64f 72ff 	movw	r2, #65535	; 0xffff
  c2:	9202      	str	r2, [sp, #8]
  c4:	4a06      	ldr	r2, [pc, #24]	; (e0 <_ZN6Screen10CommitTextEv+0xe0>)
  c6:	9201      	str	r2, [sp, #4]
  c8:	9300      	str	r3, [sp, #0]
  ca:	4603      	mov	r3, r0
  cc:	460a      	mov	r2, r1
  ce:	2100      	movs	r1, #0
  d0:	6878      	ldr	r0, [r7, #4]
  d2:	f7ff fffe 	bl	0 <_ZN6Screen10CommitTextEv>
  d6:	e000      	b.n	da <_ZN6Screen10CommitTextEv+0xda>
  d8:	bf00      	nop
  da:	3710      	adds	r7, #16
  dc:	46bd      	mov	sp, r7
  de:	bd80      	pop	{r7, pc}
	...

Disassembly of section .text._ZN6Screen18WaitForSPICompleteEv:

00000000 <_ZN6Screen18WaitForSPICompleteEv>:
   0:	b480      	push	{r7}
   2:	b083      	sub	sp, #12
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	681b      	ldr	r3, [r3, #0]
   c:	f893 3051 	ldrb.w	r3, [r3, #81]	; 0x51
  10:	b2db      	uxtb	r3, r3
  12:	2b01      	cmp	r3, #1
  14:	bf14      	ite	ne
  16:	2301      	movne	r3, #1
  18:	2300      	moveq	r3, #0
  1a:	b2db      	uxtb	r3, r3
  1c:	2b00      	cmp	r3, #0
  1e:	d001      	beq.n	24 <_ZN6Screen18WaitForSPICompleteEv+0x24>
  20:	bf00      	nop
  22:	e7f1      	b.n	8 <_ZN6Screen18WaitForSPICompleteEv+0x8>
  24:	bf00      	nop
  26:	370c      	adds	r7, #12
  28:	46bd      	mov	sp, r7
  2a:	f85d 7b04 	ldr.w	r7, [sp], #4
  2e:	4770      	bx	lr

Disassembly of section .text._ZN6Screen15SetPinToCommandEv:

00000000 <_ZN6Screen15SetPinToCommandEv>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	68d8      	ldr	r0, [r3, #12]
   c:	687b      	ldr	r3, [r7, #4]
   e:	8a1b      	ldrh	r3, [r3, #16]
  10:	2200      	movs	r2, #0
  12:	4619      	mov	r1, r3
  14:	f7ff fffe 	bl	0 <HAL_GPIO_WritePin>
  18:	bf00      	nop
  1a:	3708      	adds	r7, #8
  1c:	46bd      	mov	sp, r7
  1e:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen12SetPinToDataEv:

00000000 <_ZN6Screen12SetPinToDataEv>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	68d8      	ldr	r0, [r3, #12]
   c:	687b      	ldr	r3, [r7, #4]
   e:	8a1b      	ldrh	r3, [r3, #16]
  10:	2201      	movs	r2, #1
  12:	4619      	mov	r1, r3
  14:	f7ff fffe 	bl	0 <HAL_GPIO_WritePin>
  18:	bf00      	nop
  1a:	3708      	adds	r7, #8
  1c:	46bd      	mov	sp, r7
  1e:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen12WriteCommandEh:

00000000 <_ZN6Screen12WriteCommandEh>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	460b      	mov	r3, r1
   a:	70fb      	strb	r3, [r7, #3]
   c:	6878      	ldr	r0, [r7, #4]
   e:	f7ff fffe 	bl	0 <_ZN6Screen12WriteCommandEh>
  12:	6878      	ldr	r0, [r7, #4]
  14:	f7ff fffe 	bl	0 <_ZN6Screen12WriteCommandEh>
  18:	687b      	ldr	r3, [r7, #4]
  1a:	6818      	ldr	r0, [r3, #0]
  1c:	1cf9      	adds	r1, r7, #3
  1e:	f04f 33ff 	mov.w	r3, #4294967295	; 0xffffffff
  22:	2201      	movs	r2, #1
  24:	f7ff fffe 	bl	0 <HAL_SPI_Transmit>
  28:	bf00      	nop
  2a:	3708      	adds	r7, #8
  2c:	46bd      	mov	sp, r7
  2e:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen9WriteDataEPhm:

00000000 <_ZN6Screen9WriteDataEPhm>:
   0:	b580      	push	{r7, lr}
   2:	b084      	sub	sp, #16
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	607a      	str	r2, [r7, #4]
   c:	68f8      	ldr	r0, [r7, #12]
   e:	f7ff fffe 	bl	0 <_ZN6Screen9WriteDataEPhm>
  12:	68fb      	ldr	r3, [r7, #12]
  14:	6818      	ldr	r0, [r3, #0]
  16:	687b      	ldr	r3, [r7, #4]
  18:	b29a      	uxth	r2, r3
  1a:	f04f 33ff 	mov.w	r3, #4294967295	; 0xffffffff
  1e:	68b9      	ldr	r1, [r7, #8]
  20:	f7ff fffe 	bl	0 <HAL_SPI_Transmit>
  24:	bf00      	nop
  26:	3710      	adds	r7, #16
  28:	46bd      	mov	sp, r7
  2a:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen14WriteDataAsyncEPhm:

00000000 <_ZN6Screen14WriteDataAsyncEPhm>:
   0:	b580      	push	{r7, lr}
   2:	b084      	sub	sp, #16
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	607a      	str	r2, [r7, #4]
   c:	68f8      	ldr	r0, [r7, #12]
   e:	f7ff fffe 	bl	0 <_ZN6Screen14WriteDataAsyncEPhm>
  12:	68fb      	ldr	r3, [r7, #12]
  14:	681b      	ldr	r3, [r3, #0]
  16:	687a      	ldr	r2, [r7, #4]
  18:	b292      	uxth	r2, r2
  1a:	68b9      	ldr	r1, [r7, #8]
  1c:	4618      	mov	r0, r3
  1e:	f7ff fffe 	bl	0 <HAL_SPI_Transmit_DMA>
  22:	bf00      	nop
  24:	3710      	adds	r7, #16
  26:	46bd      	mov	sp, r7
  28:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen16WriteDataWithSetEh:

00000000 <_ZN6Screen16WriteDataWithSetEh>:
   0:	b580      	push	{r7, lr}
   2:	b082      	sub	sp, #8
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	460b      	mov	r3, r1
   a:	70fb      	strb	r3, [r7, #3]
   c:	6878      	ldr	r0, [r7, #4]
   e:	f7ff fffe 	bl	0 <_ZN6Screen16WriteDataWithSetEh>
  12:	6878      	ldr	r0, [r7, #4]
  14:	f7ff fffe 	bl	0 <_ZN6Screen16WriteDataWithSetEh>
  18:	687b      	ldr	r3, [r7, #4]
  1a:	6818      	ldr	r0, [r3, #0]
  1c:	1cf9      	adds	r1, r7, #3
  1e:	f04f 33ff 	mov.w	r3, #4294967295	; 0xffffffff
  22:	2201      	movs	r2, #1
  24:	f7ff fffe 	bl	0 <HAL_SPI_Transmit>
  28:	bf00      	nop
  2a:	3708      	adds	r7, #8
  2c:	46bd      	mov	sp, r7
  2e:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen16WriteDataWithSetEPhm:

00000000 <_ZN6Screen16WriteDataWithSetEPhm>:
   0:	b580      	push	{r7, lr}
   2:	b084      	sub	sp, #16
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	607a      	str	r2, [r7, #4]
   c:	68f8      	ldr	r0, [r7, #12]
   e:	f7ff fffe 	bl	0 <_ZN6Screen16WriteDataWithSetEPhm>
  12:	68f8      	ldr	r0, [r7, #12]
  14:	f7ff fffe 	bl	0 <_ZN6Screen16WriteDataWithSetEPhm>
  18:	68fb      	ldr	r3, [r7, #12]
  1a:	6818      	ldr	r0, [r3, #0]
  1c:	687b      	ldr	r3, [r7, #4]
  1e:	b29a      	uxth	r2, r3
  20:	f04f 33ff 	mov.w	r3, #4294967295	; 0xffffffff
  24:	68b9      	ldr	r1, [r7, #8]
  26:	f7ff fffe 	bl	0 <HAL_SPI_Transmit>
  2a:	bf00      	nop
  2c:	3710      	adds	r7, #16
  2e:	46bd      	mov	sp, r7
  30:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen14RetrieveMemoryEv:

00000000 <_ZN6Screen14RetrieveMemoryEv>:
   0:	b580      	push	{r7, lr}
   2:	b084      	sub	sp, #16
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	687b      	ldr	r3, [r7, #4]
   a:	f8d3 3a54 	ldr.w	r3, [r3, #2644]	; 0xa54
   e:	2b31      	cmp	r3, #49	; 0x31
  10:	d901      	bls.n	16 <_ZN6Screen14RetrieveMemoryEv+0x16>
  12:	f7ff fffe 	bl	0 <Error_Handler>
  16:	687b      	ldr	r3, [r7, #4]
  18:	f8d3 3a58 	ldr.w	r3, [r3, #2648]	; 0xa58
  1c:	2b31      	cmp	r3, #49	; 0x31
  1e:	d903      	bls.n	28 <_ZN6Screen14RetrieveMemoryEv+0x28>
  20:	687b      	ldr	r3, [r7, #4]
  22:	2200      	movs	r2, #0
  24:	f8c3 2a58 	str.w	r2, [r3, #2648]	; 0xa58
  28:	687b      	ldr	r3, [r7, #4]
  2a:	f8d3 3a58 	ldr.w	r3, [r3, #2648]	; 0xa58
  2e:	1c59      	adds	r1, r3, #1
  30:	687a      	ldr	r2, [r7, #4]
  32:	f8c2 1a58 	str.w	r1, [r2, #2648]	; 0xa58
  36:	2234      	movs	r2, #52	; 0x34
  38:	fb02 f303 	mul.w	r3, r2, r3
  3c:	3328      	adds	r3, #40	; 0x28
  3e:	687a      	ldr	r2, [r7, #4]
  40:	4413      	add	r3, r2
  42:	3304      	adds	r3, #4
  44:	60fb      	str	r3, [r7, #12]
  46:	687b      	ldr	r3, [r7, #4]
  48:	2201      	movs	r2, #1
  4a:	f883 2a5f 	strb.w	r2, [r3, #2655]	; 0xa5f
  4e:	687b      	ldr	r3, [r7, #4]
  50:	f8d3 3a54 	ldr.w	r3, [r3, #2644]	; 0xa54
  54:	1c5a      	adds	r2, r3, #1
  56:	687b      	ldr	r3, [r7, #4]
  58:	f8c3 2a54 	str.w	r2, [r3, #2644]	; 0xa54
  5c:	68fb      	ldr	r3, [r7, #12]
  5e:	2201      	movs	r2, #1
  60:	711a      	strb	r2, [r3, #4]
  62:	68fb      	ldr	r3, [r7, #12]
  64:	4618      	mov	r0, r3
  66:	3710      	adds	r7, #16
  68:	46bd      	mov	sp, r7
  6a:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss:

00000000 <_ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss>:
   0:	b580      	push	{r7, lr}
   2:	b08a      	sub	sp, #40	; 0x28
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	460b      	mov	r3, r1
  10:	80fb      	strh	r3, [r7, #6]
  12:	4613      	mov	r3, r2
  14:	80bb      	strh	r3, [r7, #4]
  16:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  1a:	68fa      	ldr	r2, [r7, #12]
  1c:	8992      	ldrh	r2, [r2, #12]
  1e:	4293      	cmp	r3, r2
  20:	dc4e      	bgt.n	c0 <_ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss+0xc0>
  22:	f9b7 3004 	ldrsh.w	r3, [r7, #4]
  26:	68fa      	ldr	r2, [r7, #12]
  28:	8952      	ldrh	r2, [r2, #10]
  2a:	4293      	cmp	r3, r2
  2c:	db48      	blt.n	c0 <_ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss+0xc0>
  2e:	88f8      	ldrh	r0, [r7, #6]
  30:	88b9      	ldrh	r1, [r7, #4]
  32:	68fb      	ldr	r3, [r7, #12]
  34:	895a      	ldrh	r2, [r3, #10]
  36:	68fb      	ldr	r3, [r7, #12]
  38:	899b      	ldrh	r3, [r3, #12]
  3a:	f7ff fffe 	bl	0 <_ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss>
  3e:	4603      	mov	r3, r0
  40:	617b      	str	r3, [r7, #20]
  42:	8abb      	ldrh	r3, [r7, #20]
  44:	84fb      	strh	r3, [r7, #38]	; 0x26
  46:	8afb      	ldrh	r3, [r7, #22]
  48:	8cfa      	ldrh	r2, [r7, #38]	; 0x26
  4a:	429a      	cmp	r2, r3
  4c:	d239      	bcs.n	c2 <_ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss+0xc2>
  4e:	68fb      	ldr	r3, [r7, #12]
  50:	88db      	ldrh	r3, [r3, #6]
  52:	84bb      	strh	r3, [r7, #36]	; 0x24
  54:	68fb      	ldr	r3, [r7, #12]
  56:	891b      	ldrh	r3, [r3, #8]
  58:	8cba      	ldrh	r2, [r7, #36]	; 0x24
  5a:	429a      	cmp	r2, r3
  5c:	d22c      	bcs.n	b8 <_ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss+0xb8>
  5e:	68fb      	ldr	r3, [r7, #12]
  60:	89da      	ldrh	r2, [r3, #14]
  62:	68bb      	ldr	r3, [r7, #8]
  64:	623b      	str	r3, [r7, #32]
  66:	8cfb      	ldrh	r3, [r7, #38]	; 0x26
  68:	83fb      	strh	r3, [r7, #30]
  6a:	8cbb      	ldrh	r3, [r7, #36]	; 0x24
  6c:	83bb      	strh	r3, [r7, #28]
  6e:	4613      	mov	r3, r2
  70:	837b      	strh	r3, [r7, #26]
  72:	8bbb      	ldrh	r3, [r7, #28]
  74:	005b      	lsls	r3, r3, #1
  76:	833b      	strh	r3, [r7, #24]
  78:	8b7b      	ldrh	r3, [r7, #26]
  7a:	0a1b      	lsrs	r3, r3, #8
  7c:	b299      	uxth	r1, r3
  7e:	8bfa      	ldrh	r2, [r7, #30]
  80:	4613      	mov	r3, r2
  82:	011b      	lsls	r3, r3, #4
  84:	1a9b      	subs	r3, r3, r2
  86:	015b      	lsls	r3, r3, #5
  88:	461a      	mov	r2, r3
  8a:	6a3b      	ldr	r3, [r7, #32]
  8c:	441a      	add	r2, r3
  8e:	8b3b      	ldrh	r3, [r7, #24]
  90:	b2c9      	uxtb	r1, r1
  92:	54d1      	strb	r1, [r2, r3]
  94:	8bfa      	ldrh	r2, [r7, #30]
  96:	4613      	mov	r3, r2
  98:	011b      	lsls	r3, r3, #4
  9a:	1a9b      	subs	r3, r3, r2
  9c:	015b      	lsls	r3, r3, #5
  9e:	461a      	mov	r2, r3
  a0:	6a3b      	ldr	r3, [r7, #32]
  a2:	441a      	add	r2, r3
  a4:	8b3b      	ldrh	r3, [r7, #24]
  a6:	3301      	adds	r3, #1
  a8:	8b79      	ldrh	r1, [r7, #26]
  aa:	b2c9      	uxtb	r1, r1
  ac:	54d1      	strb	r1, [r2, r3]
  ae:	bf00      	nop
  b0:	8cbb      	ldrh	r3, [r7, #36]	; 0x24
  b2:	3301      	adds	r3, #1
  b4:	84bb      	strh	r3, [r7, #36]	; 0x24
  b6:	e7cd      	b.n	54 <_ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x54>
  b8:	8cfb      	ldrh	r3, [r7, #38]	; 0x26
  ba:	3301      	adds	r3, #1
  bc:	84fb      	strh	r3, [r7, #38]	; 0x26
  be:	e7c2      	b.n	46 <_ZN6Screen22FillRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x46>
  c0:	bf00      	nop
  c2:	3728      	adds	r7, #40	; 0x28
  c4:	46bd      	mov	sp, r7
  c6:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss:

00000000 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss>:
   0:	b580      	push	{r7, lr}
   2:	b098      	sub	sp, #96	; 0x60
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	460b      	mov	r3, r1
  10:	80fb      	strh	r3, [r7, #6]
  12:	4613      	mov	r3, r2
  14:	80bb      	strh	r3, [r7, #4]
  16:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  1a:	68fa      	ldr	r2, [r7, #12]
  1c:	8992      	ldrh	r2, [r2, #12]
  1e:	4293      	cmp	r3, r2
  20:	f300 8175 	bgt.w	30e <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x30e>
  24:	f9b7 3004 	ldrsh.w	r3, [r7, #4]
  28:	68fa      	ldr	r2, [r7, #12]
  2a:	8952      	ldrh	r2, [r2, #10]
  2c:	4293      	cmp	r3, r2
  2e:	f2c0 816e 	blt.w	30e <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x30e>
  32:	68f8      	ldr	r0, [r7, #12]
  34:	f7ff fffe 	bl	0 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss>
  38:	4603      	mov	r3, r0
  3a:	f8a7 3050 	strh.w	r3, [r7, #80]	; 0x50
  3e:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  42:	68fa      	ldr	r2, [r7, #12]
  44:	8952      	ldrh	r2, [r2, #10]
  46:	4293      	cmp	r3, r2
  48:	dc05      	bgt.n	56 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x56>
  4a:	68fb      	ldr	r3, [r7, #12]
  4c:	895a      	ldrh	r2, [r3, #10]
  4e:	88fb      	ldrh	r3, [r7, #6]
  50:	1ad3      	subs	r3, r2, r3
  52:	b29b      	uxth	r3, r3
  54:	e000      	b.n	58 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x58>
  56:	2300      	movs	r3, #0
  58:	f8a7 304e 	strh.w	r3, [r7, #78]	; 0x4e
  5c:	f9b7 3004 	ldrsh.w	r3, [r7, #4]
  60:	68fa      	ldr	r2, [r7, #12]
  62:	8992      	ldrh	r2, [r2, #12]
  64:	4293      	cmp	r3, r2
  66:	da04      	bge.n	72 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x72>
  68:	88ba      	ldrh	r2, [r7, #4]
  6a:	88fb      	ldrh	r3, [r7, #6]
  6c:	1ad3      	subs	r3, r2, r3
  6e:	b29b      	uxth	r3, r3
  70:	e004      	b.n	7c <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x7c>
  72:	68fb      	ldr	r3, [r7, #12]
  74:	899a      	ldrh	r2, [r3, #12]
  76:	88fb      	ldrh	r3, [r7, #6]
  78:	1ad3      	subs	r3, r2, r3
  7a:	b29b      	uxth	r3, r3
  7c:	f8a7 304c 	strh.w	r3, [r7, #76]	; 0x4c
  80:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  84:	68fa      	ldr	r2, [r7, #12]
  86:	8952      	ldrh	r2, [r2, #10]
  88:	4293      	cmp	r3, r2
  8a:	dc56      	bgt.n	13a <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x13a>
  8c:	f8b7 204c 	ldrh.w	r2, [r7, #76]	; 0x4c
  90:	f8b7 104e 	ldrh.w	r1, [r7, #78]	; 0x4e
  94:	f8b7 3050 	ldrh.w	r3, [r7, #80]	; 0x50
  98:	440b      	add	r3, r1
  9a:	4293      	cmp	r3, r2
  9c:	bfa8      	it	ge
  9e:	4613      	movge	r3, r2
  a0:	f8a7 304a 	strh.w	r3, [r7, #74]	; 0x4a
  a4:	f8b7 304e 	ldrh.w	r3, [r7, #78]	; 0x4e
  a8:	f8a7 305e 	strh.w	r3, [r7, #94]	; 0x5e
  ac:	f8b7 205e 	ldrh.w	r2, [r7, #94]	; 0x5e
  b0:	f8b7 304a 	ldrh.w	r3, [r7, #74]	; 0x4a
  b4:	429a      	cmp	r2, r3
  b6:	d240      	bcs.n	13a <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x13a>
  b8:	68fb      	ldr	r3, [r7, #12]
  ba:	88db      	ldrh	r3, [r3, #6]
  bc:	f8a7 305c 	strh.w	r3, [r7, #92]	; 0x5c
  c0:	68fb      	ldr	r3, [r7, #12]
  c2:	891b      	ldrh	r3, [r3, #8]
  c4:	f8b7 205c 	ldrh.w	r2, [r7, #92]	; 0x5c
  c8:	429a      	cmp	r2, r3
  ca:	d230      	bcs.n	12e <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x12e>
  cc:	68fb      	ldr	r3, [r7, #12]
  ce:	89da      	ldrh	r2, [r3, #14]
  d0:	68bb      	ldr	r3, [r7, #8]
  d2:	643b      	str	r3, [r7, #64]	; 0x40
  d4:	f8b7 305e 	ldrh.w	r3, [r7, #94]	; 0x5e
  d8:	87fb      	strh	r3, [r7, #62]	; 0x3e
  da:	f8b7 305c 	ldrh.w	r3, [r7, #92]	; 0x5c
  de:	87bb      	strh	r3, [r7, #60]	; 0x3c
  e0:	4613      	mov	r3, r2
  e2:	877b      	strh	r3, [r7, #58]	; 0x3a
  e4:	8fbb      	ldrh	r3, [r7, #60]	; 0x3c
  e6:	005b      	lsls	r3, r3, #1
  e8:	873b      	strh	r3, [r7, #56]	; 0x38
  ea:	8f7b      	ldrh	r3, [r7, #58]	; 0x3a
  ec:	0a1b      	lsrs	r3, r3, #8
  ee:	b299      	uxth	r1, r3
  f0:	8ffa      	ldrh	r2, [r7, #62]	; 0x3e
  f2:	4613      	mov	r3, r2
  f4:	011b      	lsls	r3, r3, #4
  f6:	1a9b      	subs	r3, r3, r2
  f8:	015b      	lsls	r3, r3, #5
  fa:	461a      	mov	r2, r3
  fc:	6c3b      	ldr	r3, [r7, #64]	; 0x40
  fe:	441a      	add	r2, r3
 100:	8f3b      	ldrh	r3, [r7, #56]	; 0x38
 102:	b2c9      	uxtb	r1, r1
 104:	54d1      	strb	r1, [r2, r3]
 106:	8ffa      	ldrh	r2, [r7, #62]	; 0x3e
 108:	4613      	mov	r3, r2
 10a:	011b      	lsls	r3, r3, #4
 10c:	1a9b      	subs	r3, r3, r2
 10e:	015b      	lsls	r3, r3, #5
 110:	461a      	mov	r2, r3
 112:	6c3b      	ldr	r3, [r7, #64]	; 0x40
 114:	441a      	add	r2, r3
 116:	8f3b      	ldrh	r3, [r7, #56]	; 0x38
 118:	3301      	adds	r3, #1
 11a:	8f79      	ldrh	r1, [r7, #58]	; 0x3a
 11c:	b2c9      	uxtb	r1, r1
 11e:	54d1      	strb	r1, [r2, r3]
 120:	bf00      	nop
 122:	f8b7 305c 	ldrh.w	r3, [r7, #92]	; 0x5c
 126:	3301      	adds	r3, #1
 128:	f8a7 305c 	strh.w	r3, [r7, #92]	; 0x5c
 12c:	e7c8      	b.n	c0 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0xc0>
 12e:	f8b7 305e 	ldrh.w	r3, [r7, #94]	; 0x5e
 132:	3301      	adds	r3, #1
 134:	f8a7 305e 	strh.w	r3, [r7, #94]	; 0x5e
 138:	e7b8      	b.n	ac <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0xac>
 13a:	f9b7 3004 	ldrsh.w	r3, [r7, #4]
 13e:	68fa      	ldr	r2, [r7, #12]
 140:	8992      	ldrh	r2, [r2, #12]
 142:	4293      	cmp	r3, r2
 144:	db56      	blt.n	1f4 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x1f4>
 146:	f8b7 204e 	ldrh.w	r2, [r7, #78]	; 0x4e
 14a:	f8b7 104c 	ldrh.w	r1, [r7, #76]	; 0x4c
 14e:	f8b7 3050 	ldrh.w	r3, [r7, #80]	; 0x50
 152:	1acb      	subs	r3, r1, r3
 154:	4293      	cmp	r3, r2
 156:	bfb8      	it	lt
 158:	4613      	movlt	r3, r2
 15a:	f8a7 3048 	strh.w	r3, [r7, #72]	; 0x48
 15e:	f8b7 3048 	ldrh.w	r3, [r7, #72]	; 0x48
 162:	f8a7 305a 	strh.w	r3, [r7, #90]	; 0x5a
 166:	f8b7 205a 	ldrh.w	r2, [r7, #90]	; 0x5a
 16a:	f8b7 304c 	ldrh.w	r3, [r7, #76]	; 0x4c
 16e:	429a      	cmp	r2, r3
 170:	d240      	bcs.n	1f4 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x1f4>
 172:	68fb      	ldr	r3, [r7, #12]
 174:	88db      	ldrh	r3, [r3, #6]
 176:	f8a7 3058 	strh.w	r3, [r7, #88]	; 0x58
 17a:	68fb      	ldr	r3, [r7, #12]
 17c:	891b      	ldrh	r3, [r3, #8]
 17e:	f8b7 2058 	ldrh.w	r2, [r7, #88]	; 0x58
 182:	429a      	cmp	r2, r3
 184:	d230      	bcs.n	1e8 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x1e8>
 186:	68fb      	ldr	r3, [r7, #12]
 188:	89da      	ldrh	r2, [r3, #14]
 18a:	68bb      	ldr	r3, [r7, #8]
 18c:	637b      	str	r3, [r7, #52]	; 0x34
 18e:	f8b7 305a 	ldrh.w	r3, [r7, #90]	; 0x5a
 192:	867b      	strh	r3, [r7, #50]	; 0x32
 194:	f8b7 3058 	ldrh.w	r3, [r7, #88]	; 0x58
 198:	863b      	strh	r3, [r7, #48]	; 0x30
 19a:	4613      	mov	r3, r2
 19c:	85fb      	strh	r3, [r7, #46]	; 0x2e
 19e:	8e3b      	ldrh	r3, [r7, #48]	; 0x30
 1a0:	005b      	lsls	r3, r3, #1
 1a2:	85bb      	strh	r3, [r7, #44]	; 0x2c
 1a4:	8dfb      	ldrh	r3, [r7, #46]	; 0x2e
 1a6:	0a1b      	lsrs	r3, r3, #8
 1a8:	b299      	uxth	r1, r3
 1aa:	8e7a      	ldrh	r2, [r7, #50]	; 0x32
 1ac:	4613      	mov	r3, r2
 1ae:	011b      	lsls	r3, r3, #4
 1b0:	1a9b      	subs	r3, r3, r2
 1b2:	015b      	lsls	r3, r3, #5
 1b4:	461a      	mov	r2, r3
 1b6:	6b7b      	ldr	r3, [r7, #52]	; 0x34
 1b8:	441a      	add	r2, r3
 1ba:	8dbb      	ldrh	r3, [r7, #44]	; 0x2c
 1bc:	b2c9      	uxtb	r1, r1
 1be:	54d1      	strb	r1, [r2, r3]
 1c0:	8e7a      	ldrh	r2, [r7, #50]	; 0x32
 1c2:	4613      	mov	r3, r2
 1c4:	011b      	lsls	r3, r3, #4
 1c6:	1a9b      	subs	r3, r3, r2
 1c8:	015b      	lsls	r3, r3, #5
 1ca:	461a      	mov	r2, r3
 1cc:	6b7b      	ldr	r3, [r7, #52]	; 0x34
 1ce:	441a      	add	r2, r3
 1d0:	8dbb      	ldrh	r3, [r7, #44]	; 0x2c
 1d2:	3301      	adds	r3, #1
 1d4:	8df9      	ldrh	r1, [r7, #46]	; 0x2e
 1d6:	b2c9      	uxtb	r1, r1
 1d8:	54d1      	strb	r1, [r2, r3]
 1da:	bf00      	nop
 1dc:	f8b7 3058 	ldrh.w	r3, [r7, #88]	; 0x58
 1e0:	3301      	adds	r3, #1
 1e2:	f8a7 3058 	strh.w	r3, [r7, #88]	; 0x58
 1e6:	e7c8      	b.n	17a <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x17a>
 1e8:	f8b7 305a 	ldrh.w	r3, [r7, #90]	; 0x5a
 1ec:	3301      	adds	r3, #1
 1ee:	f8a7 305a 	strh.w	r3, [r7, #90]	; 0x5a
 1f2:	e7b8      	b.n	166 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x166>
 1f4:	68fb      	ldr	r3, [r7, #12]
 1f6:	88da      	ldrh	r2, [r3, #6]
 1f8:	f8b7 3050 	ldrh.w	r3, [r7, #80]	; 0x50
 1fc:	4413      	add	r3, r2
 1fe:	f8a7 3046 	strh.w	r3, [r7, #70]	; 0x46
 202:	68fb      	ldr	r3, [r7, #12]
 204:	891a      	ldrh	r2, [r3, #8]
 206:	f8b7 3050 	ldrh.w	r3, [r7, #80]	; 0x50
 20a:	1ad3      	subs	r3, r2, r3
 20c:	f8a7 3044 	strh.w	r3, [r7, #68]	; 0x44
 210:	f8b7 304e 	ldrh.w	r3, [r7, #78]	; 0x4e
 214:	f8a7 3056 	strh.w	r3, [r7, #86]	; 0x56
 218:	f8b7 2056 	ldrh.w	r2, [r7, #86]	; 0x56
 21c:	f8b7 304c 	ldrh.w	r3, [r7, #76]	; 0x4c
 220:	429a      	cmp	r2, r3
 222:	d275      	bcs.n	310 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x310>
 224:	68fb      	ldr	r3, [r7, #12]
 226:	88db      	ldrh	r3, [r3, #6]
 228:	f8a7 3054 	strh.w	r3, [r7, #84]	; 0x54
 22c:	f8b7 3044 	ldrh.w	r3, [r7, #68]	; 0x44
 230:	f8a7 3052 	strh.w	r3, [r7, #82]	; 0x52
 234:	f8b7 2054 	ldrh.w	r2, [r7, #84]	; 0x54
 238:	f8b7 3046 	ldrh.w	r3, [r7, #70]	; 0x46
 23c:	429a      	cmp	r2, r3
 23e:	d260      	bcs.n	302 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x302>
 240:	68fb      	ldr	r3, [r7, #12]
 242:	89da      	ldrh	r2, [r3, #14]
 244:	68bb      	ldr	r3, [r7, #8]
 246:	61fb      	str	r3, [r7, #28]
 248:	f8b7 3056 	ldrh.w	r3, [r7, #86]	; 0x56
 24c:	837b      	strh	r3, [r7, #26]
 24e:	f8b7 3054 	ldrh.w	r3, [r7, #84]	; 0x54
 252:	833b      	strh	r3, [r7, #24]
 254:	4613      	mov	r3, r2
 256:	82fb      	strh	r3, [r7, #22]
 258:	8b3b      	ldrh	r3, [r7, #24]
 25a:	005b      	lsls	r3, r3, #1
 25c:	82bb      	strh	r3, [r7, #20]
 25e:	8afb      	ldrh	r3, [r7, #22]
 260:	0a1b      	lsrs	r3, r3, #8
 262:	b299      	uxth	r1, r3
 264:	8b7a      	ldrh	r2, [r7, #26]
 266:	4613      	mov	r3, r2
 268:	011b      	lsls	r3, r3, #4
 26a:	1a9b      	subs	r3, r3, r2
 26c:	015b      	lsls	r3, r3, #5
 26e:	461a      	mov	r2, r3
 270:	69fb      	ldr	r3, [r7, #28]
 272:	441a      	add	r2, r3
 274:	8abb      	ldrh	r3, [r7, #20]
 276:	b2c9      	uxtb	r1, r1
 278:	54d1      	strb	r1, [r2, r3]
 27a:	8b7a      	ldrh	r2, [r7, #26]
 27c:	4613      	mov	r3, r2
 27e:	011b      	lsls	r3, r3, #4
 280:	1a9b      	subs	r3, r3, r2
 282:	015b      	lsls	r3, r3, #5
 284:	461a      	mov	r2, r3
 286:	69fb      	ldr	r3, [r7, #28]
 288:	441a      	add	r2, r3
 28a:	8abb      	ldrh	r3, [r7, #20]
 28c:	3301      	adds	r3, #1
 28e:	8af9      	ldrh	r1, [r7, #22]
 290:	b2c9      	uxtb	r1, r1
 292:	54d1      	strb	r1, [r2, r3]
 294:	bf00      	nop
 296:	68fb      	ldr	r3, [r7, #12]
 298:	89da      	ldrh	r2, [r3, #14]
 29a:	68bb      	ldr	r3, [r7, #8]
 29c:	62bb      	str	r3, [r7, #40]	; 0x28
 29e:	f8b7 3056 	ldrh.w	r3, [r7, #86]	; 0x56
 2a2:	84fb      	strh	r3, [r7, #38]	; 0x26
 2a4:	f8b7 3052 	ldrh.w	r3, [r7, #82]	; 0x52
 2a8:	84bb      	strh	r3, [r7, #36]	; 0x24
 2aa:	4613      	mov	r3, r2
 2ac:	847b      	strh	r3, [r7, #34]	; 0x22
 2ae:	8cbb      	ldrh	r3, [r7, #36]	; 0x24
 2b0:	005b      	lsls	r3, r3, #1
 2b2:	843b      	strh	r3, [r7, #32]
 2b4:	8c7b      	ldrh	r3, [r7, #34]	; 0x22
 2b6:	0a1b      	lsrs	r3, r3, #8
 2b8:	b299      	uxth	r1, r3
 2ba:	8cfa      	ldrh	r2, [r7, #38]	; 0x26
 2bc:	4613      	mov	r3, r2
 2be:	011b      	lsls	r3, r3, #4
 2c0:	1a9b      	subs	r3, r3, r2
 2c2:	015b      	lsls	r3, r3, #5
 2c4:	461a      	mov	r2, r3
 2c6:	6abb      	ldr	r3, [r7, #40]	; 0x28
 2c8:	441a      	add	r2, r3
 2ca:	8c3b      	ldrh	r3, [r7, #32]
 2cc:	b2c9      	uxtb	r1, r1
 2ce:	54d1      	strb	r1, [r2, r3]
 2d0:	8cfa      	ldrh	r2, [r7, #38]	; 0x26
 2d2:	4613      	mov	r3, r2
 2d4:	011b      	lsls	r3, r3, #4
 2d6:	1a9b      	subs	r3, r3, r2
 2d8:	015b      	lsls	r3, r3, #5
 2da:	461a      	mov	r2, r3
 2dc:	6abb      	ldr	r3, [r7, #40]	; 0x28
 2de:	441a      	add	r2, r3
 2e0:	8c3b      	ldrh	r3, [r7, #32]
 2e2:	3301      	adds	r3, #1
 2e4:	8c79      	ldrh	r1, [r7, #34]	; 0x22
 2e6:	b2c9      	uxtb	r1, r1
 2e8:	54d1      	strb	r1, [r2, r3]
 2ea:	bf00      	nop
 2ec:	f8b7 3052 	ldrh.w	r3, [r7, #82]	; 0x52
 2f0:	3301      	adds	r3, #1
 2f2:	f8a7 3052 	strh.w	r3, [r7, #82]	; 0x52
 2f6:	f8b7 3054 	ldrh.w	r3, [r7, #84]	; 0x54
 2fa:	3301      	adds	r3, #1
 2fc:	f8a7 3054 	strh.w	r3, [r7, #84]	; 0x54
 300:	e798      	b.n	234 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x234>
 302:	f8b7 3056 	ldrh.w	r3, [r7, #86]	; 0x56
 306:	3301      	adds	r3, #1
 308:	f8a7 3056 	strh.w	r3, [r7, #86]	; 0x56
 30c:	e784      	b.n	218 <_ZN6Screen22DrawRectangleProcedureERNS_10DrawMemoryEPA480_hss+0x218>
 30e:	bf00      	nop
 310:	3760      	adds	r7, #96	; 0x60
 312:	46bd      	mov	sp, r7
 314:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss:

00000000 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss>:
   0:	b580      	push	{r7, lr}
   2:	b08e      	sub	sp, #56	; 0x38
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	460b      	mov	r3, r1
  10:	80fb      	strh	r3, [r7, #6]
  12:	4613      	mov	r3, r2
  14:	80bb      	strh	r3, [r7, #4]
  16:	68f8      	ldr	r0, [r7, #12]
  18:	f7ff fffe 	bl	0 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss>
  1c:	4603      	mov	r3, r0
  1e:	85bb      	strh	r3, [r7, #44]	; 0x2c
  20:	68f8      	ldr	r0, [r7, #12]
  22:	f7ff fffe 	bl	0 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss>
  26:	4603      	mov	r3, r0
  28:	637b      	str	r3, [r7, #52]	; 0x34
  2a:	2300      	movs	r3, #0
  2c:	867b      	strh	r3, [r7, #50]	; 0x32
  2e:	68fb      	ldr	r3, [r7, #12]
  30:	895b      	ldrh	r3, [r3, #10]
  32:	461a      	mov	r2, r3
  34:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  38:	4293      	cmp	r3, r2
  3a:	bfb8      	it	lt
  3c:	4613      	movlt	r3, r2
  3e:	857b      	strh	r3, [r7, #42]	; 0x2a
  40:	68fb      	ldr	r3, [r7, #12]
  42:	899b      	ldrh	r3, [r3, #12]
  44:	461a      	mov	r2, r3
  46:	f9b7 3004 	ldrsh.w	r3, [r7, #4]
  4a:	4293      	cmp	r3, r2
  4c:	bfa8      	it	ge
  4e:	4613      	movge	r3, r2
  50:	853b      	strh	r3, [r7, #40]	; 0x28
  52:	8d7b      	ldrh	r3, [r7, #42]	; 0x2a
  54:	863b      	strh	r3, [r7, #48]	; 0x30
  56:	8e3a      	ldrh	r2, [r7, #48]	; 0x30
  58:	8d3b      	ldrh	r3, [r7, #40]	; 0x28
  5a:	429a      	cmp	r2, r3
  5c:	d279      	bcs.n	152 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss+0x152>
  5e:	2300      	movs	r3, #0
  60:	867b      	strh	r3, [r7, #50]	; 0x32
  62:	68fb      	ldr	r3, [r7, #12]
  64:	88db      	ldrh	r3, [r3, #6]
  66:	85fb      	strh	r3, [r7, #46]	; 0x2e
  68:	68fb      	ldr	r3, [r7, #12]
  6a:	891b      	ldrh	r3, [r3, #8]
  6c:	8dfa      	ldrh	r2, [r7, #46]	; 0x2e
  6e:	429a      	cmp	r2, r3
  70:	d268      	bcs.n	144 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss+0x144>
  72:	6b7b      	ldr	r3, [r7, #52]	; 0x34
  74:	781b      	ldrb	r3, [r3, #0]
  76:	461a      	mov	r2, r3
  78:	8e7b      	ldrh	r3, [r7, #50]	; 0x32
  7a:	fa02 f303 	lsl.w	r3, r2, r3
  7e:	f003 0380 	and.w	r3, r3, #128	; 0x80
  82:	2b00      	cmp	r3, #0
  84:	d028      	beq.n	d8 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss+0xd8>
  86:	68fb      	ldr	r3, [r7, #12]
  88:	89da      	ldrh	r2, [r3, #14]
  8a:	68bb      	ldr	r3, [r7, #8]
  8c:	627b      	str	r3, [r7, #36]	; 0x24
  8e:	8e3b      	ldrh	r3, [r7, #48]	; 0x30
  90:	847b      	strh	r3, [r7, #34]	; 0x22
  92:	8dfb      	ldrh	r3, [r7, #46]	; 0x2e
  94:	843b      	strh	r3, [r7, #32]
  96:	4613      	mov	r3, r2
  98:	83fb      	strh	r3, [r7, #30]
  9a:	8c3b      	ldrh	r3, [r7, #32]
  9c:	005b      	lsls	r3, r3, #1
  9e:	83bb      	strh	r3, [r7, #28]
  a0:	8bfb      	ldrh	r3, [r7, #30]
  a2:	0a1b      	lsrs	r3, r3, #8
  a4:	b299      	uxth	r1, r3
  a6:	8c7a      	ldrh	r2, [r7, #34]	; 0x22
  a8:	4613      	mov	r3, r2
  aa:	011b      	lsls	r3, r3, #4
  ac:	1a9b      	subs	r3, r3, r2
  ae:	015b      	lsls	r3, r3, #5
  b0:	461a      	mov	r2, r3
  b2:	6a7b      	ldr	r3, [r7, #36]	; 0x24
  b4:	441a      	add	r2, r3
  b6:	8bbb      	ldrh	r3, [r7, #28]
  b8:	b2c9      	uxtb	r1, r1
  ba:	54d1      	strb	r1, [r2, r3]
  bc:	8c7a      	ldrh	r2, [r7, #34]	; 0x22
  be:	4613      	mov	r3, r2
  c0:	011b      	lsls	r3, r3, #4
  c2:	1a9b      	subs	r3, r3, r2
  c4:	015b      	lsls	r3, r3, #5
  c6:	461a      	mov	r2, r3
  c8:	6a7b      	ldr	r3, [r7, #36]	; 0x24
  ca:	441a      	add	r2, r3
  cc:	8bbb      	ldrh	r3, [r7, #28]
  ce:	3301      	adds	r3, #1
  d0:	8bf9      	ldrh	r1, [r7, #30]
  d2:	b2c9      	uxtb	r1, r1
  d4:	54d1      	strb	r1, [r2, r3]
  d6:	e026      	b.n	126 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss+0x126>
  d8:	68bb      	ldr	r3, [r7, #8]
  da:	61bb      	str	r3, [r7, #24]
  dc:	8e3b      	ldrh	r3, [r7, #48]	; 0x30
  de:	82fb      	strh	r3, [r7, #22]
  e0:	8dfb      	ldrh	r3, [r7, #46]	; 0x2e
  e2:	82bb      	strh	r3, [r7, #20]
  e4:	8dbb      	ldrh	r3, [r7, #44]	; 0x2c
  e6:	827b      	strh	r3, [r7, #18]
  e8:	8abb      	ldrh	r3, [r7, #20]
  ea:	005b      	lsls	r3, r3, #1
  ec:	823b      	strh	r3, [r7, #16]
  ee:	8a7b      	ldrh	r3, [r7, #18]
  f0:	0a1b      	lsrs	r3, r3, #8
  f2:	b299      	uxth	r1, r3
  f4:	8afa      	ldrh	r2, [r7, #22]
  f6:	4613      	mov	r3, r2
  f8:	011b      	lsls	r3, r3, #4
  fa:	1a9b      	subs	r3, r3, r2
  fc:	015b      	lsls	r3, r3, #5
  fe:	461a      	mov	r2, r3
 100:	69bb      	ldr	r3, [r7, #24]
 102:	441a      	add	r2, r3
 104:	8a3b      	ldrh	r3, [r7, #16]
 106:	b2c9      	uxtb	r1, r1
 108:	54d1      	strb	r1, [r2, r3]
 10a:	8afa      	ldrh	r2, [r7, #22]
 10c:	4613      	mov	r3, r2
 10e:	011b      	lsls	r3, r3, #4
 110:	1a9b      	subs	r3, r3, r2
 112:	015b      	lsls	r3, r3, #5
 114:	461a      	mov	r2, r3
 116:	69bb      	ldr	r3, [r7, #24]
 118:	441a      	add	r2, r3
 11a:	8a3b      	ldrh	r3, [r7, #16]
 11c:	3301      	adds	r3, #1
 11e:	8a79      	ldrh	r1, [r7, #18]
 120:	b2c9      	uxtb	r1, r1
 122:	54d1      	strb	r1, [r2, r3]
 124:	bf00      	nop
 126:	8e7b      	ldrh	r3, [r7, #50]	; 0x32
 128:	3301      	adds	r3, #1
 12a:	867b      	strh	r3, [r7, #50]	; 0x32
 12c:	8e7b      	ldrh	r3, [r7, #50]	; 0x32
 12e:	2b07      	cmp	r3, #7
 130:	d904      	bls.n	13c <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss+0x13c>
 132:	2300      	movs	r3, #0
 134:	867b      	strh	r3, [r7, #50]	; 0x32
 136:	6b7b      	ldr	r3, [r7, #52]	; 0x34
 138:	3301      	adds	r3, #1
 13a:	637b      	str	r3, [r7, #52]	; 0x34
 13c:	8dfb      	ldrh	r3, [r7, #46]	; 0x2e
 13e:	3301      	adds	r3, #1
 140:	85fb      	strh	r3, [r7, #46]	; 0x2e
 142:	e791      	b.n	68 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss+0x68>
 144:	6b7b      	ldr	r3, [r7, #52]	; 0x34
 146:	3301      	adds	r3, #1
 148:	637b      	str	r3, [r7, #52]	; 0x34
 14a:	8e3b      	ldrh	r3, [r7, #48]	; 0x30
 14c:	3301      	adds	r3, #1
 14e:	863b      	strh	r3, [r7, #48]	; 0x30
 150:	e781      	b.n	56 <_ZN6Screen22DrawCharacterProcedureERNS_10DrawMemoryEPA480_hss+0x56>
 152:	2300      	movs	r3, #0
 154:	637b      	str	r3, [r7, #52]	; 0x34
 156:	bf00      	nop
 158:	3738      	adds	r7, #56	; 0x38
 15a:	46bd      	mov	sp, r7
 15c:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss:

00000000 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss>:
   0:	b580      	push	{r7, lr}
   2:	b08e      	sub	sp, #56	; 0x38
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	460b      	mov	r3, r1
  10:	80fb      	strh	r3, [r7, #6]
  12:	4613      	mov	r3, r2
  14:	80bb      	strh	r3, [r7, #4]
  16:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  1a:	68fa      	ldr	r2, [r7, #12]
  1c:	8992      	ldrh	r2, [r2, #12]
  1e:	4293      	cmp	r3, r2
  20:	f300 80af 	bgt.w	182 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0x182>
  24:	f9b7 3004 	ldrsh.w	r3, [r7, #4]
  28:	68fa      	ldr	r2, [r7, #12]
  2a:	8952      	ldrh	r2, [r2, #10]
  2c:	4293      	cmp	r3, r2
  2e:	f2c0 80a8 	blt.w	182 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0x182>
  32:	68f8      	ldr	r0, [r7, #12]
  34:	f7ff fffe 	bl	0 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss>
  38:	4603      	mov	r3, r0
  3a:	84bb      	strh	r3, [r7, #36]	; 0x24
  3c:	68f8      	ldr	r0, [r7, #12]
  3e:	f7ff fffe 	bl	0 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss>
  42:	4603      	mov	r3, r0
  44:	623b      	str	r3, [r7, #32]
  46:	68f8      	ldr	r0, [r7, #12]
  48:	f7ff fffe 	bl	0 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss>
  4c:	4603      	mov	r3, r0
  4e:	77fb      	strb	r3, [r7, #31]
  50:	68f8      	ldr	r0, [r7, #12]
  52:	f7ff fffe 	bl	0 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss>
  56:	4603      	mov	r3, r0
  58:	77bb      	strb	r3, [r7, #30]
  5a:	68f8      	ldr	r0, [r7, #12]
  5c:	f7ff fffe 	bl	0 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss>
  60:	4603      	mov	r3, r0
  62:	61bb      	str	r3, [r7, #24]
  64:	7ffb      	ldrb	r3, [r7, #31]
  66:	08db      	lsrs	r3, r3, #3
  68:	b2db      	uxtb	r3, r3
  6a:	b29b      	uxth	r3, r3
  6c:	3301      	adds	r3, #1
  6e:	82fb      	strh	r3, [r7, #22]
  70:	68fb      	ldr	r3, [r7, #12]
  72:	895b      	ldrh	r3, [r3, #10]
  74:	461a      	mov	r2, r3
  76:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  7a:	4293      	cmp	r3, r2
  7c:	bfb8      	it	lt
  7e:	4613      	movlt	r3, r2
  80:	82bb      	strh	r3, [r7, #20]
  82:	68fb      	ldr	r3, [r7, #12]
  84:	899b      	ldrh	r3, [r3, #12]
  86:	461a      	mov	r2, r3
  88:	f9b7 3004 	ldrsh.w	r3, [r7, #4]
  8c:	4293      	cmp	r3, r2
  8e:	bfa8      	it	ge
  90:	4613      	movge	r3, r2
  92:	827b      	strh	r3, [r7, #18]
  94:	2300      	movs	r3, #0
  96:	86fb      	strh	r3, [r7, #54]	; 0x36
  98:	2300      	movs	r3, #0
  9a:	633b      	str	r3, [r7, #48]	; 0x30
  9c:	2300      	movs	r3, #0
  9e:	85fb      	strh	r3, [r7, #46]	; 0x2e
  a0:	2300      	movs	r3, #0
  a2:	85bb      	strh	r3, [r7, #44]	; 0x2c
  a4:	2300      	movs	r3, #0
  a6:	857b      	strh	r3, [r7, #42]	; 0x2a
  a8:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  ac:	68fa      	ldr	r2, [r7, #12]
  ae:	8952      	ldrh	r2, [r2, #10]
  b0:	4293      	cmp	r3, r2
  b2:	dd04      	ble.n	be <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0xbe>
  b4:	88fa      	ldrh	r2, [r7, #6]
  b6:	68fb      	ldr	r3, [r7, #12]
  b8:	895b      	ldrh	r3, [r3, #10]
  ba:	1ad3      	subs	r3, r2, r3
  bc:	857b      	strh	r3, [r7, #42]	; 0x2a
  be:	8abb      	ldrh	r3, [r7, #20]
  c0:	853b      	strh	r3, [r7, #40]	; 0x28
  c2:	8d3a      	ldrh	r2, [r7, #40]	; 0x28
  c4:	8a7b      	ldrh	r3, [r7, #18]
  c6:	429a      	cmp	r2, r3
  c8:	d256      	bcs.n	178 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0x178>
  ca:	2300      	movs	r3, #0
  cc:	85fb      	strh	r3, [r7, #46]	; 0x2e
  ce:	2300      	movs	r3, #0
  d0:	85bb      	strh	r3, [r7, #44]	; 0x2c
  d2:	2300      	movs	r3, #0
  d4:	86fb      	strh	r3, [r7, #54]	; 0x36
  d6:	8efb      	ldrh	r3, [r7, #54]	; 0x36
  d8:	6a3a      	ldr	r2, [r7, #32]
  da:	4413      	add	r3, r2
  dc:	7819      	ldrb	r1, [r3, #0]
  de:	7ffb      	ldrb	r3, [r7, #31]
  e0:	b29a      	uxth	r2, r3
  e2:	7fbb      	ldrb	r3, [r7, #30]
  e4:	b29b      	uxth	r3, r3
  e6:	69b8      	ldr	r0, [r7, #24]
  e8:	f7ff fffe 	bl	0 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss>
  ec:	4601      	mov	r1, r0
  ee:	8d7b      	ldrh	r3, [r7, #42]	; 0x2a
  f0:	8afa      	ldrh	r2, [r7, #22]
  f2:	fb02 f303 	mul.w	r3, r2, r3
  f6:	440b      	add	r3, r1
  f8:	633b      	str	r3, [r7, #48]	; 0x30
  fa:	68fb      	ldr	r3, [r7, #12]
  fc:	88db      	ldrh	r3, [r3, #6]
  fe:	84fb      	strh	r3, [r7, #38]	; 0x26
 100:	68fb      	ldr	r3, [r7, #12]
 102:	891b      	ldrh	r3, [r3, #8]
 104:	8cfa      	ldrh	r2, [r7, #38]	; 0x26
 106:	429a      	cmp	r2, r3
 108:	d22f      	bcs.n	16a <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0x16a>
 10a:	8dbb      	ldrh	r3, [r7, #44]	; 0x2c
 10c:	3301      	adds	r3, #1
 10e:	85bb      	strh	r3, [r7, #44]	; 0x2c
 110:	8dfb      	ldrh	r3, [r7, #46]	; 0x2e
 112:	3301      	adds	r3, #1
 114:	85fb      	strh	r3, [r7, #46]	; 0x2e
 116:	7ffb      	ldrb	r3, [r7, #31]
 118:	b29b      	uxth	r3, r3
 11a:	8dfa      	ldrh	r2, [r7, #46]	; 0x2e
 11c:	429a      	cmp	r2, r3
 11e:	d318      	bcc.n	152 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0x152>
 120:	2300      	movs	r3, #0
 122:	85fb      	strh	r3, [r7, #46]	; 0x2e
 124:	8efb      	ldrh	r3, [r7, #54]	; 0x36
 126:	3301      	adds	r3, #1
 128:	86fb      	strh	r3, [r7, #54]	; 0x36
 12a:	8efb      	ldrh	r3, [r7, #54]	; 0x36
 12c:	6a3a      	ldr	r2, [r7, #32]
 12e:	4413      	add	r3, r2
 130:	7819      	ldrb	r1, [r3, #0]
 132:	7ffb      	ldrb	r3, [r7, #31]
 134:	b29a      	uxth	r2, r3
 136:	7fbb      	ldrb	r3, [r7, #30]
 138:	b29b      	uxth	r3, r3
 13a:	69b8      	ldr	r0, [r7, #24]
 13c:	f7ff fffe 	bl	0 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss>
 140:	4601      	mov	r1, r0
 142:	8d7b      	ldrh	r3, [r7, #42]	; 0x2a
 144:	8afa      	ldrh	r2, [r7, #22]
 146:	fb02 f303 	mul.w	r3, r2, r3
 14a:	440b      	add	r3, r1
 14c:	633b      	str	r3, [r7, #48]	; 0x30
 14e:	2300      	movs	r3, #0
 150:	85bb      	strh	r3, [r7, #44]	; 0x2c
 152:	8dbb      	ldrh	r3, [r7, #44]	; 0x2c
 154:	2b07      	cmp	r3, #7
 156:	d904      	bls.n	162 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0x162>
 158:	2300      	movs	r3, #0
 15a:	85bb      	strh	r3, [r7, #44]	; 0x2c
 15c:	6b3b      	ldr	r3, [r7, #48]	; 0x30
 15e:	3301      	adds	r3, #1
 160:	633b      	str	r3, [r7, #48]	; 0x30
 162:	8cfb      	ldrh	r3, [r7, #38]	; 0x26
 164:	3301      	adds	r3, #1
 166:	84fb      	strh	r3, [r7, #38]	; 0x26
 168:	e7ca      	b.n	100 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0x100>
 16a:	8d7b      	ldrh	r3, [r7, #42]	; 0x2a
 16c:	3301      	adds	r3, #1
 16e:	857b      	strh	r3, [r7, #42]	; 0x2a
 170:	8d3b      	ldrh	r3, [r7, #40]	; 0x28
 172:	3301      	adds	r3, #1
 174:	853b      	strh	r3, [r7, #40]	; 0x28
 176:	e7a4      	b.n	c2 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0xc2>
 178:	2300      	movs	r3, #0
 17a:	633b      	str	r3, [r7, #48]	; 0x30
 17c:	2300      	movs	r3, #0
 17e:	61bb      	str	r3, [r7, #24]
 180:	e000      	b.n	184 <_ZN6Screen19DrawStringProcedureERNS_10DrawMemoryEPA480_hss+0x184>
 182:	bf00      	nop
 184:	3738      	adds	r7, #56	; 0x38
 186:	46bd      	mov	sp, r7
 188:	bd80      	pop	{r7, pc}

Disassembly of section .text._ZN6Screen10BoundCheckERtS0_S0_S0_:

00000000 <_ZN6Screen10BoundCheckERtS0_S0_S0_>:
   0:	b480      	push	{r7}
   2:	b087      	sub	sp, #28
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	607a      	str	r2, [r7, #4]
   c:	603b      	str	r3, [r7, #0]
   e:	68bb      	ldr	r3, [r7, #8]
  10:	881a      	ldrh	r2, [r3, #0]
  12:	68fb      	ldr	r3, [r7, #12]
  14:	8cdb      	ldrh	r3, [r3, #38]	; 0x26
  16:	429a      	cmp	r2, r3
  18:	d230      	bcs.n	7c <_ZN6Screen10BoundCheckERtS0_S0_S0_+0x7c>
  1a:	683b      	ldr	r3, [r7, #0]
  1c:	881a      	ldrh	r2, [r3, #0]
  1e:	68fb      	ldr	r3, [r7, #12]
  20:	8c9b      	ldrh	r3, [r3, #36]	; 0x24
  22:	429a      	cmp	r2, r3
  24:	d22a      	bcs.n	7c <_ZN6Screen10BoundCheckERtS0_S0_S0_+0x7c>
  26:	687b      	ldr	r3, [r7, #4]
  28:	881a      	ldrh	r2, [r3, #0]
  2a:	68fb      	ldr	r3, [r7, #12]
  2c:	8cdb      	ldrh	r3, [r3, #38]	; 0x26
  2e:	429a      	cmp	r2, r3
  30:	d303      	bcc.n	3a <_ZN6Screen10BoundCheckERtS0_S0_S0_+0x3a>
  32:	68fb      	ldr	r3, [r7, #12]
  34:	8cda      	ldrh	r2, [r3, #38]	; 0x26
  36:	687b      	ldr	r3, [r7, #4]
  38:	801a      	strh	r2, [r3, #0]
  3a:	68bb      	ldr	r3, [r7, #8]
  3c:	881a      	ldrh	r2, [r3, #0]
  3e:	687b      	ldr	r3, [r7, #4]
  40:	881b      	ldrh	r3, [r3, #0]
  42:	429a      	cmp	r2, r3
  44:	d909      	bls.n	5a <_ZN6Screen10BoundCheckERtS0_S0_S0_+0x5a>
  46:	68bb      	ldr	r3, [r7, #8]
  48:	881b      	ldrh	r3, [r3, #0]
  4a:	82fb      	strh	r3, [r7, #22]
  4c:	687b      	ldr	r3, [r7, #4]
  4e:	881a      	ldrh	r2, [r3, #0]
  50:	68bb      	ldr	r3, [r7, #8]
  52:	801a      	strh	r2, [r3, #0]
  54:	687b      	ldr	r3, [r7, #4]
  56:	8afa      	ldrh	r2, [r7, #22]
  58:	801a      	strh	r2, [r3, #0]
  5a:	683b      	ldr	r3, [r7, #0]
  5c:	881a      	ldrh	r2, [r3, #0]
  5e:	6a3b      	ldr	r3, [r7, #32]
  60:	881b      	ldrh	r3, [r3, #0]
  62:	429a      	cmp	r2, r3
  64:	d90b      	bls.n	7e <_ZN6Screen10BoundCheckERtS0_S0_S0_+0x7e>
  66:	683b      	ldr	r3, [r7, #0]
  68:	881b      	ldrh	r3, [r3, #0]
  6a:	82bb      	strh	r3, [r7, #20]
  6c:	6a3b      	ldr	r3, [r7, #32]
  6e:	881a      	ldrh	r2, [r3, #0]
  70:	683b      	ldr	r3, [r7, #0]
  72:	801a      	strh	r2, [r3, #0]
  74:	6a3b      	ldr	r3, [r7, #32]
  76:	8aba      	ldrh	r2, [r7, #20]
  78:	801a      	strh	r2, [r3, #0]
  7a:	e000      	b.n	7e <_ZN6Screen10BoundCheckERtS0_S0_S0_+0x7e>
  7c:	bf00      	nop
  7e:	371c      	adds	r7, #28
  80:	46bd      	mov	sp, r7
  82:	f85d 7b04 	ldr.w	r7, [sp], #4
  86:	4770      	bx	lr

Disassembly of section .text._ZN6Screen11GetCharAddrEPhhtt:

00000000 <_ZN6Screen11GetCharAddrEPhhtt>:
   0:	b480      	push	{r7}
   2:	b085      	sub	sp, #20
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	4608      	mov	r0, r1
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	4603      	mov	r3, r0
  10:	72fb      	strb	r3, [r7, #11]
  12:	460b      	mov	r3, r1
  14:	813b      	strh	r3, [r7, #8]
  16:	4613      	mov	r3, r2
  18:	80fb      	strh	r3, [r7, #6]
  1a:	7afb      	ldrb	r3, [r7, #11]
  1c:	3b20      	subs	r3, #32
  1e:	88fa      	ldrh	r2, [r7, #6]
  20:	fb02 f303 	mul.w	r3, r2, r3
  24:	893a      	ldrh	r2, [r7, #8]
  26:	08d2      	lsrs	r2, r2, #3
  28:	b292      	uxth	r2, r2
  2a:	3201      	adds	r2, #1
  2c:	fb02 f303 	mul.w	r3, r2, r3
  30:	461a      	mov	r2, r3
  32:	68fb      	ldr	r3, [r7, #12]
  34:	4413      	add	r3, r2
  36:	4618      	mov	r0, r3
  38:	3714      	adds	r7, #20
  3a:	46bd      	mov	sp, r7
  3c:	f85d 7b04 	ldr.w	r7, [sp], #4
  40:	4770      	bx	lr

Disassembly of section .text._ZN6Screen19PushMemoryParameterERNS_10DrawMemoryEms:

00000000 <_ZN6Screen19PushMemoryParameterERNS_10DrawMemoryEms>:
   0:	b480      	push	{r7}
   2:	b087      	sub	sp, #28
   4:	af00      	add	r7, sp, #0
   6:	60f8      	str	r0, [r7, #12]
   8:	60b9      	str	r1, [r7, #8]
   a:	4613      	mov	r3, r2
   c:	80fb      	strh	r3, [r7, #6]
   e:	f9b7 3006 	ldrsh.w	r3, [r7, #6]
  12:	2b04      	cmp	r3, #4
  14:	bfa8      	it	ge
  16:	2304      	movge	r3, #4
  18:	82bb      	strh	r3, [r7, #20]
  1a:	68fb      	ldr	r3, [r7, #12]
  1c:	3310      	adds	r3, #16
  1e:	613b      	str	r3, [r7, #16]
  20:	2300      	movs	r3, #0
  22:	82fb      	strh	r3, [r7, #22]
  24:	f9b7 2016 	ldrsh.w	r2, [r7, #22]
  28:	f9b7 3014 	ldrsh.w	r3, [r7, #20]
  2c:	429a      	cmp	r2, r3
  2e:	da16      	bge.n	5e <_ZN6Screen19PushMemoryParameterERNS_10DrawMemoryEms+0x5e>
  30:	f9b7 3016 	ldrsh.w	r3, [r7, #22]
  34:	00db      	lsls	r3, r3, #3
  36:	68ba      	ldr	r2, [r7, #8]
  38:	40da      	lsrs	r2, r3
  3a:	693b      	ldr	r3, [r7, #16]
  3c:	781b      	ldrb	r3, [r3, #0]
  3e:	4619      	mov	r1, r3
  40:	b2d2      	uxtb	r2, r2
  42:	68fb      	ldr	r3, [r7, #12]
  44:	440b      	add	r3, r1
  46:	749a      	strb	r2, [r3, #18]
  48:	693b      	ldr	r3, [r7, #16]
  4a:	781b      	ldrb	r3, [r3, #0]
  4c:	3301      	adds	r3, #1
  4e:	b2da      	uxtb	r2, r3
  50:	693b      	ldr	r3, [r7, #16]
  52:	701a      	strb	r2, [r3, #0]
  54:	8afb      	ldrh	r3, [r7, #22]
  56:	3301      	adds	r3, #1
  58:	b29b      	uxth	r3, r3
  5a:	82fb      	strh	r3, [r7, #22]
  5c:	e7e2      	b.n	24 <_ZN6Screen19PushMemoryParameterERNS_10DrawMemoryEms+0x24>
  5e:	bf00      	nop
  60:	371c      	adds	r7, #28
  62:	46bd      	mov	sp, r7
  64:	f85d 7b04 	ldr.w	r7, [sp], #4
  68:	4770      	bx	lr

Disassembly of section .text._ZN6Screen10GetYBoundsEtttt:

00000000 <_ZN6Screen10GetYBoundsEtttt>:
   0:	b490      	push	{r4, r7}
   2:	b084      	sub	sp, #16
   4:	af00      	add	r7, sp, #0
   6:	4604      	mov	r4, r0
   8:	4608      	mov	r0, r1
   a:	4611      	mov	r1, r2
   c:	461a      	mov	r2, r3
   e:	4623      	mov	r3, r4
  10:	80fb      	strh	r3, [r7, #6]
  12:	4603      	mov	r3, r0
  14:	80bb      	strh	r3, [r7, #4]
  16:	460b      	mov	r3, r1
  18:	807b      	strh	r3, [r7, #2]
  1a:	4613      	mov	r3, r2
  1c:	803b      	strh	r3, [r7, #0]
  1e:	88fa      	ldrh	r2, [r7, #6]
  20:	887b      	ldrh	r3, [r7, #2]
  22:	429a      	cmp	r2, r3
  24:	d804      	bhi.n	30 <_ZN6Screen10GetYBoundsEtttt+0x30>
  26:	887a      	ldrh	r2, [r7, #2]
  28:	88fb      	ldrh	r3, [r7, #6]
  2a:	1ad3      	subs	r3, r2, r3
  2c:	b29b      	uxth	r3, r3
  2e:	e000      	b.n	32 <_ZN6Screen10GetYBoundsEtttt+0x32>
  30:	2300      	movs	r3, #0
  32:	81fb      	strh	r3, [r7, #14]
  34:	88ba      	ldrh	r2, [r7, #4]
  36:	883b      	ldrh	r3, [r7, #0]
  38:	429a      	cmp	r2, r3
  3a:	d204      	bcs.n	46 <_ZN6Screen10GetYBoundsEtttt+0x46>
  3c:	88ba      	ldrh	r2, [r7, #4]
  3e:	88fb      	ldrh	r3, [r7, #6]
  40:	1ad3      	subs	r3, r2, r3
  42:	b29b      	uxth	r3, r3
  44:	e003      	b.n	4e <_ZN6Screen10GetYBoundsEtttt+0x4e>
  46:	883a      	ldrh	r2, [r7, #0]
  48:	88fb      	ldrh	r3, [r7, #6]
  4a:	1ad3      	subs	r3, r2, r3
  4c:	b29b      	uxth	r3, r3
  4e:	81bb      	strh	r3, [r7, #12]
  50:	89fb      	ldrh	r3, [r7, #14]
  52:	813b      	strh	r3, [r7, #8]
  54:	89bb      	ldrh	r3, [r7, #12]
  56:	817b      	strh	r3, [r7, #10]
  58:	2300      	movs	r3, #0
  5a:	893a      	ldrh	r2, [r7, #8]
  5c:	f362 030f 	bfi	r3, r2, #0, #16
  60:	897a      	ldrh	r2, [r7, #10]
  62:	f362 431f 	bfi	r3, r2, #16, #16
  66:	4618      	mov	r0, r3
  68:	3710      	adds	r7, #16
  6a:	46bd      	mov	sp, r7
  6c:	bc90      	pop	{r4, r7}
  6e:	4770      	bx	lr

Disassembly of section .text._ZN6Screen19PullMemoryParameterItLb0EEET_RNS_10DrawMemoryE:

00000000 <_ZN6Screen19PullMemoryParameterItLb0EEET_RNS_10DrawMemoryE>:
   0:	b480      	push	{r7}
   2:	b087      	sub	sp, #28
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	2302      	movs	r3, #2
   a:	827b      	strh	r3, [r7, #18]
   c:	2300      	movs	r3, #0
   e:	82fb      	strh	r3, [r7, #22]
  10:	687b      	ldr	r3, [r7, #4]
  12:	3311      	adds	r3, #17
  14:	60fb      	str	r3, [r7, #12]
  16:	2300      	movs	r3, #0
  18:	82bb      	strh	r3, [r7, #20]
  1a:	f9b7 3014 	ldrsh.w	r3, [r7, #20]
  1e:	2b01      	cmp	r3, #1
  20:	dc22      	bgt.n	68 <_ZN6Screen19PullMemoryParameterItLb0EEET_RNS_10DrawMemoryE+0x68>
  22:	f9b7 3014 	ldrsh.w	r3, [r7, #20]
  26:	687a      	ldr	r2, [r7, #4]
  28:	7c12      	ldrb	r2, [r2, #16]
  2a:	4293      	cmp	r3, r2
  2c:	da1c      	bge.n	68 <_ZN6Screen19PullMemoryParameterItLb0EEET_RNS_10DrawMemoryE+0x68>
  2e:	68fb      	ldr	r3, [r7, #12]
  30:	781b      	ldrb	r3, [r3, #0]
  32:	461a      	mov	r2, r3
  34:	687b      	ldr	r3, [r7, #4]
  36:	4413      	add	r3, r2
  38:	7c9b      	ldrb	r3, [r3, #18]
  3a:	461a      	mov	r2, r3
  3c:	f9b7 3014 	ldrsh.w	r3, [r7, #20]
  40:	00db      	lsls	r3, r3, #3
  42:	fa02 f303 	lsl.w	r3, r2, r3
  46:	b21a      	sxth	r2, r3
  48:	f9b7 3016 	ldrsh.w	r3, [r7, #22]
  4c:	4313      	orrs	r3, r2
  4e:	b21b      	sxth	r3, r3
  50:	82fb      	strh	r3, [r7, #22]
  52:	68fb      	ldr	r3, [r7, #12]
  54:	781b      	ldrb	r3, [r3, #0]
  56:	3301      	adds	r3, #1
  58:	b2da      	uxtb	r2, r3
  5a:	68fb      	ldr	r3, [r7, #12]
  5c:	701a      	strb	r2, [r3, #0]
  5e:	8abb      	ldrh	r3, [r7, #20]
  60:	3301      	adds	r3, #1
  62:	b29b      	uxth	r3, r3
  64:	82bb      	strh	r3, [r7, #20]
  66:	e7d8      	b.n	1a <_ZN6Screen19PullMemoryParameterItLb0EEET_RNS_10DrawMemoryE+0x1a>
  68:	68fb      	ldr	r3, [r7, #12]
  6a:	781a      	ldrb	r2, [r3, #0]
  6c:	687b      	ldr	r3, [r7, #4]
  6e:	7c1b      	ldrb	r3, [r3, #16]
  70:	429a      	cmp	r2, r3
  72:	d302      	bcc.n	7a <_ZN6Screen19PullMemoryParameterItLb0EEET_RNS_10DrawMemoryE+0x7a>
  74:	68fb      	ldr	r3, [r7, #12]
  76:	2200      	movs	r2, #0
  78:	701a      	strb	r2, [r3, #0]
  7a:	8afb      	ldrh	r3, [r7, #22]
  7c:	4618      	mov	r0, r3
  7e:	371c      	adds	r7, #28
  80:	46bd      	mov	sp, r7
  82:	f85d 7b04 	ldr.w	r7, [sp], #4
  86:	4770      	bx	lr

Disassembly of section .text._ZN6Screen19PullMemoryParameterImLb0EEET_RNS_10DrawMemoryE:

00000000 <_ZN6Screen19PullMemoryParameterImLb0EEET_RNS_10DrawMemoryE>:
   0:	b480      	push	{r7}
   2:	b087      	sub	sp, #28
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	2304      	movs	r3, #4
   a:	823b      	strh	r3, [r7, #16]
   c:	2300      	movs	r3, #0
   e:	617b      	str	r3, [r7, #20]
  10:	687b      	ldr	r3, [r7, #4]
  12:	3311      	adds	r3, #17
  14:	60fb      	str	r3, [r7, #12]
  16:	2300      	movs	r3, #0
  18:	827b      	strh	r3, [r7, #18]
  1a:	f9b7 3012 	ldrsh.w	r3, [r7, #18]
  1e:	2b03      	cmp	r3, #3
  20:	dc20      	bgt.n	64 <_ZN6Screen19PullMemoryParameterImLb0EEET_RNS_10DrawMemoryE+0x64>
  22:	f9b7 3012 	ldrsh.w	r3, [r7, #18]
  26:	687a      	ldr	r2, [r7, #4]
  28:	7c12      	ldrb	r2, [r2, #16]
  2a:	4293      	cmp	r3, r2
  2c:	da1a      	bge.n	64 <_ZN6Screen19PullMemoryParameterImLb0EEET_RNS_10DrawMemoryE+0x64>
  2e:	68fb      	ldr	r3, [r7, #12]
  30:	781b      	ldrb	r3, [r3, #0]
  32:	461a      	mov	r2, r3
  34:	687b      	ldr	r3, [r7, #4]
  36:	4413      	add	r3, r2
  38:	7c9b      	ldrb	r3, [r3, #18]
  3a:	461a      	mov	r2, r3
  3c:	f9b7 3012 	ldrsh.w	r3, [r7, #18]
  40:	00db      	lsls	r3, r3, #3
  42:	fa02 f303 	lsl.w	r3, r2, r3
  46:	461a      	mov	r2, r3
  48:	697b      	ldr	r3, [r7, #20]
  4a:	4313      	orrs	r3, r2
  4c:	617b      	str	r3, [r7, #20]
  4e:	68fb      	ldr	r3, [r7, #12]
  50:	781b      	ldrb	r3, [r3, #0]
  52:	3301      	adds	r3, #1
  54:	b2da      	uxtb	r2, r3
  56:	68fb      	ldr	r3, [r7, #12]
  58:	701a      	strb	r2, [r3, #0]
  5a:	8a7b      	ldrh	r3, [r7, #18]
  5c:	3301      	adds	r3, #1
  5e:	b29b      	uxth	r3, r3
  60:	827b      	strh	r3, [r7, #18]
  62:	e7da      	b.n	1a <_ZN6Screen19PullMemoryParameterImLb0EEET_RNS_10DrawMemoryE+0x1a>
  64:	68fb      	ldr	r3, [r7, #12]
  66:	781a      	ldrb	r2, [r3, #0]
  68:	687b      	ldr	r3, [r7, #4]
  6a:	7c1b      	ldrb	r3, [r3, #16]
  6c:	429a      	cmp	r2, r3
  6e:	d302      	bcc.n	76 <_ZN6Screen19PullMemoryParameterImLb0EEET_RNS_10DrawMemoryE+0x76>
  70:	68fb      	ldr	r3, [r7, #12]
  72:	2200      	movs	r2, #0
  74:	701a      	strb	r2, [r3, #0]
  76:	697b      	ldr	r3, [r7, #20]
  78:	4618      	mov	r0, r3
  7a:	371c      	adds	r7, #28
  7c:	46bd      	mov	sp, r7
  7e:	f85d 7b04 	ldr.w	r7, [sp], #4
  82:	4770      	bx	lr

Disassembly of section .text._ZN6Screen19PullMemoryParameterIhLb0EEET_RNS_10DrawMemoryE:

00000000 <_ZN6Screen19PullMemoryParameterIhLb0EEET_RNS_10DrawMemoryE>:
   0:	b480      	push	{r7}
   2:	b087      	sub	sp, #28
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	2301      	movs	r3, #1
   a:	827b      	strh	r3, [r7, #18]
   c:	2300      	movs	r3, #0
   e:	75fb      	strb	r3, [r7, #23]
  10:	687b      	ldr	r3, [r7, #4]
  12:	3311      	adds	r3, #17
  14:	60fb      	str	r3, [r7, #12]
  16:	2300      	movs	r3, #0
  18:	82bb      	strh	r3, [r7, #20]
  1a:	f9b7 3014 	ldrsh.w	r3, [r7, #20]
  1e:	2b00      	cmp	r3, #0
  20:	dc22      	bgt.n	68 <_ZN6Screen19PullMemoryParameterIhLb0EEET_RNS_10DrawMemoryE+0x68>
  22:	f9b7 3014 	ldrsh.w	r3, [r7, #20]
  26:	687a      	ldr	r2, [r7, #4]
  28:	7c12      	ldrb	r2, [r2, #16]
  2a:	4293      	cmp	r3, r2
  2c:	da1c      	bge.n	68 <_ZN6Screen19PullMemoryParameterIhLb0EEET_RNS_10DrawMemoryE+0x68>
  2e:	68fb      	ldr	r3, [r7, #12]
  30:	781b      	ldrb	r3, [r3, #0]
  32:	461a      	mov	r2, r3
  34:	687b      	ldr	r3, [r7, #4]
  36:	4413      	add	r3, r2
  38:	7c9b      	ldrb	r3, [r3, #18]
  3a:	461a      	mov	r2, r3
  3c:	f9b7 3014 	ldrsh.w	r3, [r7, #20]
  40:	00db      	lsls	r3, r3, #3
  42:	fa02 f303 	lsl.w	r3, r2, r3
  46:	b25a      	sxtb	r2, r3
  48:	f997 3017 	ldrsb.w	r3, [r7, #23]
  4c:	4313      	orrs	r3, r2
  4e:	b25b      	sxtb	r3, r3
  50:	75fb      	strb	r3, [r7, #23]
  52:	68fb      	ldr	r3, [r7, #12]
  54:	781b      	ldrb	r3, [r3, #0]
  56:	3301      	adds	r3, #1
  58:	b2da      	uxtb	r2, r3
  5a:	68fb      	ldr	r3, [r7, #12]
  5c:	701a      	strb	r2, [r3, #0]
  5e:	8abb      	ldrh	r3, [r7, #20]
  60:	3301      	adds	r3, #1
  62:	b29b      	uxth	r3, r3
  64:	82bb      	strh	r3, [r7, #20]
  66:	e7d8      	b.n	1a <_ZN6Screen19PullMemoryParameterIhLb0EEET_RNS_10DrawMemoryE+0x1a>
  68:	68fb      	ldr	r3, [r7, #12]
  6a:	781a      	ldrb	r2, [r3, #0]
  6c:	687b      	ldr	r3, [r7, #4]
  6e:	7c1b      	ldrb	r3, [r3, #16]
  70:	429a      	cmp	r2, r3
  72:	d302      	bcc.n	7a <_ZN6Screen19PullMemoryParameterIhLb0EEET_RNS_10DrawMemoryE+0x7a>
  74:	68fb      	ldr	r3, [r7, #12]
  76:	2200      	movs	r2, #0
  78:	701a      	strb	r2, [r3, #0]
  7a:	7dfb      	ldrb	r3, [r7, #23]
  7c:	4618      	mov	r0, r3
  7e:	371c      	adds	r7, #28
  80:	46bd      	mov	sp, r7
  82:	f85d 7b04 	ldr.w	r7, [sp], #4
  86:	4770      	bx	lr
