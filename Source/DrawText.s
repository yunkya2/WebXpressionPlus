*	DrawText.s

	.include	IOCSCALL.MAC
	.include	WebXpression.inc

	.xdef	_DrawTextAll,_DrawBack,_ClearText,_ClearText01
	.xdef	_CheckLink,_WaitVdisp
	.xdef	_ScrollForward,_ScrollBackward
	.xdef	_ScrollFastForward,_ScrollFastBackward
	.xdef	_PageForward,_PageBackward
	.xdef	_PageTop,_PageEnd,_PageJump
	.xdef	_SetConfigColor,_SetHtmlColor
	.xdef	_CalcScbarY,_CalcScbarLine
	.xdef	DrawLine,DrawLineLoop	*! debug

	.xref	put_6x12_jt,put_12x12_jt
	.xref	put_8x16_jt,put_16x16_jt
	.xref	_color_mode,_config_color,_html_color


GVRAM		equ	$c0_0000
TEXTVRAM	equ	$e0_0000
TEXTPALET	equ	$e8_2200
CRTC_R12	equ	$e8_0018
CRTC_R21	equ	$e8_002a
CRTC_R22	equ	$e8_002c
CRTC_R23	equ	$e8_002e
CRTC_PORT	equ	$e8_0480	* CRTC 動作設定ポート
GPIP_DATA	equ	$e8_8001

NEXT_LINE	equ	128	* １ラインあたりのバイト数
DISP_Y		equ	32	* 表示行数
DISP_X_OFFSET	equ	8	* スクロール部をずらして表示するオフセット


	.text
	.even

****************************************************************
*	１行表示ルーチン
		* 速度最優先のためレジスタ保存なし！
		* スーパーで呼ぶこと
DrawLine:	* in  : a5.l = 行管理テーブル
		*	a2.l = 表示する TEXTVRAM 左端のアドレス
		*	a3.l = 表示する GVRAM 左端のアドレス
		*	d5.l =  bit31 : = 1 実際に表示しない（ドット数を数えるだけ）
		*			bit30~16 : マウスカーソル X 座標
		*	image_table_ptr : xptext->image_table を設定しておくこと
		*	link_table_ptr : xptext->link_table を設定しておくこと
		*	check_mouse_x : eventrec->mouse_x を設定しておくこと
		* out : 通常描画時
		*		: なし
		*	ドット数数えモード時（d5.l<0 時）
		*		d0.w >= 0 : リンク中（`Lxxxx`のリンク番号）
		*		     <  0 : リンク外
		* reg : a5-a6 は保存、それ以外は破壊
	movem.l	a6,-(sp)

	move.w	#-1,in_link

	tst.l	d5
	bmi	1f
				* 描画モード時
	move.w	CRTC_R21,-(sp)
	move.w	#%11_00010000,CRTC_R21	* テキスト画面同時アクセス（黒で描画）
	bra	2f
1:				* ドット数数えモード時
	move.w	check_mouse_x,d0	* d0.w = マウスカーソルのＸ座標
	cmp.w	start_dot(a5),d0	* マウスカーソル X < start_dot だったら
	bcs	DrawLineRts
2:

	movem.l	a2-a3,tvram_left	* `Dxxx` `Gxxxx` の計算に必要

	move.w	start_dot(a5),d7	* d7.w = 画面左端からのドット数
					* （論理０ : DISP_X_OFFSET を含まない）
	moveq.l	#6,d6		* d6.w = フォントサイズ（全角はこの２倍）
				*	b31 = 下線フラグ

	movea.l	ptr(a5),a4	* a4.l = テキストへのポインタ
	move.l	a4,d0		* 念のためエラーチェック
	beq	DrawLineRts	*

	move.w	#DISP_X_OFFSET,d5
	add.w	start_dot(a5),d5
@@:	subi.w	#16,d5
	bmi	@f
	addq.w	#2,a2
	bra	@b
@@:	addi.w	#16,d5



DrawLineLoop:		* 桁ループ
	moveq.l	#0,d1
	move.b	(a4)+,d1	* d1.w = 文字コード
	beq	DrawLineRts2
	bpl	DrawLine半角前半	* $00~$7f は半角前半

	cmpi.b	#$a0,d1
	bcs	@f
	cmpi.b	#$df,d1		* $a0-$df は半角後半
	bls	DrawLine半角後半
@@:
	move.b	d1,-(sp)	* lsl.w #8,d1 より速いって例のヤツ
	move.w	(sp)+,d1	*
	move.b	(a4)+,d1	* d1.w = 文字コード

	cmpi.w	#$80ff,d1
	bls	DrawLine２バイト半角	* $80xx は２バイト半角
	cmpi.w	#'亜',d1
	bcs	DrawLine全角非漢字
	cmpi.w	#'弌',d1
	bcs	DrawLine全角第１水準
	cmpi.w	#'瑤',d1
	bls	DrawLine全角第２水準
	cmpi.w	#$f3ff,d1
	bls	DrawLine２バイト半角	* $f0xx-$f3xx は２バイト半角

	moveq.l	#0,d0
	bra	DrawLineNext	* ここには来ないハズ

****	****
DrawLineJmp:			* ジャンプテーブルに従って分岐
				* in  : d0.w = フォント種類
				*	a0.l = ジャンプテーブル
	moveq.l	#0,d0		*! debug
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jmp	(a0,d0.w)	* 分岐（分岐した後は DrawLineNext に帰ってくる）
				* out : d0.w = 文字のドット数
****	****
DrawLineNext:			* ここに来る時 d0.w に描画したドット数が入っている
				* もしくは独自に d5.w/d7.w/a2.l を操作して d0.w = 0
	add.w	d0,d5
	add.w	d0,d7
@@:	subi.w	#16,d5
	bmi	@f
	addq.w	#2,a2
	bra	@b
@@:	addi.w	#16,d5

	tst.l	d5
	bpl	DrawLineLoop
			* d5.l < 0 （ドット数数えモード）時の処理
	cmp.w	check_mouse_x,d7
	bcs	DrawLineLoop
	bra	DrawLineRts

DrawLineRts2:
	move.w	#-1,in_link	* 最後まで表示しきった時はマウス外れ
DrawLineRts:
	tst.l	d5
	bmi	@f
	move.w	(sp)+,CRTC_R21	* テキスト画面同時アクセス
@@:
	move.w	in_link,d0
	movem.l	(sp)+,a6
	rts


	.bss
	.even
tvram_left:	.ds.l	1
gvram_left:	.ds.l	1
in_link:	.ds.w	1
check_mouse_x:	.ds.w	1	* ドット数数えモード時用マウスカーソルのＸ座標

link_table_ptr:		.ds.l	1
image_table_ptr:	.ds.l	1
gr_y:			.ds.w	1	* GVRAM の y 座標

font_work:	.ds.b	3*24	* 下線を引いたりする処理用ワーク
				* 24x24 ドットまで大丈夫なハズ
	.text
	.even


****************************************************************


DrawLine半角前半:
	cmpi.b	#$09,d1
	beq	DrawLineTab
*	cmpi.b	#$0d,d1
*	beq	DrawLineCr
*	cmpi.b	#$0a,d1
*	beq	DrawLineLf
*	cmpi.b	#$1a,d1
*	beq	DrawLineRts

	cmpi.b	#'`',d1
	bne	@f
	cmpi.b	#'`',(a4)	* `` だった場合はそのまま素通り
	bne	DrawLineCmd	* そうでなければ `Lnn` のようなコマンド
	addq.w	#1,a4		*
@@:
*	moveq.l	#0,d0
*	move.b	dw_han0_font(a6),d0
	moveq.l	#0,d0			* フォント種類
	lea.l	DrawLine半角前半_6x12_jt(pc),a0
	cmpi.b	#6,d6
	beq	DrawLineJmp
	lea.l	DrawLine半角前半_8x16_jt(pc),a0
	cmpi.b	#8,d6
	beq	DrawLineJmp
*	lea.l	DrawLine半角前半_mp_jt(pc),a0
	bra	DrawLineJmp


DrawLineTab:
	sub.w	start_dot(a5),d7
	addi.w	#6*8,d7		* tab のドット数
	swap.w	d7
	clr.w	d7		* 念のため上位ワードをクリア
	swap.w	d7
	divu.w	#6*8,d7
	mulu.w	#6*8,d7
	swap.w	d7
	clr.w	d7		* 念のため上位ワードをクリア
	swap.w	d7
	add.w	start_dot(a5),d7	* d7.w =

	move.w	d7,d5
	add.w	#DISP_X_OFFSET,d5
	move.w	d5,d0
	lsr.w	#4,d0
	add.w	d0,d0
	movea.l	tvram_left,a2
	adda.w	d0,a2		* a2.w =
	andi.w	#15,d5		* d5.w =
	moveq.l	#0,d0
	bra	DrawLineNext


DrawLineCmd:
	moveq.l	#0,d0		* DrawLineNext の引き数
	move.b	(a4)+,d1
	cmpi.b	#'`',d1		* コマンドの終わり？
	beq	DrawLineNext
	cmpi.b	#'S',d1
	beq	DrawLineCmd_s
	cmpi.b	#'D',d1
	beq	DrawLineCmd_d
	cmpi.b	#'L',d1
	beq	DrawLineCmd_l
	cmpi.b	#'l',d1
	beq	DrawLineCmd_ls
	cmpi.b	#'G',d1
	beq	DrawLineCmd_g
	cmpi.b	#'U',d1
	beq	DrawLineCmd_u
	cmpi.b	#'u',d1
	beq	DrawLineCmd_us
	cmpi.b	#'E',d1
	beq	DrawLineCmd_e
	bra	DrawLineCmd	* ここには来ないハズ


DrawLineCmd_s:		* `Sxx`	半角文字サイズ
	bsr	get_word_num	* d0.w = `Sx` の数値部
	move.w	d0,d6
	bra	DrawLineCmd


DrawLineCmd_d:		* `Dxxx` ドットスキップ
	bsr	get_word_num	* d0.w = `Dxxx` の数値部

	move.w	d0,d5
	add.w	start_dot(a5),d5
	move.w	d5,d7		* d7.w =
	addi.w	#DISP_X_OFFSET,d5
	move.w	d5,d0
	lsr.w	#4,d0
	add.w	d0,d0
	movea.l	tvram_left,a2
	adda.w	d0,a2		* a2.w =
	andi.w	#15,d5		* d5.w =
	bra	DrawLineCmd


DrawLineCmd_l:		* `L`
	bsr	get_word_num	* d0.w = `Lxxxx` の数値部
	move.w	d0,in_link

	tst.l	d5
	bmi	DrawLineCmd_l_count

			* 表示時
	mulu.w	#link_table_size,d0
	movea.l	link_table_ptr,a0
	tst.b	link_table_in_cache(a0,d0.w)
	beq	@f
	move.w	#%11_00010000,CRTC_R21	* テキスト画面同時アクセス（黒で描画）
	bra	DrawLineCmd
@@:	move.w	#%11_00110000,CRTC_R21	* テキスト画面同時アクセス（赤で描画）
	bra	DrawLineCmd

DrawLineCmd_l_count:	* ドット数えモード時
	bra	DrawLineCmd


DrawLineCmd_ls:		* `l`	l-small だから ls ね
	tst.l	d5
	bmi	DrawLineCmd_ls_count
			* 表示時
	move.w	#%11_00010000,CRTC_R21	* テキスト画面同時アクセス（黒で描画）
	bra	DrawLineCmd
DrawLineCmd_ls_count:	* ドット数えモード時
	move.w	#-1,in_link
	bra	DrawLineCmd

	.offset	-10
g_work_image_no:	.ds.w	1
g_work_hh:		.ds.w	1
g_work_y_offset:	.ds.l	1
g_work_disp_x:		.ds.w	1

	.text
	.even
DrawLineCmd_g:		* `Gnnnn`
	link	a6,#-10
	bsr	get_word_num	* d0.w = `Gnnnn` の nnnn（イメージ番号）
	addq.w	#1,a4
	move.w	d0,d2
	move.w	d0,g_work_image_no(a6)
	bsr	get_word_num	* d0.w = `Gnnnn,hh` の hh
	addq.w	#1,a4
	move.w	d0,g_work_hh(a6)
	bsr	get_long_hex	* d0.w = `Gnnnn,hh,yyyyyyyy` の yyyyyyyy
	move.l	d0,g_work_y_offset(a6)

	move.w	d2,d0
	tst.l	d5		* ドット数数えモード？
	bmi	DrawLineCmd_g_count

			* イメージリストから各種情報を得る
	movea.l	image_table_ptr,a0
	mulu.w	#image_table_size,d0
	movea.l	image_table_image_list(a0,d0.l),a1	* a1.l = イメージリスト
	move.w	image_table_disp_x(a0,d0.l),d4	* d4.w = disp_x
	move.w	d4,g_work_disp_x(a6)


	move.w	CRTC_R21,-(sp)	* テキスト画面同時アクセス
	move.w	#%11_00100000,CRTC_R21	* テキスト画面同時アクセス（透明で描画）

			* テキスト画面に「穴」を開ける
			* （ここからグラフィック画面が透けて見える）
			* 16ドット境界によって分岐
	tst.w	d5
	beq	DrawLineCmd_g_text_16

	moveq.l	#16,d0
	sub.w	d5,d0
	cmp.w	d0,d4
	blt	DrawLineCmd_g_text_mini
	sub.w	d0,d4

	move.w	d5,d0
	add.w	d0,d0
	lea.l	DrawLineCmd_g_text_mask(pc),a0
	move.w	(a0,d0.w),CRTC_R23	* テキストマスク

	move.w	g_work_hh(a6),d0
	mulu.w	#6,d0
	jmp	@f(pc,d0.w)
@@:
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	#-1,NEXT_LINE*%A.w(a2)
	.endm
	addq.w	#2,a2
	moveq.l	#0,d5
	bra	DrawLineCmd_g_text_16


DrawLineCmd_g_text_mini:	* １回で描画できて16ドット以下の場合
	move.w	d4,d0
	add.w	d0,d0
	lea.l	DrawLineCmd_g_text_mask(pc),a0
	move.w	(a0,d0.w),d0
	not.w	d0
	ror.w	d5,d0
	move.w	d0,CRTC_R23	* テキストマスク

	move.w	g_work_hh(a6),d0
	mulu.w	#6,d0
	jmp	@f(pc,d0.w)
@@:	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	#-1,NEXT_LINE*%A.w(a2)
	.endm
	add.w	d4,d5
	bra	DrawLineCmd_g_text_end


DrawLineCmd_g_text_16:
	clr.w	CRTC_R23	* テキストマスク
	move.w	g_work_hh(a6),d0
	mulu.w	#6,d0
	lea.l	2f(pc,d0.w),a0

1:	cmpi.w	#16,d4
	blt	DrawLineCmd_g_text_last
	jmp	(a0)
2:	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	#-1,NEXT_LINE*%A.w(a2)
	.endm
	addq.w	#2,a2
	subi.w	#16,d4
	bra	1b

DrawLineCmd_g_text_last:
	move.w	d4,d0
	add.w	d0,d0
	lea.l	DrawLineCmd_g_text_mask(pc),a0
	move.w	(a0,d0.w),d0
	not.w	d0
	move.w	d0,CRTC_R23	* テキストマスク

	move.w	g_work_hh(a6),d0
	mulu.w	#6,d0
	jmp	@f(pc,d0.w)
@@:	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	#-1,NEXT_LINE*%A.w(a2)
	.endm
	move.w	d4,d5
	bra	DrawLineCmd_g_text_end

	.even
DrawLineCmd_g_text_mask:
	.dc.w	%00000000_00000000
	.dc.w	%10000000_00000000
	.dc.w	%11000000_00000000
	.dc.w	%11100000_00000000
	.dc.w	%11110000_00000000
	.dc.w	%11111000_00000000
	.dc.w	%11111100_00000000
	.dc.w	%11111110_00000000
	.dc.w	%11111111_00000000
	.dc.w	%11111111_10000000
	.dc.w	%11111111_11000000
	.dc.w	%11111111_11100000
	.dc.w	%11111111_11110000
	.dc.w	%11111111_11111000
	.dc.w	%11111111_11111100
	.dc.w	%11111111_11111110

	.even
DrawLineCmd_g_text_end:
	move.w	(sp)+,CRTC_R21	* テキスト画面同時アクセス


			* グラフィック画面に描画する
	movem.l	d1-d7/a0-a5,-(sp)
	movea.l	gvram_left,a0

	move.w	d7,d0		* d0.w = 画面左端からのドット数
	add.w	d0,d0
	adda.w	d0,a0		* a0.l = 表示する GVRAM 上のアドレス

	movea.l	image_list_data(a1),a2
	move.l	a2,d0		* = NULL の時はデータがない
	beq	DrawLineCmd_g_clear
	adda.l	g_work_y_offset(a6),a2
	move.w	g_work_hh(a6),d0
	bne	1f

	moveq.l	#16-1,d2	* d2.w = ループ回数
	bra	2f
1:
	moveq.l	#16-1,d2
	sub.w	d0,d2		* d2.w = ループ回数
	mulu.w	#512*2,d0
	adda.l	d0,a0
2:

DrawLineCmd_g_trans_y_Loop:	* グラフィック画面に転送
	move.w	g_work_disp_x(a6),d0	* d0.w = disp_x

	move.w	d0,d1
	lsr.w	#5,d0
	beq	DrawLineCmd_g_trans_x_2	* 横32ドット以下

	subq.w	#1,d0
DrawLineCmd_g_trans_x_1:	* 横32ドットまとめて転送
	movem.l	(a2)+,d3-d7/a3-a5
	movem.l	d3-d7/a3-a5,(a0)
	movem.l	(a2)+,d3-d7/a3-a5
	movem.l	d3-d7/a3-a5,32(a0)
	lea.l	64(a0),a0
	dbra	d0,DrawLineCmd_g_trans_x_1
DrawLineCmd_g_trans_x_2:
	andi.w	#31,d1
	beq	DrawLineCmd_g_trans_x_4
	subq.w	#1,d1
DrawLineCmd_g_trans_x_3:
	move.w	(a2)+,(a0)+
	dbra	d1,DrawLineCmd_g_trans_x_3
DrawLineCmd_g_trans_x_4:

	move.w	#512,d0
	sub.w	g_work_disp_x(a6),d0
	add.w	d0,d0
	adda.w	d0,a0

	move.w	image_list_x(a1),d0
	sub.w	g_work_disp_x(a6),d0
	add.w	d0,d0
	adda.w	d0,a2

	dbra	d2,DrawLineCmd_g_trans_y_Loop

DrawLineCmd_gRts:
	movem.l	(sp)+,d1-d7/a0-a5

	add.w	g_work_disp_x(a6),d7	* d7.w =

	unlk	a6
	bra	DrawLineCmd


			* データがなかったのでグラフィック画面をクリア
DrawLineCmd_g_clear:
	move.w	g_work_hh(a6),d0
	bne	1f

	moveq.l	#16-1,d2	* d2.w = ループ回数
	bra	2f
1:
	moveq.l	#16-1,d2
	sub.w	d0,d2		* d2.w = ループ回数
	mulu.w	#512*2,d0
	adda.l	d0,a0
2:
	moveq.l	#0,d3
	moveq.l	#0,d4
	moveq.l	#0,d5
	moveq.l	#0,d6
	moveq.l	#0,d7
	move.l	d3,a3
	move.l	d3,a4
	move.l	d3,a5

DrawLineCmd_g_clear_y_Loop:
	move.w	g_work_disp_x(a6),d0	* d0.w = disp_x

	move.w	d0,d1
	lsr.w	#5,d0
	beq	DrawLineCmd_g_clear_x_2

	subq.w	#1,d0
DrawLineCmd_g_clear_x_1:
	movem.l	d3-d7/a3-a5,(a0)
	movem.l	d3-d7/a3-a5,32(a0)
	lea.l	64(a0),a0
	dbra	d0,DrawLineCmd_g_clear_x_1
DrawLineCmd_g_clear_x_2:
	andi.w	#31,d1
	beq	DrawLineCmd_g_clear_x_4
	subq.w	#1,d1
DrawLineCmd_g_clear_x_3:
	move.w	d3,(a0)+
	dbra	d1,DrawLineCmd_g_clear_x_3
DrawLineCmd_g_clear_x_4:

	move.w	#512,d0
	sub.w	g_work_disp_x(a6),d0
	add.w	d0,d0
	adda.w	d0,a0

	move.w	image_list_x(a1),d0
	sub.w	g_work_disp_x(a6),d0
	add.w	d0,d0
	adda.w	d0,a2

	dbra	d2,DrawLineCmd_g_clear_y_Loop

	bra	DrawLineCmd_gRts


DrawLineCmd_g_count:		* ドット数数えモード
	move.w	g_work_image_no(a6),d0
	movea.l	image_table_ptr,a0
	mulu.w	#image_table_size,d0
	movea.l	image_table_image_list(a0,d0.l),a1	* a1.l = イメージリスト
	move.w	image_table_disp_x(a0,d0.l),d0	* d0.w = disp_x

	add.w	d0,d5
@@:	subi.w	#16,d5
	bmi	@f
	addq.w	#2,a2
	bra	@b
@@:	addi.w	#16,d5

	add.w	d0,d7

	unlk	a6
	bra	DrawLineCmd


DrawLineCmd_u:		* `U`
	bset.l	#31,d6
	bra	DrawLineCmd

DrawLineCmd_us:		* `u`
	bclr.l	#31,d6
	bra	DrawLineCmd


DrawLineCmd_e:		* `Exx`
	bsr	get_word_num	* d0.w = `Exx` の数値部

	tst.l	d5		* ドット数えモード？
	bmi	9f
*	bra	9f

	lea.l	extended_charcter,a1
	lsl.w	#5,d0
	adda.w	d0,a1

	move.w	d5,d0
	lea.l	put_16x16_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)
9:	moveq.l	#16,d0		* 文字のドット数
	addq.w	#1,a4
	bra	DrawLineNext



****	****
get_word_num:		* 文字列を数値に変換する
			* in  : a4.l = 文字列のアドレス
			* out : d0.w = 数値
			*	a4.l = 次のアドレス
	movem.l	d1,-(sp)
	moveq.l	#0,d0

@@:	moveq.l	#0,d1
	move.b	(a4)+,d1
	cmpi.b	#'0',d1
	blt	@f
	cmpi.b	#'9',d1
	bgt	@f
	subi.b	#'0',d1
	mulu.w	#10,d0
	add.w	d1,d0
	bra	@b
@@:
	subq.w	#1,a4
	movem.l	(sp)+,d1
	rts


****	****
get_long_hex:		* 16進文字列を数値に変換する
			* in  : a4.l = 文字列（16進）のアドレス
			* out : d0.l = 数値
			*	a4.l = 次のアドレス
	movem.l	d1,-(sp)
	moveq.l	#0,d0

1:	moveq.l	#0,d1
	move.b	(a4)+,d1
	cmpi.b	#'0',d1
	blt	2f
	cmpi.b	#'9',d1
	bgt	2f
	subi.b	#'0',d1
	lsl.l	#4,d0
	add.l	d1,d0
	bra	1b
2:
	cmpi.b	#'a',d1
	blt	3f
	cmpi.b	#'f',d1
	bgt	3f
	subi.b	#'a',d1
	addi.b	#10,d1
	lsl.l	#4,d0
	add.l	d1,d0
	bra	1b
3:
	subq.w	#1,a4
	movem.l	(sp)+,d1
	rts


****************************************************************
DrawLine半角後半:
*	moveq.l	#0,d0
*	move.b	dw_han0_font(a6),d0
	moveq.l	#0,d0			* フォント種類
	lea.l	DrawLine半角後半_6x12_jt(pc),a0
	cmpi.b	#6,d6
	beq	DrawLineJmp
	lea.l	DrawLine半角後半_8x16_jt(pc),a0
	cmpi.b	#8,d6
	beq	DrawLineJmp
*	lea.l	DrawLine半角後半_mp_jt(pc),a0
	bra	DrawLineJmp



****************************************************************
DrawLine全角非漢字:
*	moveq.l	#0,d0
*	move.b	dw_zen0_font(a6),d0
	lea.l	DrawLine全角非漢字_12x12_jt(pc),a0
	cmpi.b	#6,d6
	beq	DrawLineJmp
	lea.l	DrawLine全角非漢字_16x16_jt(pc),a0
	cmpi.b	#8,d6
	beq	DrawLineJmp
*	lea.l	DrawLine全角非漢字_mp_jt(pc),a0
	bra	DrawLineJmp

DrawLine全角第１水準:
*	moveq.l	#0,d0
*	move.b	dw_zen1_font(a6),d0
	lea.l	DrawLine全角第１水準_12x12_jt(pc),a0
	cmpi.b	#6,d6
	beq	DrawLineJmp
	lea.l	DrawLine全角第１水準_16x16_jt(pc),a0
	cmpi.b	#8,d6
	beq	DrawLineJmp
*	lea.l	DrawLine全角第１水準_mp_jt(pc),a0
	bra	DrawLineJmp


DrawLine全角第２水準:
	moveq.l	#0,d0
*	move.b	dw_zen2_font(a6),d0
	lea.l	DrawLine全角第２水準_12x12_jt(pc),a0
	cmpi.b	#6,d6
	beq	DrawLineJmp
	lea.l	DrawLine全角第２水準_16x16_jt(pc),a0
	cmpi.b	#8,d6
	beq	DrawLineJmp
*	lea.l	DrawLine全角第２水準_mp_jt(pc),a0
	bra	DrawLineJmp


DrawLine２バイト半角:
	moveq.l	#0,d0
	bra	DrawLineNext	*!debug
.if	0
	moveq.l	#0,d0
*	move.b	dw_han1_font(a6),d0
	lea.l	DrawLine２バイト半角_6x16_jt(pc),a0
*	cmpi.w	#12<<8|16,dw_x_size(a6)
*	beq	DrawLineJmp
*	lea.l	DrawLine２バイト半角_mp_jt(pc),a0
	bra	DrawLineJmp
.endif



*********************************************************
DrawLine半角前半_6x12_jt:
q	=	DrawLine半角前半_6x12_jt
	.dc.w	DrawLine半角前半_6x12_ROM12-q
*	.dc.w	DrawLine半角前半_6x12_ROM16-q
*	.dc.w	DrawLine半角前半_6x12_ROM24-q

DrawLine半角前半_6x12_ROM12:	* ROM12 の半角前半を 6x12 で任意のアドレスへ書く
*DrawLine半角前半_6x12_ROM16:	* ROM16 の半角前半		〃
*DrawLine半角前半_6x12_ROM24:	* ROM24 の半角前半		〃
DrawLine２バイト半角_6x16_ROM12:
	tst.l	d5		* ドット数えモード？
	bmi	9f

	moveq.l	#6,d2		* 12x12 dot
	IOCS	_FNTADR
	movea.l	d0,a1		* a1.l = フォントアドレス

	tst.l	d6	* 下線？
	bpl	2f
			* 下線の処理（だせえ）
	lea.l	font_work,a0
	moveq.l	#12-1-1,d0
1:	move.b	(a1)+,(a0)+
	dbra	d0,1b
	move.b	#-1,(a0)
	lea.l	font_work,a1

2:
	lea.l	NEXT_LINE*4(a2),a2	* 上４ドットは描かない
	move.w	d5,d0
	lea.l	put_6x12_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)
9:	moveq.l	#6,d0		* 文字のドット数
	lea.l	-NEXT_LINE*4(a2),a2
	bra	DrawLineNext

*********************************************************
DrawLine半角後半_6x12_jt:
q	=	DrawLine半角後半_6x12_jt
	.dc.w	DrawLine半角後半_6x12_ROM12-q
*	.dc.w	DrawLine半角後半_6x12_ROM16-q
*	.dc.w	DrawLine半角後半_6x12_ROM24-q

DrawLine半角後半_6x12_ROM12:	* ROM12 の半角後半を 6x12 で任意のアドレスへ書く
*DrawLine半角後半_6x12_ROM16:	* ROM16 の半角後半		〃
*DrawLine半角後半_6x12_ROM24:	* ROM24 の半角後半		〃
	tst.l	d5		* ドット数えモード？
	bmi	9f

	moveq.l	#6,d2		* 12x12 dot
	IOCS	_FNTADR
	movea.l	d0,a1		* a1.l = フォントアドレス

	tst.l	d6	* 下線？
	bpl	2f
			* 下線の処理（だせえ）
	lea.l	font_work,a0
	moveq.l	#12-1-1,d0
1:	move.b	(a1)+,(a0)+
	dbra	d0,1b
	move.b	#-1,(a0)
	lea.l	font_work,a1

2:
	lea.l	NEXT_LINE*4(a2),a2	* 上４ドットは描かない
	move.w	d5,d0
	lea.l	put_6x12_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)
9:	moveq.l	#6,d0		* 文字のドット数
	lea.l	-NEXT_LINE*4(a2),a2
	bra	DrawLineNext

*********************************************************
DrawLine全角非漢字_12x12_jt:
q	=	DrawLine全角非漢字_12x12_jt
	.dc.w	DrawLine全角非漢字_12x12_ROM12-q
*	.dc.w	DrawLine全角非漢字_12x12_ROM16-q

DrawLine全角第１水準_12x12_jt:
q	=	DrawLine全角第１水準_12x12_jt
	.dc.w	DrawLine全角第１水準_12x12_ROM12-q
*	.dc.w	DrawLine全角第１水準_12x12_ROM16-q

DrawLine全角第２水準_12x12_jt:
q	=	DrawLine全角第２水準_12x12_jt
	.dc.w	DrawLine全角第２水準_12x12_ROM12-q
*	.dc.w	DrawLine全角第２水準_12x12_ROM16-q

DrawLine全角非漢字_12x12_ROM12:
DrawLine全角第１水準_12x12_ROM12
DrawLine全角第２水準_12x12_ROM12
	tst.l	d5		* ドット数えモード？
	bmi	9f

	moveq.l	#6,d2		* 12x12 dot
	IOCS	_FNTADR
	movea.l	d0,a1		* a1.l = フォントアドレス

	tst.l	d6	* 下線？
	bpl	2f
			* 下線の処理（だせえ）
	lea.l	font_work,a0
	moveq.l	#(12-1)*2-1,d0
1:	move.b	(a1)+,(a0)+
	dbra	d0,1b
	move.b	#-1,(a0)+
	move.b	#-1,(a0)+
	lea.l	font_work,a1

2:
	lea.l	NEXT_LINE*4(a2),a2	* 上４ドットは描かない
	move.w	d5,d0
	lea.l	put_12x12_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)
9:	move.w	#12,d0		* 文字のドット数
	lea.l	-NEXT_LINE*4(a2),a2
	bra	DrawLineNext

*********************************************************
DrawLine半角前半_8x16_jt:
q	=	DrawLine半角前半_8x16_jt
	.dc.w	DrawLine半角前半_8x16_ROM12-q
*	.dc.w	DrawLine半角前半_8x16_ROM16-q
*	.dc.w	DrawLine半角前半_8x16_ROM24-q

DrawLine半角前半_8x16_ROM12:	* ROM12 の半角前半を 8x16 で任意のアドレスへ書く
*DrawLine半角前半_8x16_ROM16:	* ROM16 の半角前半		〃
*DrawLine半角前半_8x16_ROM24:	* ROM24 の半角前半		〃
*DrawLine２バイト半角_6x16_ROM12:
	tst.l	d5		* ドット数えモード？
	bmi	9f

	moveq.l	#8,d2		* 16x16 dot
	IOCS	_FNTADR
	movea.l	d0,a1		* a1.l = フォントアドレス

	tst.l	d6	* 下線？
	bpl	2f

			* 下線の処理（だせえ）
	lea.l	font_work,a0
	moveq.l	#16-1-1,d0
1:	move.b	(a1)+,(a0)+
	dbra	d0,1b
	move.b	#-1,(a0)
	lea.l	font_work,a1

2:
	move.w	d5,d0
	lea.l	put_8x16_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)
9:	moveq.l	#8,d0		* 文字のドット数
	bra	DrawLineNext

*********************************************************
DrawLine半角後半_8x16_jt:
q	=	DrawLine半角後半_8x16_jt
	.dc.w	DrawLine半角後半_8x16_ROM12-q
*	.dc.w	DrawLine半角後半_8x16_ROM16-q
*	.dc.w	DrawLine半角後半_8x16_ROM24-q

DrawLine半角後半_8x16_ROM12:	* ROM12 の半角後半を 8x16 で任意のアドレスへ書く
*DrawLine半角後半_8x16_ROM16:	* ROM16 の半角後半		〃
*DrawLine半角後半_8x16_ROM24:	* ROM24 の半角後半		〃
*DrawLine２バイト半角_6x16_ROM12:
	tst.l	d5		* ドット数えモード？
	bmi	9f

	moveq.l	#8,d2		* 16x16 dot
	IOCS	_FNTADR
	movea.l	d0,a1		* a1.l = フォントアドレス

	tst.l	d6	* 下線？
	bpl	2f

			* 下線の処理（だせえ）
	lea.l	font_work,a0
	moveq.l	#16-1-1,d0
1:	move.b	(a1)+,(a0)+
	dbra	d0,1b
	move.b	#-1,(a0)
	lea.l	font_work,a1

2:
	move.w	d5,d0
	lea.l	put_8x16_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)
9:	moveq.l	#8,d0		* 文字のドット数
	bra	DrawLineNext

****************************************************************
DrawLine全角非漢字_16x16_jt:
q	=	DrawLine全角非漢字_16x16_jt
	.dc.w	DrawLine全角非漢字_16x16_ROM12-q
*	.dc.w	DrawLine全角非漢字_16x16_ROM16-q

DrawLine全角第１水準_16x16_jt:
q	=	DrawLine全角第１水準_16x16_jt
	.dc.w	DrawLine全角第１水準_16x16_ROM12-q
*	.dc.w	DrawLine全角第１水準_16x16_ROM16-q

DrawLine全角第２水準_16x16_jt:
q	=	DrawLine全角第２水準_16x16_jt
	.dc.w	DrawLine全角第２水準_16x16_ROM12-q
*	.dc.w	DrawLine全角第２水準_16x16_ROM16-q

DrawLine全角非漢字_16x16_ROM12:
DrawLine全角第１水準_16x16_ROM12
DrawLine全角第２水準_16x16_ROM12
	tst.l	d5		* ドット数えモード？
	bmi	9f

	moveq.l	#8,d2		* 16x16 dot
	IOCS	_FNTADR
	movea.l	d0,a1		* a1.l = フォントアドレス

	tst.l	d6	* 下線？
	bpl	2f

			* 下線の処理（だせえ）
	lea.l	font_work,a0
	moveq.l	#(16-1)*2-1,d0
1:	move.b	(a1)+,(a0)+
	dbra	d0,1b
	move.b	#-1,(a0)+
	move.b	#-1,(a0)+
	lea.l	font_work,a1

2:
	move.w	d5,d0
	lea.l	put_16x16_jt(pc),a0
	add.w	d0,d0
	move.w	(a0,d0.w),d0
	jsr	(a0,d0.w)
9:	moveq.l	#16,d0		* 文字のドット数
	bra	DrawLineNext



****************************************************************
****************************************************************
*	正方向低速スクロール
_ScrollForward:
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

			* エラーチェック
	move.l	8(a6),d0
	beq	ScrollForwardRts
	movea.l	d0,a4			* a4.l = &xp_text
	move.l	xptext_current_line(a4),d0
	addi.l	#DISP_Y,d0
	cmp.l	xptext_line(a4),d0
	bge	ScrollForwardRts

			* ワーク部 TEXTVRAM に１行描画
	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	move.l	xptext_current_line(a4),d0
	addq.l	#1,xptext_current_line(a4)
	addi.l	#DISP_Y,d0
	mulu.w	#size_of_line_ptr,d0
	movea.l	xptext_line_ptr(a4),a5
	adda.w	d0,a5		* a5.l = 表示する line_ptr
	lea.l	TEXTVRAM+NEXT_LINE*32*16,a2
	move.w	gr_y,d0
	mulu.w	#512*2,d0
	lea.l	GVRAM,a3
	adda.l	d0,a3
	moveq.l	#0,d5
	bsr	DrawLine

	move.l	8(a6),-(sp)
	bsr	DrawScbar
	addq.w	#4,sp

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	bsr	ScrollForward_continue


ScrollForwardRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


ScrollForward_continue:		* 全体をスクロールアップ
	moveq.l	#4-1,d5
	bra	2f			* 初回は垂直同期を待たない
1:
	bsr	_WaitVdisp
2:
	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	move.w	gr_y,d0
	addi.w	#4,d0
	andi.w	#511,d0
	bsr	set_gr_y

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	move.w	#1<<8|0,d1		* d1.w = 転送元<<8|転送先
	move.w	#(DISP_Y+1)*4,d2	* d2.w = コピー回数
	moveq.l	#%0011,d3
	IOCS	_TXRASCPY

	dbra	d5,1b
			* ワーク用 TEXTVRAM をクリア
	move.w	#(128+4)<<8|128,d1	* d1.w = 転送元<<8|転送先
	moveq.l	#4,d2			* d2.w = コピー回数
	moveq.l	#%1111,d3
	IOCS	_TXRASCPY

	rts


****************************************************************
*	逆方向低速スクロール
_ScrollBackward:
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

			* エラーチェック
	move.l	8(a6),d0
	beq	ScrollBackwardRts
	movea.l	d0,a4			* a4.l = &xp_text
	move.l	xptext_current_line(a4),d0
	beq	ScrollBackwardRts
	cmp.l	xptext_line(a4),d0
	bge	ScrollBackwardRts

			* ワーク部 TEXTVRAM に１行描画
	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	move.l	xptext_current_line(a4),d0
	subq.l	#1,xptext_current_line(a4)
	subq.l	#1,d0
	mulu.w	#size_of_line_ptr,d0
	movea.l	xptext_line_ptr(a4),a5
	adda.w	d0,a5		* a5.l = 表示する line_ptr
	lea.l	TEXTVRAM+NEXT_LINE*32*16,a2
	move.w	gr_y,d0
	addi.w	#(DISP_Y-1)*16,d0
	andi.w	#511,d0
	mulu.w	#512*2,d0
	lea.l	GVRAM,a3
	adda.l	d0,a3
	moveq.l	#0,d5
	bsr	DrawLine

	move.l	8(a6),-(sp)
	bsr	DrawScbar
	addq.w	#4,sp

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	bsr	ScrollBackward_continue

ScrollBackwardRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


ScrollBackward_continue:	* 全体をスクロールダウン
	move.w	#(128+4-1)<<8|0,d4	* d4.w = 転送元<<8|転送先

	moveq.l	#4-1,d5
	bra	2f			* 初回は垂直同期を待たない
1:
	bsr	_WaitVdisp
2:
	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	move.w	gr_y,d0
	subi.w	#4,d0
	andi.w	#511,d0
	bsr	set_gr_y

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	move.w	#(DISP_Y*4-2)<<8|DISP_Y*4-1,d1		* d1.w = 転送元<<8|転送先
	move.w	#DISP_Y*4-1,d2		* d2.w = コピー回数
	move.w	#%10000000_00000011,d3	* d3.w = コピー方向<<15|アクセスプレーン
	IOCS	_TXRASCPY

	move.w	d4,d1			* d1.w = 転送元<<8|転送先
	moveq.l	#1,d2			* d2.w = コピー回数
	move.w	#%00000000_00000011,d3	* d3.w = コピー方向<<15|アクセスプレーン
	IOCS	_TXRASCPY
	subi.w	#(1<<8),d4

	dbra	d5,1b
			* ワーク用 TEXTVRAM をクリア
	move.w	#(128+4)<<8|128,d1	* d1.w = 転送元<<8|転送先
	moveq.l	#4,d2			* d2.w = コピー回数
	moveq.l	#%1111,d3
	IOCS	_TXRASCPY
@@:
	rts


****************************************************************
*	正方向高速スクロール
_ScrollFastForward:
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	move.l	8(a6),d0
	beq	ScrollForwardRts
	movea.l	d0,a4			* a4.l = &xp_text
	move.l	xptext_current_line(a4),d0
	addi.l	#DISP_Y,d0
	cmp.l	xptext_line(a4),d0
	bge	ScrollFastForwardRts

			* ワーク部 TEXTVRAM に１行描画
	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	move.l	xptext_current_line(a4),d0
	addq.l	#1,xptext_current_line(a4)
	addi.l	#DISP_Y,d0
	mulu.w	#size_of_line_ptr,d0
	movea.l	xptext_line_ptr(a4),a5
	adda.w	d0,a5		* a5.l = 表示する line_ptr
	lea.l	TEXTVRAM+NEXT_LINE*32*16,a2
	move.w	gr_y,d0
	mulu.w	#512*2,d0
	lea.l	GVRAM,a3
	adda.l	d0,a3
	moveq.l	#0,d5
	bsr	DrawLine

			* 全体をスクロールアップ
	move.w	gr_y,d0
	addi.w	#16,d0
	andi.w	#511,d0
	bsr	set_gr_y

	move.w	#4<<8|0,d1		* d1.w = 転送元<<8|転送先
	move.w	#(DISP_Y+1)*4,d2	* d2.w = コピー回数
	moveq.l	#%0011,d3
	IOCS	_TXRASCPY

			* ワーク用 TEXTVRAM をクリア
	move.w	#(128+4)<<8|128,d1	* d1.w = 転送元<<8|転送先
	moveq.l	#4,d2			* d2.w = コピー回数
	moveq.l	#%1111,d3
	IOCS	_TXRASCPY

	move.l	8(a6),-(sp)
	bsr	DrawScbar
	addq.w	#4,sp

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

ScrollFastForwardRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
*	逆方向高速スクロール
_ScrollFastBackward:
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	move.l	8(a6),d0
	beq	ScrollBackwardRts
	movea.l	d0,a4			* a4.l = &xp_text
	move.l	xptext_current_line(a4),d0
	beq	ScrollFastBackwardRts
	cmp.l	xptext_line(a4),d0
	bge	ScrollFastBackwardRts

			* ワーク部 TEXTVRAM に１行描画
	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	move.l	xptext_current_line(a4),d0
	subq.l	#1,d0
	move.l	d0,xptext_current_line(a4)
	mulu.w	#size_of_line_ptr,d0
	movea.l	xptext_line_ptr(a4),a5
	adda.w	d0,a5		* a5.l = 表示する line_ptr
	lea.l	TEXTVRAM+NEXT_LINE*32*16,a2
	move.w	gr_y,d0
	addi.w	#(DISP_Y-1)*16,d0
	andi.w	#511,d0
	mulu.w	#512*2,d0
	lea.l	GVRAM,a3
	adda.l	d0,a3
	moveq.l	#0,d5
	bsr	DrawLine

			* 全体をスクロールダウン
	move.w	gr_y,d0
	subi.w	#16,d0
	andi.w	#511,d0
	bsr	set_gr_y

	move.w	#((DISP_Y-1)*4-1)<<8|DISP_Y*4-1,d1	* d1.w = 転送元<<8|転送先
	move.w	#(DISP_Y-1)*4,d2	* d2.w = コピー回数
	move.w	#%10000000_00000011,d3	* d3.w = コピー方向<<15|アクセスプレーン
	IOCS	_TXRASCPY

	move.w	#128<<8|0,d1	* d1.w = 転送元<<8|転送先
	moveq.l	#4,d2			* d2.w = コピー回数
	move.w	#%00000000_00000011,d3	* d3.w = コピー方向<<15|アクセスプレーン
	IOCS	_TXRASCPY

			* ワーク用 TEXTVRAM をクリア
	move.w	#(128+4)<<8|128,d1	* d1.w = 転送元<<8|転送先
	moveq.l	#4,d2			* d2.w = コピー回数
	moveq.l	#%1111,d3
	IOCS	_TXRASCPY

	move.l	8(a6),-(sp)
	bsr	DrawScbar
	addq.w	#4,sp

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

ScrollFastBackwardRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
*	１画面進む
_PageForward:
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	move.l	8(a6),d0
	beq	PageForwardRts
	movea.l	d0,a4			* a4.l = &xp_text

	move.l	xptext_current_line(a4),d0
	addi.l	#DISP_Y,d0
	move.l	xptext_line(a4),d1
	subi.l	#DISP_Y,d1
	ble	PageForwardRts
	cmp.l	d1,d0
	blt	@f
	move.l	d1,d0
@@:
	cmp.l	xptext_current_line(a4),d0
	beq	PageForwardRts
	move.l	d0,xptext_current_line(a4)

	pea.l	(a4)
	bsr	DrawPage
	addq.w	#4,sp

PageForwardRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
*	１画面戻る
_PageBackward:
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	move.l	8(a6),d0
	beq	PageBackwardRts
	movea.l	d0,a4			* a4.l = &xp_text

	move.l	xptext_current_line(a4),d0
	beq	PageBackwardRts
	subi.l	#DISP_Y,d0
	bpl	@f
	moveq.l	#0,d0
@@:
	cmp.l	xptext_current_line(a4),d0
	beq	PageBackwardRts
	move.l	d0,xptext_current_line(a4)

	pea.l	(a4)
	bsr	DrawPage
	addq.w	#4,sp

PageBackwardRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
_PageTop:		* ページの先頭に
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	move.l	8(a6),d0
	beq	PageTopRts
	movea.l	d0,a4			* a4.l = &xp_text

	move.l	xptext_current_line(a4),d0
	beq	PageTopRts

	clr.l	xptext_current_line(a4)

	pea.l	(a4)
	bsr	DrawPage
	addq.w	#4,sp
PageTopRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
_PageEnd:		* ページの末尾に
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	move.l	8(a6),d0
	beq	PageEndRts
	movea.l	d0,a4			* a4.l = &xp_text

	move.l	xptext_line(a4),d0
	subi.l	#DISP_Y,d0
	cmp.l	xptext_current_line(a4),d0
	beq	PageEndRts

	move.l	d0,xptext_current_line(a4)

	pea.l	(a4)
	bsr	DrawPage
	addq.w	#4,sp
PageEndRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
_PageJump:		* 指定された行数のページへ
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	move.l	8(a6),d0
	beq	PageJumpRts
	movea.l	d0,a4			* a4.l = &xp_text

	move.l	xptext_line(a4),d0
	subi.l	#DISP_Y,d0
	cmp.l	xptext_current_line(a4),d0
	blt	PageJumpRts

	pea.l	(a4)
	bsr	DrawPage
	addq.w	#4,sp
PageJumpRts:
	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
DrawPage:		* １画面表示（外部からは呼ばれない）
			* reg : a6 のみ保存
	link	a6,#0
	movem.l	a6,-(sp)

	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	bsr	_ClearText01

	lea.l	TEXTVRAM,a2
	movea.l	8(a6),a4	* a4.l = xptext

	move.l	xptext_current_line(a4),d0
	move.l	d0,d1
	mulu.w	#size_of_line_ptr,d0
	movea.l	xptext_line_ptr(a4),a5
	adda.w	d0,a5		* a5.l = 表示する line_ptr

	lea.l	GVRAM,a3
	moveq.l	#0,d0
	bsr	set_gr_y

	moveq.l	#DISP_Y-1,d2
DrawPageLoop:		* 行数ぶんループ
	cmp.l	xptext_line(a4),d1
	bge	DrawPageEnd
	addq.l	#1,d1

	moveq.l	#0,d5
	movem.l	d1-d2/a2-a4,-(sp)
	bsr	DrawLine
	movem.l	(sp)+,d1-d2/a2-a4
	lea.l	NEXT_LINE*16(a2),a2
	lea.l	512*2*16(a3),a3
	lea.l	size_of_line_ptr(a5),a5

	dbra	d2,DrawPageLoop

DrawPageEnd:
	move.l	8(a6),-(sp)
	bsr	DrawScbar
	addq.w	#4,sp

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	movem.l	(sp)+,a6
	unlk	a6
	rts


****************************************************************
*	１画面表示ルーチン

_DrawTextAll:		* in  : 4(sp).l = xp_text のアドレス
	link	a6,#0
	movem.l	d1-d7/a0-a5,-(sp)

	move.l	8(a6),d0
	beq	DrawTextAllRts
	movea.l	d0,a4			* a4.l = &xp_text

	move.l	xptext_image_table(a4),image_table_ptr
	move.l	xptext_link_table(a4),link_table_ptr

	bsr	SetTextColor

	move.l	8(a6),-(sp)
	bsr	DrawPage
	addq.w	#4,sp

DrawTextAllRts:
	moveq.l	#0,d0

	movem.l	(sp)+,d1-d7/a0-a5
	unlk	a6
	rts


****************************************************************
_SetConfigColor:
	sf.b	_color_mode	* WebXpression.cnf で指定された色を利用する
	bsr	SetTextColor
	rts


_SetHtmlColor:
	st.b	_color_mode	* HTML で指定された色を利用する
	bsr	SetTextColor
	rts

****************************************************************
SetTextColor:		* テキスト表示部のパレットを設定
	movem.l	a0-a1,-(sp)

	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

			* スクロール部のパレットを設定
	lea.l	_config_color,a0
	tst.b	_color_mode	* どっちのモード？
	beq	@f
	lea.l	_html_color,a0
@@:	lea.l	TEXTPALET,a1
	move.w	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	addq.w	#2,a0
	addq.w	#2,a1
	move.w	(a0)+,(a1)+

	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	movem.l	(sp)+,a0-a1
	rts


****************************************************************
_DrawBack:	* 背景を描画
	movem.l	d0-d2/a0-a1,-(sp)

	IOCS	_MS_CUROF

	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	lea.l	TEXTVRAM,a0

	moveq.l	#4-1,d1
DrawBackYLoop:
*			* 左端のバー
*	move.w	#%1_01000000,CRTC_R21	* テキスト画面同時アクセス
*	move.b	#-1,(a0)+
	move.b	#0,(a0)+
			* スクロール部をスキップ
	lea.l	512/8(a0),a0
	move.b	#0,(a0)+
			* 固定部を描く
	move.w	#%1_01000000,CRTC_R21	* テキスト画面同時アクセス
	move.w	#256/8-1,d2
	moveq.l	#-1,d0
@@:	move.b	d0,(a0)+
	dbra	d2,@b

	lea.l	256/8-2(a0),a0
	dbra	d1,DrawBackYLoop

		* あとはラスターコピー
	move.w	#512/4+3*4-1,d4
	move.w	#0<<8|1,d1		* d1.w = 転送元<<8|転送先
@@:	moveq.l	#1,d2			* d2.w = コピー回数
	move.w	#%00000000_00001111,d3	* d3.w = コピー方向<<15|アクセスプレーン
	IOCS	_TXRASCPY
	addq.w	#1,d1
	dbra	d4,@b

	move.w	#0,CRTC_R21	* テキスト画面同時アクセス
	movea.l	(sp)+,a1
	IOCS	_B_SUPER

	IOCS	_MS_CURON

	movem.l	(sp)+,d0-d2/a0-a1
	rts


****************************************************************



****************************************************************
_CheckLink:		* リンク上にマウスカーソルがあるかチェック
			* out : d0.w >= リンク番号
			*	      <  0 ならリンク外
	link	a6,#0
	movem.l	d1-d7/a1-a6,-(sp)

	moveq.l	#-1,d5		* ドット数数えモード
	move.w	8(a6),d0	* d0.w = x
	subi.w	#DISP_X_OFFSET,d0
	move.w	d0,check_mouse_x

			* マウスカーソルがスクロール領域（テキスト部）に
			* あるので、リンクの上にあるかどうかチェック
	moveq.l	#0,d1
	move.w	10(a6),d1	* d1.w = y
	lsr.w	#4,d1
	movea.l	12(a6),a0	* a0.l = チェックする xptext
	movea.l	xptext_line_ptr(a0),a5	* a5.l =
	move.l	xptext_current_line(a0),d0	* d0.l =
	add.l	d0,d1		* d1.l = マウスカーソルのある行の y
	cmp.l	xptext_line(a0),d1
	bge	CheckLinkNoScroll

	mulu.w	#size_of_line_ptr,d1
	adda.w	d1,a5		* a5.l = マウスカーソルがある行の行管理テーブル

	lea.l	TEXTVRAM,a2	* 必要ないはず（画面に描かれたらバグ）
	suba.l	a3,a3		*		〃
	bsr	DrawLine

CheckLinkRts:
	movem.l	(sp)+,d1-d7/a1-a6
	unlk	a6
	rts

CheckLinkNoScroll:
	moveq.l	#-1,d0
	bra	CheckLinkRts




****************************************************************
_ClearText:	* TEXTVRAM 全画面クリア
	movem.l	d1-d4/a0-a1,-(sp)

	IOCS	_MS_CUROF

	suba.l	a1,a1		* スーパーに
	IOCS	_B_SUPER
	move.l	d0,-(sp)

	move.w	CRTC_R21,-(sp)
	move.w	#%1_11111111,CRTC_R21	* テキスト画面同時アクセス

	lea.l	TEXTVRAM,a0
	moveq.l	#0,d0
	moveq.l	#128*4/4/4-1,d1
@@:	move.l	d0,(a0)+
	move.l	d0,(a0)+
	move.l	d0,(a0)+
	move.l	d0,(a0)+
	dbra	d1,@b

		* あとはラスターコピー
	move.w	#1024/4-1,d4
	move.w	#0<<8|1,d1		* d1.w = 転送元<<8|転送先
@@:	moveq.l	#1,d2			* d2.w = コピー回数
	move.w	#%00000000_00001111,d3	* d3.w = コピー方向<<15|アクセスプレーン
	IOCS	_TXRASCPY
	addq.w	#1,d1
	dbra	d4,@b

	move.w	(sp)+,CRTC_R21

	move.l	(sp)+,d0
	bmi	@f			* 既にスーパーだった
	movea.l	d0,a1
	IOCS	_B_SUPER
@@:
	IOCS	_MS_CURON

	movem.l	(sp)+,d1-d4/a0-a1
	rts


****************************************************************
_ClearText01:	* TEXTVRAM 0/1 ページ全画面クリア
	movem.l	d1-d4/a0,-(sp)

	IOCS	_MS_CUROF

	move.w	CRTC_R21,-(sp)
	move.w	#%1_00110011,CRTC_R21	* テキスト画面同時アクセス

	lea.l	TEXTVRAM,a0
	moveq.l	#0,d0
	moveq.l	#128*4/4/4-1,d1
@@:	move.l	d0,(a0)+
	move.l	d0,(a0)+
	move.l	d0,(a0)+
	move.l	d0,(a0)+
	dbra	d1,@b

		* あとはラスターコピー
	move.w	#1024/4-1,d4
	move.w	#0<<8|1,d1		* d1.w = 転送元<<8|転送先
@@:	moveq.l	#1,d2			* d2.w = コピー回数
	move.w	#%00000000_00000011,d3	* d3.w = コピー方向<<15|アクセスプレーン
	IOCS	_TXRASCPY
	addq.w	#1,d1
	dbra	d4,@b

	move.w	(sp)+,CRTC_R21

	IOCS	_MS_CURON

	movem.l	(sp)+,d1-d4/a0
	rts


*********************************************************
set_gr_y:		* グラフィック画面のスクロール座標をセット
			* in  : d0.w = y 座標
	movem.l	a0,-(sp)

	move.w	d0,gr_y
	swap.w	d0
	move.w	#512-DISP_X_OFFSET,d0	* x 座標は常に固定値
	swap.w	d0

	lea.l	CRTC_R12,a0
	move.l	d0,(a0)+
	move.l	d0,(a0)+
	move.l	d0,(a0)+
	move.l	d0,(a0)+

	movem.l	(sp)+,a0
	rts



*********************************************************
reverse_button:		* テキスト画面に描かれたボタンを反転する
			* in  : move.l	#X0.shl.16+Y0,-(sp)
			*	move.l	#X1.shl.16+Y1,-(sp)
			*	bsr	reverse_button
			*	lea.l	10(sp),sp
	link	a6,#0
	movem.l	d1-d7,-(sp)

	move.w	CRTC_R21,-(sp)	**
	clr.w	CRTC_R21

	move.w	12(a6),d4	* d4.w = X0
	move.w	14(a6),d5	* d5.w = Y0
	move.w	8(a6),d6	* d6.w = X1
	move.w	10(a6),d7	* d7.w = Y1

	move.w	d6,d2
	sub.w	d4,d2
	addq.w	#1,d2		* d2.w = X length
	move.w	d7,d3
	sub.w	d5,d3
	addq.w	#1,d3		* d3.w = Y length

	move.w	d1,-(sp)
	move.w	d2,-(sp)
	move.w	d5,-(sp)
	move.w	d4,-(sp)
	bsr	text_xline
	addq.w	#8,sp

	move.w	d1,-(sp)
	move.w	d3,-(sp)
	move.w	d5,-(sp)
	move.w	d4,-(sp)
	bsr	text_yline
	addq.w	#8,sp

	move.w	d1,-(sp)
	move.w	d2,-(sp)
	move.w	d7,-(sp)
	move.w	d4,-(sp)
	bsr	text_xline
	addq.w	#8,sp

	move.w	d1,-(sp)
	move.w	d3,-(sp)
	move.w	d5,-(sp)
	move.w	d6,-(sp)
	bsr	text_yline
	addq.w	#8,sp

	move.w	(sp)+,CRTC_R21	**

	movem.l	(sp)+,d1-d7
	unlk	a6
	rts


text_xline:
	link	a6,#0
	movem.l	d1-d7,-(sp)

	moveq.l	#0,d6
	moveq.l	#0,d7
	move.w	8(a6),d6	* d6.w = X
	move.w	10(a6),d7	* d7.w = Y
	move.w	12(a6),d5
	subq.w	#1,d5		* d5.w = length-1
@@:
	moveq.l	#0,d3
	moveq.l	#0,d4
	move.w	d6,d3		* d3.w = X
	move.w	d7,d4		* d4.w = Y

	move.l	d3,d0
	andi.l	#7,d0
	moveq.l	#7,d1
	sub.b	d0,d1
	moveq.l	#0,d0
	bset.l	d1,d0		* d0.b =
	lsr.l	#3,d3		* 1byte = 8dot

	lsl.l	#7,d4
	lea.l	TEXTVRAM+$2_0000*2,a0
	adda.l	d3,a0
	adda.l	d4,a0

	eor.b	d0,(a0)

	addq.w	#1,d6
	dbra	d5,@b

	movem.l	(sp)+,d1-d7
	unlk	a6
	rts


text_yline:
	link	a6,#0
	movem.l	d1/d5-d7,-(sp)

	moveq.l	#0,d6
	moveq.l	#0,d7
	move.w	8(a6),d6	* d6.w = X
	move.w	10(a6),d7	* d7.w = Y
	move.w	12(a6),d5
	subq.w	#1,d5		* d5.w = length-1

	move.l	d6,d0
	andi.l	#7,d0
	moveq.l	#7,d1
	sub.b	d0,d1
	moveq.l	#0,d0
	bset.l	d1,d0		* d0.b =
	lsr.l	#3,d6		* 1byte = 8dot

	lsl.l	#7,d7
	lea.l	TEXTVRAM+$2_0000*2,a0
	adda.l	d6,a0
	adda.l	d7,a0

@@:	eor.b	d0,(a0)
	lea.l	128(a0),a0
	dbra	d5,@b

	movem.l	(sp)+,d1/d5-d7
	unlk	a6
	rts


*********************************************************
_WaitVdisp:		* 垂直同期待ち
	movem.l	a1,-(sp)

	suba.l	a1,a1
	IOCS	_B_SUPER
	move.l	d0,-(sp)		**

@@:	btst.b	#4,GPIP_DATA
	beq	@b
@@:	btst.b	#4,GPIP_DATA
	bne	@b

	move.l	(sp)+,d0		**
	bmi	@f			* 既にスーパーだった
	movea.l	d0,a1
	IOCS	_B_SUPER
@@:
	movem.l	(sp)+,a1
	rts


*********************************************************
DrawScbar:		* スクロールバーを描画する
	link	a6,#0
	movem.l	d1/a1-a2/a4,-(sp)

	IOCS	_MS_CUROF
			* エラーチェック
	move.l	8(a6),d0
	beq	DrawScbarRts
	movea.l	d0,a4			* a4.l = &xp_text

			* 以前描いたスクロールバーを消す
	move.l	old_scbar_address,d0	* 前回描画した？
	beq	DrawScbarDraw
	movea.l	d0,a1
	movea.l	d0,a2
	adda.l	#$2_0000,a2
	lea.l	scbar_erase_charcter,a0	* a0.l = 転送元パターン
			* ２ページ目
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15	* HAS.X の拡張疑似命令
	move.w	(a0)+,NEXT_LINE*%A(a1)
	.endm
			* ３ページ目
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a0)+,NEXT_LINE*%A(a2)
	.endm

DrawScbarDraw:
	move.l	xptext_line(a4),d0	* テキストの行数が１画面以下か？
	subi.l	#DISP_Y,d0
	bgt	@f
	clr.l	old_scbar_address	* 次は消さなくて良い
	bra	DrawScbarRts
@@:
			* TEXTVRAM 上のアドレスを求める
	move.l	d0,-(sp)
	move.l	xptext_current_line(a4),-(sp)
	bsr	_CalcScbarY
	addq.w	#8,sp

	lsl.l	#7,d0
	lea.l	TEXTVRAM+$2_0000*2+66,a1
	adda.l	d0,a1
	move.l	a1,old_scbar_address
	movea.l	a1,a2
	adda.l	#$2_0000,a2
	lea.l	scbar_charcter,a0
			* ２ページ目
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15	* HAS.X の拡張疑似命令
	move.w	(a0)+,NEXT_LINE*%A(a1)
	.endm
			* ３ページ目
	.irp	%A,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
	move.w	(a0)+,NEXT_LINE*%A(a2)
	.endm

DrawScbarRts:
	IOCS	_MS_CURON

	movem.l	(sp)+,d1/a1-a2/a4
	unlk	a6
	rts


*********************************************************
_CalcScbarY:			* 現在の行数からスクロールバーのサムの位置を得る
				* in  : 4(sp).l = curent_line
				*	8(sp).l = line-DISP_Y
				* out : d0.l = scbar_y ( 0 ～ 512-16 )

	move.l	4(sp),d0	* d0.l = current_line
	mulu.w	#(512-16),d0	* (512-16) = Ｙサイズ - スクロールバーのつまみＹサイズ
	divu.w	8+2(sp),d0
	swap.w	d0		* 上位ワードをクリア
	clr.w	d0
	swap.w	d0

	rts


_CalcScbarLine:			* スクロールバーのサムの位置から行数を得る
				* in  : 4(sp).l = mouse_y
				*	8(sp).l = line-DISP_Y
				* out : d0.l = current_line
	move.w	4+2(sp),d0	* d0.w = mouse_y
	cmpi.w	#512-16,d0
	bcs	@f
	move.w	#512-16,d0
@@:	mulu.w	8+2(sp),d0	*      *= (line-DISP_Y)
	divu.w	#512-16,d0
	swap.w	d0		* 上位ワードをクリア
	clr.w	d0
	swap.w	d0

	rts


*********************************************************
	.data
	.even

old_scbar_address:
	.dc.l	0

scbar_erase_charcter:		* スクロールバーの消去パターン
				* ページ２
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111

	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111
	.dc.w	%11111111_11111111

				* ページ３
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001

	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001
	.dc.w	%10111111_11111001

scbar_charcter:		* スクロールバーのパターン
				* ページ２
	.dc.w	%10000000_00000001
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111

	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%10111111_11111111
	.dc.w	%11111111_11111111

				* ページ３
	.dc.w	%11111111_11111111
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011

	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11000000_00000011
	.dc.w	%11111111_11111111



extended_charcter:
				* パターン０
	.dc.w	%00000000_00000000
	.dc.w	%01111111_11111110
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010

	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01111111_11111110
	.dc.w	%00000000_00000000

				* パターン１
	.dc.w	%00000000_00000000
	.dc.w	%01111111_11111110
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010

	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01000000_00000010
	.dc.w	%01111111_11111110
	.dc.w	%00000000_00000000

