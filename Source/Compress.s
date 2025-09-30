*	イメージ1/2圧縮

	.xdef	_CompressImageHS
	.xdef	_CompressImageHQ
	.xdef	_CompressImage256HS
	.xdef	_CompressImage256HQ
	.xdef	_NonCompressImage256

	.text
	.even

_CompressImageHS:	* 65536 色べたイメージ 1/2 圧縮ルーチン（高速・低画質版）
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	movea.l	8(a6),a1	* a1.l = 転送先
	movea.l	12(a6),a0	* a0.l = 転送元


	move.l	20(a6),d7	* d7.w = y
	addq.w	#1,d7
	lsr.w	d7
	subq.w	#1,d7		* d7.w = y ループ回数

	move.l	16(a6),d0	* d0.w = x
	move.w	d0,d1
	add.w	d1,d1
	movea.w	d1,a5
	btst.l	#0,d0
	beq	@f
	subq.w	#2,a5
@@:			* a5.w = 次の y に行く時 a0.l に足す値

	move.w	d0,d6
	addq.w	#1,d6
	lsr.w	d6
	subq.w	#1,d6
	move.w	d6,a4		* a4.w = x ループ回数

1:
	move.w	a4,d6
2:
	move.w	(a0)+,(a1)+
	addq.w	#2,a0
	dbra	d6,2b

	adda.w	a5,a0
	dbra	d7,1b

CompressImageHS_rts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
_CompressImageHQ:	* 65536 色べたイメージ 1/2 圧縮ルーチン（低速・高画質版）
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	movea.l	8(a6),a1	* a1.l = 転送先
	movea.l	12(a6),a0	* a0.l = 転送元

	move.l	20(a6),d7	* d7.w = y
	move.w	d7,d0
	addq.w	#1,d7
	lsr.w	d7
	subq.w	#1,d7		* d7.w = y ループ回数
	btst.l	#0,d0		* y サイズが奇数の時は
	beq	@f		*
	subq.w	#1,d7		* ループ回数を１回少なく
@@:
	move.l	16(a6),d0	* d0.w = x
	move.w	d0,d1
	add.w	d1,d1
	movea.w	d1,a5
	btst.l	#0,d0
	beq	@f
	subq.w	#2,a5
@@:			* a5.w = 次の y に行く時 a0.l に足す値

	move.w	d0,d6
	addq.w	#1,d6
	lsr.w	d6
	subq.w	#1,d6
	move.w	d6,a4		* a4.w = x ループ回数


	move.w	d0,d4
	add.w	d4,d4
	move.w	d4,a3		* a3.w = x*2

	move.w	#%1111100000111110,d4	* d4.w = 定数
	move.w	#%0000011111000000,d5

1:
	move.w	a4,d6
2:
	move.w	(a0),d0
	move.w	d0,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	lsr.w	d0
	lsr.w	d1
	move.w	2(a0),d2
	move.w	d2,d3
*	andi.w	#%1111100000111110,d2
	and.w	d4,d2
*	andi.w	#%0000011111000000,d3
	and.w	d5,d3
	lsr.w	d2
	lsr.w	d3
	add.w	d2,d0
	add.w	d3,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	swap.w	d0	* 上２ドットの平均結果を
	swap.w	d1	* 一時的に退避

	move.w	(a3.w,a0),d0
	move.w	d0,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	lsr.w	d0
	lsr.w	d1
	move.w	2(a3.w,a0),d2
	move.w	d2,d3
*	andi.w	#%1111100000111110,d2
	and.w	d4,d2
*	andi.w	#%0000011111000000,d3
	and.w	d5,d3
	lsr.w	d2
	lsr.w	d3
	add.w	d0,d2
	add.w	d1,d3
*	andi.w	#%1111100000111110,d2
	and.w	d4,d2
*	andi.w	#%0000011111000000,d3
	and.w	d5,d3
	swap.w	d0
	swap.w	d1

	lsr.w	d0
	lsr.w	d1
	lsr.w	d2
	lsr.w	d3
	add.w	d2,d0
	add.w	d3,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	or.w	d1,d0
	move.w	d0,(a1)+
	addq.w	#4,a0


.if	0
	* x 方向のみ平均を取る
	move.w	(a0),d0
	move.w	d0,d1
	andi.w	#%1111100000111110,d0
	andi.w	#%0000011111000000,d1
	lsr.w	d0
	lsr.w	d1
	move.w	2(a0),d2
	move.w	d2,d3
	andi.w	#%1111100000111110,d2
	andi.w	#%0000011111000000,d3
	lsr.w	d2
	lsr.w	d3
	add.w	d2,d0
	add.w	d3,d1
	andi.w	#%1111100000111110,d0
	andi.w	#%0000011111000000,d1
	or.w	d1,d0
	move.w	d0,(a1)+
	addq.w	#4,a0
.endif
	dbra	d6,2b

	adda.w	a5,a0
	dbra	d7,1b


	move.l	20(a6),d0	* d0.w = y
	btst.l	#0,d0
	beq	CompressImageHQ_rts
			* y サイズが奇数の時、最下段は
			* x 方向のみ平均を取る
	move.w	a4,d6
2:
	move.w	(a0),d0
	move.w	d0,d1
	andi.w	#%1111100000111110,d0
	andi.w	#%0000011111000000,d1
	lsr.w	d0
	lsr.w	d1
	move.w	2(a0),d2
	move.w	d2,d3
*	andi.w	#%1111100000111110,d2
	and.w	d4,d2
*	andi.w	#%0000011111000000,d3
	and.w	d5,d3
	lsr.w	d2
	lsr.w	d3
	add.w	d2,d0
	add.w	d3,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	or.w	d1,d0
	move.w	d0,(a1)+
	addq.w	#4,a0

	dbra	d6,2b

CompressImageHQ_rts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts



****************************************************************
_CompressImage256HS:	* 256 色->65536 色変換付き
			* べたイメージ 1/2 圧縮ルーチン（高速・低画質版）
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	movea.l	8(a6),a1	* a1.l = 転送先
	movea.l	12(a6),a0	* a0.l = 転送元
	movea.l	16(a6),a2	* a2.l = パレットバッファ

	move.l	24(a6),d7	* d7.w = y
	addq.w	#1,d7
	lsr.w	d7
	subq.w	#1,d7		* d7.w = y ループ回数

	move.l	20(a6),d0	* d0.w = x
	move.w	d0,a5
	btst.l	#0,d0
	beq	@f
	subq.w	#1,a5
@@:			* a5.w = 次の y に行く時 a0.l に足す値

	move.w	d0,d6
	addq.w	#1,d6
	lsr.w	d6
	subq.w	#1,d6
	move.w	d6,a4		* a4.w = x ループ回数

	move.w	d0,a3		* a3.w = x

1:
	move.w	a4,d6
2:
	moveq.l	#0,d0
	move.b	(a0),d0
	add.w	d0,d0
	move.w	(d0.w,a2),(a1)+
	addq.w	#2,a0

	dbra	d6,2b

	adda.w	a5,a0
	dbra	d7,1b

CompressImage256HS_rts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
_CompressImage256HQ:	* 256 色->65536 色変換付き
			* べたイメージ 1/2 圧縮ルーチン（低速・高画質版）
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	movea.l	8(a6),a1	* a1.l = 転送先
	movea.l	12(a6),a0	* a0.l = 転送元
	movea.l	16(a6),a2	* a2.l = パレットバッファ

	move.l	24(a6),d7	* d7.w = y
	move.l	d7,d0
	addq.w	#1,d7
	lsr.w	d7
	subq.w	#1,d7		* d7.w = y ループ回数
	ror.l	d0		* y サイズが奇数なら
	bcc	@f		* ループ回数を１回少なく
	subq.w	#1,d7		* （最後の１回は横方向のみの平均）
@@:

	move.l	20(a6),d0	* d0.w = x
	move.w	d0,a5
	btst.l	#0,d0
	beq	@f
	subq.w	#1,a5
@@:			* a5.w = 次の y に行く時 a0.l に足す値

	move.w	d0,d6
	addq.w	#1,d6
	lsr.w	d6
	subq.w	#1,d6
	move.w	d6,a4		* a4.w = x ループ回数

	move.w	d0,a3		* a3.w = x


	move.w	#%1111100000111110,d4	* d4.w = 定数
	move.w	#%0000011111000000,d5	* d4.w = 定数

	tst.w	d7
	bmi	3f
1:
	move.w	a4,d6
2:
	clr.w	d0
	move.b	(a0),d0
	add.w	d0,d0
	move.w	(d0.w,a2),d0
	move.w	d0,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	lsr.w	d0
	lsr.w	d1
	clr.w	d2
	move.b	1(a0),d2
	add.w	d2,d2
	move.w	(d2.w,a2),d2
	move.w	d2,d3
*	andi.w	#%1111100000111110,d2
	and.w	d4,d2
*	andi.w	#%0000011111000000,d3
	and.w	d5,d3
	lsr.w	d2
	lsr.w	d3
	add.w	d2,d0
	add.w	d3,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	swap.w	d0	* 上２ドットの平均結果を
	swap.w	d1	* 一時的に退避

	clr.w	d0
	move.b	(a3.w,a0),d0
	add.w	d0,d0
	move.w	(d0.w,a2),d0
	move.w	d0,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	lsr.w	d0
	lsr.w	d1
	clr.w	d2
	move.b	1(a3.w,a0),d2
	add.w	d2,d2
	move.w	(d2.w,a2),d2
	move.w	d2,d3
*	andi.w	#%1111100000111110,d2
	and.w	d4,d2
*	andi.w	#%0000011111000000,d3
	and.w	d5,d3
	lsr.w	d2
	lsr.w	d3
	add.w	d0,d2
	add.w	d1,d3
*	andi.w	#%1111100000111110,d2
	and.w	d4,d2
*	andi.w	#%0000011111000000,d3
	and.w	d5,d3
	swap.w	d0
	swap.w	d1

	lsr.w	d0
	lsr.w	d1
	lsr.w	d2
	lsr.w	d3
	add.w	d2,d0
	add.w	d3,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	or.w	d1,d0
	move.w	d0,(a1)+
	addq.w	#2,a0

	dbra	d6,2b

	adda.w	a5,a0
	dbra	d7,1b

3:
	move.l	24(a6),d0	* d0.w = y
*	btst.l	#0,d0
*	beq	CompressImage256HQ_rts
	ror.l	d0
	bcc	CompressImage256HQ_rts
			* y サイズが奇数の時、最下段は
			* x 方向のみ平均を取る
	move.w	a4,d6
2:
	clr.w	d0
	move.b	(a0),d0
	add.w	d0,d0
	move.w	(d0.w,a2),d0
	move.w	d0,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	lsr.w	d0
	lsr.w	d1
	clr.w	d2
	move.b	1(a0),d2
	add.w	d2,d2
	move.w	(d2.w,a2),d2
	move.w	d2,d3
*	andi.w	#%1111100000111110,d2
	and.w	d4,d2
*	andi.w	#%0000011111000000,d3
	and.w	d5,d3
	lsr.w	d2
	lsr.w	d3
	add.w	d2,d0
	add.w	d3,d1
*	andi.w	#%1111100000111110,d0
	and.w	d4,d0
*	andi.w	#%0000011111000000,d1
	and.w	d5,d1
	or.w	d1,d0
	move.w	d0,(a1)+
	addq.w	#2,a0
	dbra	d6,2b


CompressImage256HQ_rts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
_NonCompressImage256:	* 256 色->65536 色変換・圧縮なし
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	movea.l	8(a6),a1	* a1.l = 転送先
	movea.l	12(a6),a0	* a0.l = 転送元
	movea.l	16(a6),a2	* a2.l = パレットバッファ

	move.l	24(a6),d7	* d7.w = y
	subq.w	#1,d7		* d7.w = y ループ回数

	move.l	20(a6),a4	* a4.w = x
	subq.w	#1,a4
1:
	move.w	a4,d6
2:
	moveq.l	#0,d0
	move.b	(a0)+,d0
	add.w	d0,d0
	move.w	(d0.w,a2),(a1)+

	dbra	d6,2b
	dbra	d7,1b

	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


