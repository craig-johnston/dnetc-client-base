; ARM (8KB cache) version of the bitslicing
; DES core, incorporating the s box and deseval functions.
;
; Optimised for ARM by Chris Berry and Steve Lee,
; based on deseval.c from Matthew Kwan's bitslicing DES key search.
;
; $Log: des-slice-arm.s,v $
; Revision 1.13  1999/12/07 23:44:25  cyp
; standardized calling conventions, converted nbbits parameter to iterstodo
; (cores return effective iterstodo), removed MIN_DES_BITS/MAX_DES_BITS gunk,
; removed BIT_32/BIT_64 craziness.
;
; Revision 1.12.2.1  1999/11/24 19:11:12  chrisb
; miscellaneous RISC OS changes
;
; Revision 1.12  1998/06/26 07:43:02  cberry
; Yet more new cores from Steve Lee.
;
; Revision 1.11  1998/06/24 08:51:41  kbracey
; New DES code from Steve Lee
;
; Revision 1.10  1998/06/23 13:56:00  kbracey
; New ARM DES cores - calling convention changed to add a few %
;
; Revision 1.9  1998/06/15 12:04:10  kbracey
; Lots of consts.
;
; Revision 1.8  1998/06/15 10:15:38  kbracey
; New ARM sboxes from Steve Lee based on the latest Kwan sboxes.
;
; Revision 1.7  1998/06/14 10:40:55  friedbait
; 'Log' keywords added
;

	AREA	fastdesarea, CODE, READONLY

        DCB     "@(#)$Id: des-slice-arm.s,v 1.13 1999/12/07 23:44:25 cyp Exp $", 0
        ALIGN

        EXPORT	des_unit_func_arm_asm
	IMPORT	convert_key_from_des_to_inc__FPUiT1
	IMPORT	convert_key_from_inc_to_des__FPUiT1

	GBLL	patch
patch	SETL	{FALSE}


 [ patch
startofcode	; &2c000

	DCD	&b7e15163

	B	des_unit_func_arm_asm

__rt_udiv	B	startofcode - &2c000 + &1b938
__rt_sdiv	B	startofcode - &2c000 + &1b8d8
__rt_stkovf_split_big	B	startofcode - &2c000 + &1bc8c
 |
	IMPORT	__rt_udiv
	IMPORT	__rt_sdiv
	IMPORT	__rt_stkovf_split_big
 ]


; these functiona do not follow APCS.
; a1-a6 (r0-r5) are the inputs.
; x1-x6 are same registers as a1-a6. Considered available for
;       general use after last access of a1-a6.
; t1-t8 are used as temporaries.

;a1 RN 0
x1 RN 0
;a2 RN 1
x2 RN 1
;a3 RN 2
x3 RN 2
;a4 RN 3
x4 RN 3
a5 RN 4
x5 RN 4
a6 RN 5
x6 RN 5

t1 RN 6
t2 RN 7
t3 RN 8
t4 RN 9
t5 RN 10
t6 RN 11
t7 RN 12
t8 RN 14


	GBLL	loadr14

	MACRO
	s_start	$s, $inp1, $r14unused
	LCLS	reglist
	LCLS	reg
	LCLS	reg2
	LCLA	i
s$s
 [ $r14unused = 0
loadr14	SETL	{TRUE}
  [ $inp1 = 0
	STMFD	r13!,{t5,r14}
  |
	STR	r14,[r13,#-4]!
  ]
 |
loadr14	SETL	{FALSE}
  [ $inp1 = 0
	STR	t5,[r13,#-4]!
  ]
 ]

reglist	SETS	"t1,t2,t3,t4,t6,t7"

 [ $s = 1
reg	SETS	reglist:LEFT:2
	LDR	$reg,[t7,#4*31]
reg	SETS	reglist:RIGHT:14
	LDMIA	t7,{$reg}
 |
  [ $s = 8
reg	SETS	reglist:LEFT:14
	LDMIA	t7,{$reg}
reg	SETS	reglist:RIGHT:2
	LDR	$reg,[t7,#4*(-27)]
  |
	LDMIA	t7,{$reglist}
  ]
 ]

reglist	SETS	reglist:CC:","
i	SETA	1

	WHILE	i <= 6
reg	SETS	"a":CC:("$i":RIGHT:1)
reg2	SETS	reglist:LEFT:2
reglist	SETS	reglist:RIGHT:(:LEN:reglist - 3)
	EOR	$reg,$reg,$reg2
i	SETA	i+1
	WEND

	MEND


	MACRO
	s_end	$s, $got1, $got2, $got3, $got4

 [ loadr14
	LDR	t8,[r13],#4
 ]
	B	s_call_loop

	GBLA	got1_$s
	GBLA	got2_$s
	GBLA	got3_$s
	GBLA	got4_$s
got1_$s	SETA	$got1
got2_$s	SETA	$got2
got3_$s	SETA	$got3
got4_$s	SETA	$got4

	MEND




;        AREA |C$$code|, CODE, READONLY

;|x$codeseg| DATA




lowbits
	DCD	0xaaaaaaaa
	DCD	0xcccccccc
	DCD	0xf0f0f0f0
	DCD	0xff00ff00
	DCD	0xffff0000


des_unit_func_arm_asm
        MOV      r12,r13
        STMDB    r13!,{r0,r1,r4-r9,r11,r12,lr,pc}
        SUB      r11,r12,#4
        SUB      r12,r13,#&400
        CMP      r12,r10
        BLMI     __rt_stkovf_split_big
        MOV      r4,r0
        SUB      r13,r13,#&2e8
        LDR      r0,[r0,#&10]
        STR      r0,[r13,#4]
        LDR      r0,[r4,#&14]
        STR      r0,[r13,#0]
        MOV      r1,r13
        ADD      r0,r13,#4
        BL       convert_key_from_inc_to_des__FPUiT1
        MOV      r5,#1
        MOV      r6,#0
        LDMIA    r13,{r7,r8}
|L000048.J5|
        MOV      r1,r6
        MOV      r0,#7
        BL       __rt_udiv
        CMP      r1,#0
        MOVEQ    r5,r5,LSL #1
        ANDS     r1,r7,r5
        MVNNE    r1,#0
        ADD      r0,r13,#&208
        STR      r1,[r0,r6,LSL #2]
        MOVS     r5,r5,LSL #1
        MOVEQ    r7,r8
        MOVEQ    r5,#1
        ADD      r6,r6,#1
        CMP      r6,#&38
        BCC      |L000048.J5|

; zero area to be used for plain and cypher bits.
; this allows expandbit macro to be 2/3 the size...
	MOV	r5,#0
	MOV	r6,#0
	MOV	r1,#32
	ADD	r12,r13,#&208
clearbits
	STMDB	r12!,{r5-r6}
	STMDB	r12!,{r5-r6}
	SUBS	r1,r1,#1
	BNE	clearbits

        LDR      r2,[r4,#&c]
	LDR	r3,[r4,#8]
        ADD      r12,r13,#8
	MVN	r7,#0

	MACRO
	expandbit	$to, $from
 [ $from < 32
	TST	r2,#1<<$from
 |
	TST	r3,#1<<($from-32)
 ]
	STRNE	r7,[r12,#4*$to]

	MEND

	expandbit	63, 5
	expandbit	62, 3
	expandbit	61, 51
	expandbit	60, 49
	expandbit	59, 37
	expandbit	58, 25
	expandbit	57, 15
	expandbit	56, 11
	expandbit	55, 59
	expandbit	54, 61
	expandbit	53, 41
	expandbit	52, 47
	expandbit	51, 9
	expandbit	50, 27
	expandbit	49, 13
	expandbit	48, 7
	expandbit	47, 63
	expandbit	46, 45
	expandbit	45, 1
	expandbit	44, 23
	expandbit	43, 31
	expandbit	42, 33
	expandbit	41, 21
	expandbit	40, 19
	expandbit	39, 57
	expandbit	38, 29
	expandbit	37, 43
	expandbit	36, 55
	expandbit	35, 39
	expandbit	34, 17
	expandbit	33, 53
	expandbit	32, 35
	expandbit	31, 4
	expandbit	30, 2
	expandbit	29, 50
	expandbit	28, 48
	expandbit	27, 36
	expandbit	26, 24
	expandbit	25, 14
	expandbit	24, 10
	expandbit	23, 58
	expandbit	22, 60
	expandbit	21, 40
	expandbit	20, 46
	expandbit	19, 8
	expandbit	18, 26
	expandbit	17, 12
	expandbit	16, 6
	expandbit	15, 62
	expandbit	14, 44
	expandbit	13, 0
	expandbit	12, 22
	expandbit	11, 30
	expandbit	10, 32
	expandbit	 9, 20
	expandbit	 8, 18
	expandbit	 7, 56
	expandbit	 6, 28
	expandbit	 5, 42
	expandbit	 4, 54
	expandbit	 3, 38
	expandbit	 2, 16
	expandbit	 1, 52
	expandbit	 0, 34

        LDR      r2,[r4,#4]
         LDR    r3,[r4,#0]
       ADD      r12,r13,#&108

	expandbit	63, 57
	expandbit	62, 49
	expandbit	61, 41
	expandbit	60, 33
	expandbit	59, 25
	expandbit	58, 17
	expandbit	57,  9
	expandbit	56,  1
	expandbit	55, 59
	expandbit	54, 51
	expandbit	53, 43
	expandbit	52, 35
	expandbit	51, 27
	expandbit	50, 19
	expandbit	49, 11
	expandbit	48,  3
	expandbit	47, 61
	expandbit	46, 53
	expandbit	45, 45
	expandbit	44, 37
	expandbit	43, 29
	expandbit	42, 21
	expandbit	41, 13
	expandbit	40,  5
	expandbit	39, 63
	expandbit	38, 55
	expandbit	37, 47
	expandbit	36, 39
	expandbit	35, 31
	expandbit	34, 23
	expandbit	33, 15
	expandbit	32,  7
	expandbit	31, 56
	expandbit	30, 48
	expandbit	29, 40
	expandbit	28, 32
	expandbit	27, 24
	expandbit	26, 16
	expandbit	25,  8
	expandbit	24,  0
	expandbit	23, 58
	expandbit	22, 50
	expandbit	21, 42
	expandbit	20, 34
	expandbit	19, 26
	expandbit	18, 18
	expandbit	17, 10
	expandbit	16,  2
	expandbit	15, 60
	expandbit	14, 52
	expandbit	13, 44
	expandbit	12, 36
	expandbit	11, 28
	expandbit	10, 20
	expandbit	 9, 12
	expandbit	 8,  4
	expandbit	 7, 62
	expandbit	 6, 54
	expandbit	 5, 46
	expandbit	 4, 38
	expandbit	 3, 30
	expandbit	 2, 22
	expandbit	 1, 14
	expandbit	 0,  6




	ADRL	r0,lowbits
	LDMIA	r0,{r5-r9}
        STR      r5,[r13,#&214]
        STR      r6,[r13,#&21c]
        STR      r7,[r13,#&228]
        STR      r8,[r13,#&230]
        STR      r9,[r13,#&234]
        LDR      r0,[r11,#-&28]
        SUB      r5,r0,#5


        MOV      r8,#0
        MOV      r7,#0
        MOV      r6,#0
        MOV      r0,#1
        MOV      r9,r0,LSL r5

resettwiddles
        MOV      r0,#0
        MOV      r12,#0
        ADR      r3,twiddles
        ADD      r2,r13,#&208
resettwiddleloop
        LDRB     r1,[r3,r0]
        STR      r12,[r2,r1,LSL #2]
        ADD      r0,r0,#1
        CMP      r0,r5
        BCC      resettwiddleloop
        B        deseval

half_keys_done
        CMP      r8,#0
        BNE      inc_then_exit
        MOV      r8,#1
        MOV      r6,#0
        MOV      r7,r6
        MOV      r0,#&38
        ADD      r1,r13,#&208
invertloop
        LDR      r2,[r1]
        SUBS     r0,r0,#1
        MVN      r2,r2
        STR      r2,[r1],#4
        BNE      invertloop
	B	resettwiddles

inc_then_exit
        LDR      r1,[r4,#&14]
        MOV      r2,#1
        LDR      r0,[r11,#-&28]
        MOV      r0,r2,LSL r0
        ADD      r1,r1,r0
        STR      r1,[r4,#&14]
        LDMDB    r11,{r4-r9,r11,r13,pc}^

odd_parity
        DCB      0x01,0x01,0x02,0x02
        DCB      0x04,0x04,0x07,0x07
        DCB      0x08,0x08,0x0b,0x0b
        DCB      0x0d,0x0d,0x0e,0x0e
        DCB      0x10,0x10,0x13,0x13
        DCB      0x15,0x15,0x16,0x16
        DCB      0x19,0x19,0x1a,0x1a
        DCB      0x1c,0x1c,0x1f,0x1f
        DCB      0x20,0x20,0x23,0x23
        DCB      0x25,0x25,0x26,0x26
        DCB      0x29,0x29,0x2a,0x2a
        DCB      0x2c,0x2c,0x2f,0x2f
        DCB      0x31,0x31,0x32,0x32
        DCB      0x34,0x34,0x37,0x37
        DCB      0x38,0x38,0x3b,0x3b
        DCB      0x3d,0x3d,0x3e,0x3e
        DCB      0x40,0x40,0x43,0x43
        DCB      0x45,0x45,0x46,0x46
        DCB      0x49,0x49,0x4a,0x4a
        DCB      0x4c,0x4c,0x4f,0x4f
        DCB      0x51,0x51,0x52,0x52
        DCB      0x54,0x54,0x57,0x57
        DCB      0x58,0x58,0x5b,0x5b
        DCB      0x5d,0x5d,0x5e,0x5e
        DCB      0x61,0x61,0x62,0x62
        DCB      0x64,0x64,0x67,0x67
        DCB      0x68,0x68,0x6b,0x6b
        DCB      0x6d,0x6d,0x6e,0x6e
        DCB      0x70,0x70,0x73,0x73
        DCB      0x75,0x75,0x76,0x76
        DCB      0x79,0x79,0x7a,0x7a
        DCB      0x7c,0x7c,0x7f,0x7f
        DCB      0x80,0x80,0x83,0x83
        DCB      0x85,0x85,0x86,0x86
        DCB      0x89,0x89,0x8a,0x8a
        DCB      0x8c,0x8c,0x8f,0x8f
        DCB      0x91,0x91,0x92,0x92
        DCB      0x94,0x94,0x97,0x97
        DCB      0x98,0x98,0x9b,0x9b
        DCB      0x9d,0x9d,0x9e,0x9e
        DCB      0xa1,0xa1,0xa2,0xa2
        DCB      0xa4,0xa4,0xa7,0xa7
        DCB      0xa8,0xa8,0xab,0xab
        DCB      0xad,0xad,0xae,0xae
        DCB      0xb0,0xb0,0xb3,0xb3
        DCB      0xb5,0xb5,0xb6,0xb6
        DCB      0xb9,0xb9,0xba,0xba
        DCB      0xbc,0xbc,0xbf,0xbf
        DCB      0xc1,0xc1,0xc2,0xc2
        DCB      0xc4,0xc4,0xc7,0xc7
        DCB      0xc8,0xc8,0xcb,0xcb
        DCB      0xcd,0xcd,0xce,0xce
        DCB      0xd0,0xd0,0xd3,0xd3
        DCB      0xd5,0xd5,0xd6,0xd6
        DCB      0xd9,0xd9,0xda,0xda
        DCB      0xdc,0xdc,0xdf,0xdf
        DCB      0xe0,0xe0,0xe3,0xe3
        DCB      0xe5,0xe5,0xe6,0xe6
        DCB      0xe9,0xe9,0xea,0xea
        DCB      0xec,0xec,0xef,0xef
        DCB      0xf1,0xf1,0xf2,0xf2
        DCB      0xf4,0xf4,0xf7,0xf7
        DCB      0xf8,0xf8,0xfb,0xfb
        DCB      0xfd,0xfd,0xfe,0xfe

twiddles
        DCB      0x0c,0x0f,0x12,0x28
        DCB      0x29,0x2a,0x2b,0x2d
        DCB      0x2e,0x31,0x32,0x00
        DCB      0x01,0x02,0x04,0x06
        DCB      0x07,0x09,0x0d,0x00


foundkey
        MVN      r5,#0
        MOV      r1,#0
        MOV      r2,#1
|L000228.J51|
        TST      r0,r2,LSL r1
        MOVNE    r5,r1
        ADD      r1,r1,#1
        CMP      r1,#&20
        BCC      |L000228.J51|
        MOV      r0,#0
        STR      r0,[r13,#4]
        MOV      r6,#49
        STR      r0,[r13,#0]
        ADR      r9,odd_parity
|L000250.J55|
        ADD      r0,r13,#&208
        ADD      r0,r0,r6,LSL #2
        LDR      r1,[r0,#6*4]
        MOV      r1,r1,LSR r5
        AND      r1,r1,#1
        MOV      r2,r1,LSL #7
        LDR      r1,[r0,#5*4]
        MOV      r1,r1,LSR r5
        AND      r1,r1,#1
        ORR      r2,r2,r1,LSL #6
        LDR      r1,[r0,#4*4]
        MOV      r1,r1,LSR r5
        AND      r1,r1,#1
        ORR      r2,r2,r1,LSL #5
        LDR      r1,[r0,#3*4]
        MOV      r1,r1,LSR r5
        AND      r1,r1,#1
        ORR      r2,r2,r1,LSL #4
        LDR      r1,[r0,#2*4]
        MOV      r1,r1,LSR r5
        AND      r1,r1,#1
        ORR      r2,r2,r1,LSL #3
        LDR      r1,[r0,#1*4]
        MOV      r1,r1,LSR r5
        AND      r1,r1,#1
        ORR      r1,r2,r1,LSL #2
        LDR      r0,[r0,#0*4]
        MOV      r0,r0,LSR r5
        AND      r0,r0,#1
        ORR      r0,r1,r0,LSL #1
        LDRB     r7,[r9,r0]
        ADD      r1,r6,#7
        MOV      r0,#7
        BL       __rt_sdiv
        SUB      r0,r0,#1
        CMP      r0,#4
        BLT      |L000300.J58|
        MVN      r1,#&1f
        ADD      r0,r1,r0,LSL #3
        LDR      r1,[r13,#4]
        ORR      r0,r1,r7,LSL r0
        STR      r0,[r13,#4]
        B        |L000310.J60|
|L000300.J58|
        MOV      r0,r0,LSL #3
        LDR      r1,[r13,#0]
        ORR      r0,r1,r7,LSL r0
        STR      r0,[r13,#0]
|L000310.J60|
        SUB      r6,r6,#7
        CMP      r6,#0
        BGE      |L000250.J55|
        CMP      r8,#0
        BEQ      |L00033c.J62|
        LDR      r0,[r13,#4]
        MVN      r0,r0
        STR      r0,[r13,#4]
        LDR      r0,[r13,#0]
        MVN      r0,r0
        STR      r0,[r13,#0]
|L00033c.J62|
        MOV      r1,r13
        ADD      r0,r13,#4
        BL       convert_key_from_des_to_inc__FPUiT1
        LDR      r0,[r4,#&14]
        LDR      r1,[r13,#0]
        SUB      r0,r1,r0
        STR      r1,[r4,#&14]
        LDR      r1,[r13,#4]
        STR      r1,[r4,#&10]
        LDMDB    r11,{r4-r9,r11,r13,pc}^








cachedcode_start

	GET	kwan-sboxes-arm.h

timingloop
        ADD      r2,r13,#&208
        LDR      r0,[r2,r1,LSL #2]
        MVN      r0,r0
        STR      r0,[r2,r1,LSL #2]
deseval
        ADD      r1,r13,#8

	MVN	r3,#0
	STMFD	r13!,{r1-r11}

	LDMDB	r2!,{r4-r11}
	STMFD	r13!,{r4-r11}
	LDMDB	r2!,{r4-r11}
	STMFD	r13!,{r4-r11}
	LDMDB	r2!,{r4-r11}
	STMFD	r13!,{r4-r11}
	LDMDB	r2!,{r4-r11}
	STMFD	r13!,{r4-r11}
	LDMDB	r2!,{r4-r11}
	STMFD	r13!,{r4-r11}
	LDMDB	r2!,{r4-r11}
	STMFD	r13!,{r4-r11}
	LDMDB	r2!,{r4-r11}
	STMFD	r13!,{r4-r11}
	LDMDB	r2!,{r4-r11}
	STMFD	r13!,{r4-r11}



	MACRO
	do_s	$s,$i1,$i2,$i3,$i4,$i5,$i6,$o
	LCLA	offset

	DCD	($i1<<26)+($i2<<18)+($i3<<10)+($i4<<2)
 [ $s = 1
offset	SETA	((1-$o)<<7)
 |
offset	SETA	((1-$o)<<7)+(($s-1)<<4)-(4)
 ]
	DCD	($i5<<26)+($i6<<18)+(offset<<8)+(($o)<<0)+(($s-1)<<4)

	MEND
; parameters for s box call stored as follows:

; word 0:  111111-- 222222-- 333333-- 444444-0
; word 1:  555555-- 666666-- oooooooo -sss---A

; key: s : sbox number
; A      : top bit of &1-&4.
; 1-6    : bits for i1-i6
; 0      : identification bit
; -      : zero padding bits.
; o      : offset to callee loaded half of a1-a6.

	ADR	t8, s_param_table

s_call_loop
	LDR	t6,[t8],#4

; is this an s-box call or a check?
	TST	t6,#&00000001
	BNE	do_check

donecheck
; load values of inputs
	LDR	t4,[r13,#4*(64+1)]
	LDR	t1,[t8],#4
	LDR	a1,[t4,t6,LSR #24]
	BIC	t6,t6,#&ff000000
	LDR	a2,[t4,t6,LSR #16]
	BIC	t6,t6,#&00ff0000
	LDR	a3,[t4,t6,LSR #8]
	BIC	t6,t6,#&0000ff00
	LDR	a4,[t4,t6]

	LDR	a5,[t4,t1,LSR #24]
	BIC	t1,t1,#&ff000000
	LDR	a6,[t4,t1,LSR #16]
	BIC	t1,t1,#&00ff0000

; create pointer to callee loaded stuff
	ADD	t7,r13,t1,LSR #8

; calculate address of first output
	AND	t5,t1,#&00000001
	ADD	t5,r13,t5,LSL#7

; no need to provide return address - it is always s_call_loop,
; so s_boxes jump directly to it.
	AND	t1,t1,#&00000070
	LDR	pc,[pc,t1,LSR #2]

	DCD	&12345678

 [ patch
	DCD	s1 - startofcode + &2c000
	DCD	s2 - startofcode + &2c000
	DCD	s3 - startofcode + &2c000
	DCD	s4 - startofcode + &2c000
	DCD	s5 - startofcode + &2c000
	DCD	s6 - startofcode + &2c000
	DCD	s7 - startofcode + &2c000
	DCD	s8 - startofcode + &2c000
 |
	DCD	s1
	DCD	s2
	DCD	s3
	DCD	s4
	DCD	s5
	DCD	s6
	DCD	s7
	DCD	s8
 ]

	MACRO
	check	$s, $coff, $r1, $r2, $r3, $r4

	LCLA	value
value	SETA	(1<<0)+(($coff/4)<<27)+((1-(($r1)>>5))<<1)
 [ got1_$s = 0
value	SETA	value+((($r1):OR:32)<<20)
 ]
 [ got2_$s = 0
value	SETA	value+((($r2):OR:32)<<14)
 ]
 [ got3_$s = 0
value	SETA	value+((($r3):OR:32)<<8)
 ]
 [ got4_$s = 0
value	SETA	value+((($r4):OR:32)<<2)
 ]
	DCD	value
	MEND

; parameters for check stored as follows:

; word 0 :  -nnnn-Aa aaaaBbbb bbCccccc DdddddL1

; 1   : identification bit
; A-D : flag - zero if value already available
; a-d ; zero if coresponding A-D zero, otherwise offset to output addr.
; L   ; 1 - top bit of a-d.
; n   ; offset to relevant section of cyphertext.


do_check
	AND	a6,t6,#&00000002
	SUB	a6,r13,a6,LSL#6
	ANDS	a5,t6,#&03f00000
	LDRNE	t1,[a6,a5,LSR #18]
	ANDS	a5,t6,#&000fc000
	LDRNE	t2,[a6,a5,LSR #12]
	ANDS	a5,t6,#&3f00
	LDRNE	t3,[a6,a5,LSR #6]
	ANDS	a5,t6,#&00fc
	LDRNE	t4,[a6,a5]

	LDR	a2,[r13,#4*64]
	AND	a5,t6,#&78000000
	ADD	a2,a2,a5,LSR #23
	LDMIA	a2,{a3,a4,a5,a6}
	LDR	a1,[r13,#4*(64+2)]
	EOR	a6,a6,t1
	BIC	a1,a1,a6
	EOR	a5,a5,t2
	BIC	a1,a1,a5
	EOR	a4,a4,t3
	BIC	a1,a1,a4
	EOR	a3,a3,t4
	BICS	a1,a1,a3
	BEQ	gotresult

	STR	a1,[r13,#4*(64+2)]
	LDR	t6,[t8],#4
	TST	t6,#&00000001
	BEQ	donecheck

gotresult
	ADD	r13,r13,#4*(64+3)
	LDMIA	r13!,{r4-r11}
deseval_end

;        CMP      r0,#0   (BICS in deseval bit)
        BNE      foundkey
	EOR	r6,r7,r7,LSR #1
        ADD      r7,r7,#1
        CMP      r7,r9
        BCS      half_keys_done
        EOR      r1,r7,r7,LSR #1
        EOR      r1,r1,r6

	MACRO
	checktwiddle	$bit, $twiddle
        TST	r1,#1<<$bit
	MOVNE	r1,#$twiddle
	BNE	timingloop
	MEND

	checktwiddle	0,12
	checktwiddle	1,15
	checktwiddle	2,18
	checktwiddle	3,40
	checktwiddle	4,41
	checktwiddle	5,42
	checktwiddle	6,43
	checktwiddle	7,45
	checktwiddle	8,46
	checktwiddle	9,49
	checktwiddle	10,50
	checktwiddle	11,0
	checktwiddle	12,1
	checktwiddle	13,2
	checktwiddle	14,4
	checktwiddle	15,6
	checktwiddle	16,7
	checktwiddle	17,9
	checktwiddle	18,13
	; timeslice is too big... what shall we do???
	; des-slice.c++ twiddles the wrong bit. That seems unacceptable,
	; as it will claim to check keys it hasn't but in a manner that
	; wouldn't show up on -test or on blocks from a test port.


	; the following is an illegal instruction on all current ARMs
	; While this is perhaps a bad idea, at least it won't give
	; an incorrect result

	DCD	&E6000010
this_shouldnt_happen
	; and just in case it becomes legal - here's an infinite loop...
	B	this_shouldnt_happen

s_param_table
	do_s	1, 47, 11, 26,  3, 13, 41, 0
	do_s	2, 27,  6, 54, 48, 39, 19, 0
	do_s	3, 53, 25, 33, 34, 17,  5, 0
	do_s	4,  4, 55, 24, 32, 40, 20, 0
	do_s	5, 36, 31, 21,  8, 23, 52, 0
	do_s	6, 14, 29, 51,  9, 35, 30, 0
	do_s	7,  2, 37, 22,  0, 42, 38, 0
	do_s	8, 16, 43, 44,  1,  7, 28, 0
	do_s	1, 54, 18, 33, 10, 20, 48, 1
	do_s	2, 34, 13,  4, 55, 46, 26, 1
	do_s	3,  3, 32, 40, 41, 24, 12, 1
	do_s	4, 11,  5,  6, 39, 47, 27, 1
	do_s	5, 43, 38, 28, 15, 30,  0, 1
	do_s	6, 21, 36, 31, 16, 42, 37, 1
	do_s	7,  9, 44, 29,  7, 49, 45, 1
	do_s	8, 23, 50, 51,  8, 14, 35, 1
	do_s	1, 11, 32, 47, 24, 34,  5, 0
	do_s	2, 48, 27, 18, 12,  3, 40, 0
	do_s	3, 17, 46, 54, 55, 13, 26, 0
	do_s	4, 25, 19, 20, 53,  4, 41, 0
	do_s	5,  2, 52, 42, 29, 44, 14, 0
	do_s	6, 35, 50, 45, 30,  1, 51, 0
	do_s	7, 23, 31, 43, 21,  8,  0, 0
	do_s	8, 37,  9, 38, 22, 28, 49, 0
	do_s	1, 25, 46,  4, 13, 48, 19, 1
	do_s	2,  5, 41, 32, 26, 17, 54, 1
	do_s	3,  6,  3, 11, 12, 27, 40, 1
	do_s	4, 39, 33, 34, 10, 18, 55, 1
	do_s	5, 16,  7,  1, 43, 31, 28, 1
	do_s	6, 49,  9,  0, 44, 15, 38, 1
	do_s	7, 37, 45,  2, 35, 22, 14, 1
	do_s	8, 51, 23, 52, 36, 42,  8, 1
	do_s	1, 39,  3, 18, 27,  5, 33, 0
	do_s	2, 19, 55, 46, 40,  6, 11, 0
	do_s	3, 20, 17, 25, 26, 41, 54, 0
	do_s	4, 53, 47, 48, 24, 32, 12, 0
	do_s	5, 30, 21, 15,  2, 45, 42, 0
	do_s	6,  8, 23, 14, 31, 29, 52, 0
	do_s	7, 51,  0, 16, 49, 36, 28, 0
	do_s	8, 38, 37,  7, 50,  1, 22, 0
	do_s	1, 53, 17, 32, 41, 19, 47, 1
	do_s	2, 33, 12,  3, 54, 20, 25, 1
	do_s	3, 34,  6, 39, 40, 55, 11, 1
	do_s	4, 10,  4,  5, 13, 46, 26, 1
	do_s	5, 44, 35, 29, 16,  0,  1, 1
	do_s	6, 22, 37, 28, 45, 43,  7, 1
	do_s	7, 38, 14, 30,  8, 50, 42, 1
	do_s	8, 52, 51, 21,  9, 15, 36, 1
	do_s	1, 10,  6, 46, 55, 33,  4, 0
	do_s	2, 47, 26, 17, 11, 34, 39, 0
	do_s	3, 48, 20, 53, 54, 12, 25, 0
	do_s	4, 24, 18, 19, 27,  3, 40, 0
	do_s	5, 31, 49, 43, 30, 14, 15, 0
	do_s	6, 36, 51, 42,  0,  2, 21, 0
	do_s	7, 52, 28, 44, 22,  9,  1, 0
	do_s	8,  7, 38, 35, 23, 29, 50, 0
	do_s	1, 24, 20,  3, 12, 47, 18, 1
	do_s	2,  4, 40,  6, 25, 48, 53, 1
	do_s	3,  5, 34, 10, 11, 26, 39, 1
	do_s	4, 13, 32, 33, 41, 17, 54, 1
	do_s	5, 45,  8,  2, 44, 28, 29, 1
	do_s	6, 50, 38,  1, 14, 16, 35, 1
	do_s	7,  7, 42, 31, 36, 23, 15, 1
	do_s	8, 21, 52, 49, 37, 43,  9, 1
	do_s	1,  6, 27, 10, 19, 54, 25, 0
	do_s	2, 11, 47, 13, 32, 55,  3, 0
	do_s	3, 12, 41, 17, 18, 33, 46, 0
	do_s	4, 20, 39, 40, 48, 24,  4, 0
	do_s	5, 52, 15,  9, 51, 35, 36, 0
	do_s	6,  2, 45,  8, 21, 23, 42, 0
	do_s	7, 14, 49, 38, 43, 30, 22, 0
	do_s	8, 28,  0,  1, 44, 50, 16, 0
	do_s	1, 20, 41, 24, 33, 11, 39, 1
	do_s	2, 25,  4, 27, 46, 12, 17, 1
	do_s	3, 26, 55,  6, 32, 47,  3, 1
	do_s	4, 34, 53, 54,  5, 13, 18, 1
	do_s	5,  7, 29, 23, 38, 49, 50, 1
	do_s	6, 16,  0, 22, 35, 37,  1, 1
	do_s	7, 28,  8, 52,  2, 44, 36, 1
	do_s	8, 42, 14, 15, 31,  9, 30, 1
	do_s	1, 34, 55, 13, 47, 25, 53, 0
	do_s	2, 39, 18, 41,  3, 26,  6, 0
	do_s	3, 40, 12, 20, 46,  4, 17, 0
	do_s	4, 48, 10, 11, 19, 27, 32, 0
	do_s	5, 21, 43, 37, 52,  8,  9, 0
	do_s	6, 30, 14, 36, 49, 51, 15, 0
	do_s	7, 42, 22,  7, 16, 31, 50, 0
	do_s	8,  1, 28, 29, 45, 23, 44, 0
	do_s	1, 48, 12, 27,  4, 39, 10, 1
	do_s	2, 53, 32, 55, 17, 40, 20, 1
	do_s	3, 54, 26, 34,  3, 18,  6, 1
	do_s	4,  5, 24, 25, 33, 41, 46, 1
	do_s	5, 35,  2, 51,  7, 22, 23, 1
	do_s	6, 44, 28, 50,  8, 38, 29, 1
	do_s	7,  1, 36, 21, 30, 45,  9, 1
	do_s	8, 15, 42, 43,  0, 37, 31, 1
	do_s	1,  5, 26, 41, 18, 53, 24, 0
	do_s	2, 10, 46, 12,  6, 54, 34, 0
	do_s	3, 11, 40, 48, 17, 32, 20, 0
	do_s	4, 19, 13, 39, 47, 55,  3, 0
	do_s	5, 49, 16, 38, 21, 36, 37, 0
	do_s	6, 31, 42,  9, 22, 52, 43, 0
	do_s	7, 15, 50, 35, 44,  0, 23, 0
	do_s	8, 29,  1,  2, 14, 51, 45, 0
	do_s	1, 19, 40, 55, 32, 10, 13, 1
	do_s	2, 24,  3, 26, 20, 11, 48, 1
	do_s	3, 25, 54,  5,  6, 46, 34, 1
	do_s	4, 33, 27, 53,  4, 12, 17, 1
	do_s	5,  8, 30, 52, 35, 50, 51, 1
	do_s	6, 45,  1, 23, 36,  7,  2, 1
	do_s	7, 29,  9, 49, 31, 14, 37, 1
	do_s	8, 43, 15, 16, 28, 38,  0, 1

; current order of s-boxes (smallest no. of cycles first): 47286351
	do_s	4, 47, 41, 10, 18, 26,  6, 0
	check	4, 48, 25, 19, 9, 0
	do_s	7, 43, 23,  8, 45, 28, 51, 0
	check	7, 36, 31, 11, 21, 6
	do_s	2, 13, 17, 40, 34, 25,  5, 0
	check	2, 56, 12, 27, 1, 17
	do_s	8,  2, 29, 30, 42, 52, 14, 0
	check	8, 32, 4, 26, 14, 20
	do_s	6,  0, 15, 37, 50, 21, 16, 0
	check	6, 40, 3, 28, 10, 18
	do_s	3, 39, 11, 19, 20,  3, 48, 0
	check	3, 52, 23, 15, 29, 5
	do_s	5, 22, 44,  7, 49,  9, 38, 0
	check	5, 44, 7, 13, 24, 2
	do_s	1, 33, 54, 12, 46, 24, 27, 0
	check	1, 60, 8, 16, 22, 30
	do_s	4, 54, 48, 17, 25, 33, 13, 1
	check	4, 16, 32+25, 32+19, 32+9, 32+0
	do_s	7, 50, 30, 15, 52, 35, 31, 1
	check	7,  4, 32+31, 32+11, 32+21, 32+6
	do_s	2, 20, 24, 47, 41, 32, 12, 1
	check	2, 24, 32+12, 32+27, 32+1, 32+17
	do_s	8,  9, 36, 37, 49,  0, 21, 1
	check	8,  0, 32+4, 32+26, 32+14, 32+20
	do_s	6,  7, 22, 44,  2, 28, 23, 1
	check	6,  8, 32+3, 32+28, 32+10, 32+18
	do_s	3, 46, 18, 26, 27, 10, 55, 1
	check	3, 20, 32+23, 32+15, 32+29, 32+5
	do_s	5, 29, 51, 14,  1, 16, 45, 1
	check	5, 12, 32+7, 32+13, 32+24, 32+2
	do_s	1, 40,  4, 19, 53,  6, 34, 1
	check	1, 28, 32+8, 32+16, 32+22, 32+30

	DCD	-1 ; mark end of table...

cachedcode_end


 [ patch
	DCD	cachedcode_end-cachedcode_start
	DCD	&DEAD
 ]
	END