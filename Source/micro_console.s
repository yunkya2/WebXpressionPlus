*	micro_console.s

	.include	DOSCALL.MAC
	.include	IOCSCALL.MAC
	.include	WebXpression.inc

	.xdef	_McInit,_McPuts,_McDbPuts,_McCursorTop
	.xref	put_6x12_jt,put_12x12_jt
	.xref	_d_option,_eventrec

TEXTVRAM	equ	$e0_0000
CRTC_R21	equ	$e8_002a

NEXT_LINE	equ	128	* １ラインあたりのバイト数
MC_ADR		equ	TEXTVRAM+$60000+NEXT_LINE*358+34*2
MC_BIT		equ	2
MC_LINE		equ	10
MC_XSIZE	equ	18*12

	.text
	.even

_McInit:
	move.l	#MC_ADR+(MC_LINE-1)*12*NEXT_LINE,mc_a2
	move.w	#MC_BIT,mc_d5
	clr.w	mc_d7
	rts


****************************************************************
_McCursorTop:		* カーソルを行の先頭に（かつ、その行をクリア）
	movem.l	d1/a0-a1,-(sp)

	movea.l	#MC_ADR+(MC_LINE-1)*12*NEXT_LINE,a0
	move.l	a0,mc_a2
	move.w	#MC_BIT,mc_d5
	clr.w	mc_d7

	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	moveq.l	#12-1,d0
	moveq.l	#0,d1
1:	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
*	move.b	#$02,-1(a0)	* 外枠を描画（うわー）
	lea.l	NEXT_LINE-4*7(a0),a0
	dbra	d0,1b

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	movem.l	(sp)+,d1/a0-a1
	rts

****************************************************************
_McDbPuts:		* デバッグ用 McPuts
			* -d オプションが指定されていたら Mc と標準出力に出力
			* そうでなければ何もせずに帰る
			* in  : 8(a6).l = 表示する文字列のアドレス
	tst.b	_d_option
	beq	McDbPutsRts

	link	a6,#0
	move.l	8(a6),-(sp)
	bsr	_McPuts
	addq.w	#4,sp
	unlk	a6

McDbPutsRts:
	rts


****************************************************************
_McPuts:
			* in  : 8(a6).l = 表示する文字列のアドレス
	link	a6,#-2
	movem.l	d1-d7/a0-a5,-(sp)

	tst.b	_d_option
	beq	@f
	move.l	8(a6),-(sp)
	DOS	_PRINT
	addq.w	#4,sp
@@:

	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	movea.l	_eventrec,a4	* a4.l =
	clr.w	-2(a6)		* マウスカーソル消したフラグ
*	move.w	_ms_pos_x,d0
	move.w	mouse_x(a4),d0
	cmpi.w	#288+256,d0
	blt	@f
	move.w	mouse_y(a4),d0
	cmpi.w	#356,d0
	blt	@f
	cmpi.w	#480+16,d0
	bgt	@f
	st.b	-2(a6)
	IOCS	_MS_CUROF	* microconsole 内にマウスカーソルがある時のみ
				* マウスを消す
@@:

	movea.l	8(a6),a4	* a4.l = 表示する文字列
McPrint_loop:
	movea.l	mc_a2,a2
	move.w	mc_d5,d5
	move.w	#MC_XSIZE,d6
	move.w	mc_d7,d7
	bsr	draw1
	tst.b	-1(a4)
	beq	McPrint_rts

			* 行の右端に達した or $0a を見つけた
*	cmpi.b	#$0a,-1(a4)	* cr?
*	bne	@f

	move.l	#MC_ADR+(MC_LINE-1)*12*NEXT_LINE,mc_a2
	move.w	#MC_BIT,mc_d5
	clr.w	mc_d7

			* １行スクロールアップ
	clr.w	CRTC_R21
	lea.l	MC_ADR,a1
	lea.l	12*NEXT_LINE(a1),a0

	moveq.l	#MC_LINE-1-1,d1
1:
	moveq.l	#12-1,d0
2:	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	lea.l	NEXT_LINE-4*7(a0),a0
	lea.l	NEXT_LINE-4*7(a1),a1
	dbra	d0,2b
	dbra	d1,1b

	movea.l	a1,a0
	moveq.l	#12-1,d0
	moveq.l	#0,d1
1:	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
	move.l	d1,(a0)+
*	move.b	#$02,-1(a0)	* 外枠を描画（うわー）
	lea.l	NEXT_LINE-4*7(a0),a0
	dbra	d0,1b

	bra	McPrint_loop

McPrint_rts:
	move.l	a2,mc_a2
	move.w	d5,mc_d5
	move.w	d7,mc_d7

	tst.w	-2(a6)
	beq	@f
	IOCS	_MS_CURON
@@:
	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts

****************************************************************
*	１行表示ルーチン
		* 速度最優先のためレジスタ保存なし！
		* スーパーで呼ぶこと
		* a4.l が \0 を検出するかドット数が d6.w になったら帰る
draw1:		* in  : a4.l = 表示する文字列のアドレス
		*	a2.l = 表示する TEXTVRAM 左端のアドレス
		*	d5.w = 表示する TEXTVRAM 上のドット数 0~15
		*	d6.w = 表示するドット数
		*	d7.w = ドット数
		* out : a4.l = 次のアドレス
	move.w	CRTC_R21,-(sp)
	move.w	#%11_10000000,CRTC_R21	* テキスト画面同時アクセス（黒で描画）

draw1_loop:
	moveq.l	#0,d1
	move.b	(a4)+,d1	* d1.w = 文字コード
	beq	draw1_rts
	bpl	draw1_半角前半	* $00~$7f は半角前半

	cmpi.b	#$a0,d1
	bcs	@f
	cmpi.b	#$df,d1		* $a0-$df は半角後半
*	bls	draw1_半角後半
@@:

	move.b	d1,-(sp)	* lsl.w #8,d1 より速いって例のヤツ
	move.w	(sp)+,d1	*
	move.b	(a4)+,d1	* d1.w = 文字コード

*	cmpi.w	#$80ff,d1
*	bls	draw1_２バイト半角	* $80xx は２バイト半角
	cmpi.w	#'亜',d1
	bcs	draw1_全角非漢字
	cmpi.w	#'弌',d1
	bcs	draw1_全角第１水準
	cmpi.w	#'瑤',d1
	bls	draw1_全角第２水準
*	cmpi.w	#$f3ff,d1
*	bls	draw1_２バイト半角	* $f0xx-$f3xx は２バイト半角

	bra	draw1_next	* ここには来ないハズ

****	****
draw1_next:
	add.w	d0,d5
1:	cmpi.w	#16,d5
	bcs	2f
	addq.w	#2,a2
	subi.w	#16,d5
	bra	1b
2:
	bra	draw1_loop
draw1_rts:
	move.w	(sp)+,CRTC_R21	* テキスト画面同時アクセス

	rts



****************************************************************


draw1_半角前半:
*	cmpi.b	#$09,d1
*	beq	draw1_tab
*	cmpi.b	#$0d,d1
*	beq	draw1_cr
	cmpi.b	#$0a,d1
	beq	draw1_rts
*	cmpi.b	#$1a,d1
*	beq	draw1_rts
	cmpi.b	#$20,d1		* $20 以下は表示しない
	bcs	draw1_半角前半_nodisp

	addq.w	#6,d7		* 文字のドット数
	cmp.w	d6,d7
	ble	@f
	subq.w	#1,a4
	bra	draw1_rts
@@:
	moveq.l	#6,d2		* 12x12 dot
	IOCS	_FNTADR
	movea.l	d0,a1		* a1.l = フォントアドレス

	move.w	d5,d0
	lea.l	put_6x12_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)

	moveq.l	#6,d0		* 文字のドット数
	bra	draw1_next

draw1_半角前半_nodisp:
	moveq.l	#0,d0		* 文字のドット数
	bra	draw1_next


****************************************************************
draw1_全角非漢字:
draw1_全角第１水準:
draw1_全角第２水準:
	addi.w	#12,d7		* 文字のドット数
	cmp.w	d6,d7
	ble	@f
	subq.w	#2,a4
	bra	draw1_rts
@@:

	moveq.l	#6,d2		* 12x12 dot
	IOCS	_FNTADR
	movea.l	d0,a1		* a1.l = フォントアドレス

	move.w	d5,d0
	lea.l	put_12x12_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)

	moveq.l	#12,d0		* 文字のドット数
	bra	draw1_next


****************************************************************
	.bss
	.even
mc_a2:	.ds.l	1
mc_d5:	.ds.w	1
mc_d7:	.ds.w	1

