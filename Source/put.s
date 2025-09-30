*	put.s
*		ÇPï∂éöï\é¶ÉãÅ[É`Éì
*
*				We'll Never Stop Programming This Way.

	.xdef	put_6x12_jt,put_12x12_jt
	.xdef	put_8x16_jt,put_16x16_jt

CRTC_R23	equ	$e8_002e
NEXT_LINE	equ	128	* ÇPÉâÉCÉìÇ†ÇΩÇËÇÃÉoÉCÉgêî

	.text
	.even

		* ÇPï∂éöï\é¶ÉãÅ[É`Éì
		* in  : a1.l = ï∂éöÉtÉHÉìÉgÇÃÉAÉhÉåÉX
		* 	a2.l = ï\é¶Ç∑ÇÈ TEXTVRAM ÇÃÉAÉhÉåÉX
		* out : Ç»Çµ
		* reg : d0-d3/a0-a1 îjâÛ
		*	ÇªÇÍà»äOÇÕï€ë∂

put_6x12_jt:
q	=	put_6x12_jt
	.dc.w	put_6x12_0-q
	.dc.w	put_6x12_1-q
	.dc.w	put_6x12_2-q
	.dc.w	put_6x12_3-q
	.dc.w	put_6x12_4-q
	.dc.w	put_6x12_5-q
	.dc.w	put_6x12_6-q
	.dc.w	put_6x12_7-q
	.dc.w	put_6x12_8-q
	.dc.w	put_6x12_9-q
	.dc.w	put_6x12_10-q
	.dc.w	put_6x12_11-q
	.dc.w	put_6x12_12-q
	.dc.w	put_6x12_13-q
	.dc.w	put_6x12_14-q
	.dc.w	put_6x12_15-q


*	Å°Å°Å°Å°Å°Å°Å†Å†_Å†Å†Å†Å†Å†Å†Å†Å†
put_6x12_0:
	move.w	#%00000011_11111111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11	* HAS.X ÇÃägí£ã^éóñΩóﬂ
	move.b	(a1)+,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å°Å°Å°Å°Å°Å°Å†_Å†Å†Å†Å†Å†Å†Å†Å†
put_6x12_1:
	move.w	#%10000001_11111111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.b	(a1)+,d0
	ror.b	d0
	move.b	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å°Å°Å°Å°Å°Å°_Å†Å†Å†Å†Å†Å†Å†Å†
put_6x12_2:
	move.w	#%11000000_11111111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.b	(a1)+,d0
	ror.b	#2,d0
	move.b	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_6x12_3:
*	Å†Å†Å†Å°Å°Å°Å°Å°_Å°Å†Å†Å†Å†Å†Å†Å†
	move.w	#%11100000_01111111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	rol.w	#8-3,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_6x12_4:
*	Å†Å†Å†Å†Å°Å°Å°Å°_Å°Å°Å†Å†Å†Å†Å†Å†
	move.w	#%11110000_00111111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	rol.w	#8-4,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_6x12_5:
*	Å†Å†Å†Å†Å†Å°Å°Å°_Å°Å°Å°Å†Å†Å†Å†Å†
	move.w	#%11111000_00011111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	rol.w	#8-5,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_6x12_6:
*	Å†Å†Å†Å†Å†Å†Å°Å°_Å°Å°Å°Å°Å†Å†Å†Å†
	move.w	#%11111100_00001111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	rol.w	#8-6,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_6x12_7:
*	Å†Å†Å†Å†Å†Å†Å†Å°_Å°Å°Å°Å°Å°Å†Å†Å†
	move.w	#%11111110_00000111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	rol.w	d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_6x12_8:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å°Å°Å°Å°Å°Å°Å†Å†
	move.w	#%11111111_00000011,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.b	(a1)+,NEXT_LINE*%A+1(a2)
	.endm
	rts

put_6x12_9:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å°Å°Å°Å°Å°Å°Å†
	move.w	#%11111111_10000001,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.b	(a1)+,d0
	ror.b	d0
	move.b	d0,NEXT_LINE*%A+1(a2)
	.endm
	rts

put_6x12_10:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å°Å°Å°Å°Å°Å°
	move.w	#%11111111_11000000,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.b	(a1)+,d0
	ror.b	#2,d0
	move.b	d0,NEXT_LINE*%A+1(a2)
	.endm
	rts

put_6x12_11:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å°Å°Å°Å°Å°_Å°Å†Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11100000,d2
	move.w	#%01111111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#11-8,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_6x12_12:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å°Å°Å°Å°_Å°Å°Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11110000,d2
	move.w	#%00111111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#12-8,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_6x12_13:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å°Å°Å°_Å°Å°Å°Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111000,d2
	move.w	#%00011111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#13-8,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_6x12_14:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å†Å°Å°_Å°Å°Å°Å°Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111100,d2
	move.w	#%00001111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#14-8,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_6x12_15:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å†Å†Å°_Å°Å°Å°Å°Å°Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111110,d2
	move.w	#%00000111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#15-8,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts


****************************************************************
put_12x12_jt:
q	=	put_12x12_jt
	.dc.w	put_12x12_0-q
	.dc.w	put_12x12_1-q
	.dc.w	put_12x12_2-q
	.dc.w	put_12x12_3-q
	.dc.w	put_12x12_4-q
	.dc.w	put_12x12_5-q
	.dc.w	put_12x12_6-q
	.dc.w	put_12x12_7-q
	.dc.w	put_12x12_8-q
	.dc.w	put_12x12_9-q
	.dc.w	put_12x12_10-q
	.dc.w	put_12x12_11-q
	.dc.w	put_12x12_12-q
	.dc.w	put_12x12_13-q
	.dc.w	put_12x12_14-q
	.dc.w	put_12x12_15-q


put_12x12_0:
*	Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å†Å†Å†Å†
	move.w	#%00000000_00001111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11	* HAS.X ÇÃägí£ã^éóñΩóﬂ
	move.w	(a1)+,NEXT_LINE*%A(a2)
	.endm
	rts

put_12x12_1:
*	Å†Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å†Å†Å†
	move.w	#%10000000_00000111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	ror.w	d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_12x12_2:
*	Å†Å†Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å†Å†
	move.w	#%11000000_00000011,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	ror.w	#2,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_12x12_3:
*	Å†Å†Å†Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å†
	move.w	#%11100000_00000001,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	ror.w	#3,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_12x12_4:
*	Å†Å†Å†Å†Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°
	move.w	#%11110000_00000000,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	ror.w	#4,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

put_12x12_5:
*	Å†Å†Å†Å†Å†Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å†Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111000_00000000,d2
	move.w	#%01111111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	ror.w	#5,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_6:
*	Å†Å†Å†Å†Å†Å†Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111100_00000000,d2
	move.w	#%00111111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	ror.w	#6,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_7:
*	Å†Å†Å†Å†Å†Å†Å†Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111110_00000000,d2
	move.w	#%00011111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	ror.w	#7,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_8:
*	Å†Å†Å†Å†Å†Å†Å†Å†Å†_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_00000000,d2
	move.w	#%00001111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	d2,(a0)
	move.b	(a1)+,NEXT_LINE*%A+1(a2)
	move.w	d3,(a0)
	move.b	(a1)+,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_9:
*	Å†Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_10000000,d2
	move.w	#%00000111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	rol.w	#16-9,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_10:
*	Å†Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11000000,d2
	move.w	#%00000011_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	rol.w	#16-10,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_11:
*	Å†Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11100000,d2
	move.w	#%00000001_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	rol.w	#16-11,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_12:
*	Å†Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11110000,d2
	move.w	#%00000000_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	rol.w	#16-12,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_13:
*	Å†Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å†Å†Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111000,d2
	move.w	#%00000000_01111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	rol.w	#16-13,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_14:
*	Å†Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å†Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å†Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111100,d2
	move.w	#%00000000_00111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	rol.w	#16-14,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_12x12_15:
*	Å†Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å†Å†Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111110,d2
	move.w	#%00000000_00011111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11
	move.w	(a1)+,d0
	rol.w	#16-15,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts


****************************************************************
put_8x16_jt:
q	=	put_8x16_jt
	.dc.w	put_8x16_0-q
	.dc.w	put_8x16_1-q
	.dc.w	put_8x16_2-q
	.dc.w	put_8x16_3-q
	.dc.w	put_8x16_4-q
	.dc.w	put_8x16_5-q
	.dc.w	put_8x16_6-q
	.dc.w	put_8x16_7-q
	.dc.w	put_8x16_8-q
	.dc.w	put_8x16_9-q
	.dc.w	put_8x16_10-q
	.dc.w	put_8x16_11-q
	.dc.w	put_8x16_12-q
	.dc.w	put_8x16_13-q
	.dc.w	put_8x16_14-q
	.dc.w	put_8x16_15-q


*	Å°Å°Å°Å°Å°Å°Å°Å°_Å†Å†Å†Å†Å†Å†Å†Å†
put_8x16_0:
	move.w	#%00000000_11111111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15	* HAS.X ÇÃägí£ã^éóñΩóﬂ
	move.b	(a1)+,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å°Å°Å°Å°Å°Å°Å°_Å°Å†Å†Å†Å†Å†Å†Å†
put_8x16_1:
	move.w	#%10000000_01111111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	lsl.w	#7,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å°Å°Å°Å°Å°Å°_Å°Å°Å†Å†Å†Å†Å†Å†
put_8x16_2:
	move.w	#%11000000_00111111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	lsl.w	#6,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å†Å°Å°Å°Å°Å°_Å°Å°Å°Å†Å†Å†Å†Å†
put_8x16_3:
	move.w	#%11100000_00011111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	lsl.w	#5,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å°Å°Å°Å°_Å°Å°Å°Å°Å†Å†Å†Å†
put_8x16_4:
	move.w	#%11110000_00001111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	lsl.w	#4,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å°Å°Å°_Å°Å°Å°Å°Å°Å†Å†Å†
put_8x16_5:
	move.w	#%11111000_00000111,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	lsl.w	#3,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å°Å°_Å°Å°Å°Å°Å°Å°Å†Å†
put_8x16_6:
	move.w	#%11111100_00000011,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	lsl.w	#2,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å°_Å°Å°Å°Å°Å°Å°Å°Å†
put_8x16_7:
	move.w	#%11111110_00000001,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	add.w	d0,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å†_Å°Å°Å°Å°Å°Å°Å°Å°
put_8x16_8:
	move.w	#%11111111_00000000,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	move.w	d0,NEXT_LINE*%A(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å°Å°Å°Å°Å°Å°Å°_Å°Å†Å†Å†Å†Å†Å†Å†
put_8x16_9:
	lea.l	CRTC_R23,a0
	move.w	#%11111111_10000000,d2
	move.w	#%01111111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å°Å°Å°Å°Å°Å°_Å°Å°Å†Å†Å†Å†Å†Å†
put_8x16_10:
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11000000,d2
	move.w	#%00111111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#2,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å°Å°Å°Å°Å°_Å°Å°Å°Å†Å†Å†Å†Å†
put_8x16_11:
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11100000,d2
	move.w	#%00011111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#3,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å°Å°Å°Å°_Å°Å°Å°Å°Å†Å†Å†Å†
put_8x16_12:
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11110000,d2
	move.w	#%00001111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#4,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å°Å°Å°_Å°Å°Å°Å°Å°Å†Å†Å†
put_8x16_13:
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111000,d2
	move.w	#%00000111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#5,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å†Å°Å°_Å°Å°Å°Å°Å°Å°Å†Å†
put_8x16_14:
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111100,d2
	move.w	#%00000011_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#6,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å†Å†Å°_Å°Å°Å°Å°Å°Å°Å°Å†
put_8x16_15:
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111110,d2
	move.w	#%00000001_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	moveq.l	#0,d0
	move.b	(a1)+,d0
	ror.w	#7,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts


****************************************************************
put_16x16_jt:
q	=	put_16x16_jt
	.dc.w	put_16x16_0-q
	.dc.w	put_16x16_1-q
	.dc.w	put_16x16_2-q
	.dc.w	put_16x16_3-q
	.dc.w	put_16x16_4-q
	.dc.w	put_16x16_5-q
	.dc.w	put_16x16_6-q
	.dc.w	put_16x16_7-q
	.dc.w	put_16x16_8-q
	.dc.w	put_16x16_9-q
	.dc.w	put_16x16_10-q
	.dc.w	put_16x16_11-q
	.dc.w	put_16x16_12-q
	.dc.w	put_16x16_13-q
	.dc.w	put_16x16_14-q
	.dc.w	put_16x16_15-q


put_16x16_0:
*	Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°
	move.w	#%00000000_00000000,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15	* HAS.X ÇÃägí£ã^éóñΩóﬂ
	move.w	(a1)+,NEXT_LINE*%A(a2)
	.endm
	rts

put_16x16_1:
*	Å†Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å†Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%10000000_00000000,d2
	move.w	#%01111111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	ror.w	d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_2:
*	Å†Å†Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11000000_00000000,d2
	move.w	#%00111111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	ror.w	#2,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_3: *	Å†Å†Å†Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11100000_00000000,d2
	move.w	#%00011111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	ror.w	#3,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_4:
*	Å†Å†Å†Å†Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11110000_00000000,d2
	move.w	#%00001111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	ror.w	#4,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_5:
*	Å†Å†Å†Å†Å†Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111000_00000000,d2
	move.w	#%00000111_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	ror.w	#5,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_6:
*	Å†Å†Å†Å†Å†Å†Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111100_00000000,d2
	move.w	#%00000011_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	ror.w	#6,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_7:
*	Å†Å†Å†Å†Å†Å†Å†Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111110_00000000,d2
	move.w	#%00000001_11111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	ror.w	#7,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_8:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°
	move.w	#0,CRTC_R23
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.b	(a1)+,NEXT_LINE*%A+1(a2)
	move.b	(a1)+,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_9:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å†Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_10000000,d2
	move.w	#%00000000_01111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	rol.w	#16-9,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_10:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å†Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11000000,d2
	move.w	#%00000000_00111111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	rol.w	#16-10,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_11:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å†Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11100000,d2
	move.w	#%00000000_00011111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	rol.w	#16-11,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_12:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å†Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11110000,d2
	move.w	#%00000000_00001111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	rol.w	#16-12,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_13:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å†Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111000,d2
	move.w	#%00000000_00000111,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	rol.w	#16-13,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_14:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å†Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å†Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111100,d2
	move.w	#%00000000_00000011,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	rol.w	#16-14,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

put_16x16_15:
*	Å†Å†Å†Å†Å†Å†Å†Å†_Å†Å†Å†Å†Å†Å†Å†Å°_Å°Å°Å°Å°Å°Å°Å°Å°_Å°Å°Å°Å°Å°Å°Å°Å†
	lea.l	CRTC_R23,a0
	move.w	#%11111111_11111110,d2
	move.w	#%00000000_00000001,d3
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a1)+,d0
	rol.w	#16-15,d0
	move.w	d2,(a0)
	move.w	d0,NEXT_LINE*%A(a2)
	move.w	d3,(a0)
	move.w	d0,NEXT_LINE*%A+2(a2)
	.endm
	rts

