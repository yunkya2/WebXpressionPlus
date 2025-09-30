*	gifl.s


	.include	DOSCALL.MAC
	.include	gifl.inc

	.xdef	_gifdecodemain



* オリジナルの main.has（コメント by Mitsuky）

		.text
		.even

a7buf:	.ds.l	1

_gifdecodemain:
		movem.l	d3-d7/a3-a6,-(sp)
		move.l	a7,a7buf

		move.l	40(sp),a5
		move.l	44(sp),_srcfile(a5)

		bsr	get_filesize	* ファイルオープン，ついでにファイルサイズ取得

		bsr	gif_header_read	* ヘッダ解析
		tst.l	d0
		bmi	error

		bsr	read_adr_get	* 読み込み＆メモリセット
		tst.l	d0
		bmi	error
		bsr	gif_decode

		move.l	a5,d0
		move.l	a7buf,a7
		movem.l	(sp)+,d3-d7/a3-a6
		rts

error:
NO_MEMORY:
		move.l	a7buf,a7
		ori.l	#$80000000,d0
		movem.l	(sp)+,d3-d7/a3-a6
		rts


*-------------------------------------------------------------------------------
* ファイルオープン＆画像データファイルサイズ取得

get_filesize:
		moveq.l	#1,d6
		move.w	#0,-(sp)
		move.l	_srcfile(a5),-(sp)
		DOS	_OPEN
		addq.l	#6,sp
		move.w	d0,_file(a5)
		bmi	error
		move.w	#2,-(sp)
		pea.l	0.w
		move.w	_file(a5),-(sp)
		DOS	_SEEK
		move.l	d0,d1
		move.w	#0,6(sp)
		DOS	_SEEK
		addq.l	#8,sp
		move.l	d1,_src_fsize(a5)
		moveq.l	#0,d0
		rts


*-------------------------------------------------------------------------------
* 読み込み＆メモリセット

read_adr_get:
	tst.b	_gif_hispeed+1(a5)		* インターレスか？
	beq	read_adr_get256_normal		* 違うのなら normal へ

read_adr_get256_interlace:
		clr.b	_gif_hispeed(a5)
		move.w	_line(a5),d0
		mulu.w	_colum(a5),d0
		addq.l	#1,d0
		and.w	#$fffe,d0
		move.l	d0,d1

		* インターレス高速展開のため，通常の倍の仮想GRAMを用意
		add.l	d0,d0

		add.l	_src_fsize(a5),d0
		movem.l	d1-d2/a0-a2,-(sp)
			move.l	d0,-(sp)
			move.l	d0,_buff_size(a5)
.if 1
			move.w	#2,-(sp)
			DOS	_MALLOC2
			addq.l	#6,sp
.else
			jbsr	_malloc
			addq.l	#4,sp
.endif
		movem.l	(sp)+,d1-d2/a0-a2
		tst.l	d0
		bmi	NO_MEMORY
		move.l	d0,_buff_addr(a5)
		move.l	d0,a1
		adda.l	d1,a1
		move.l	a1,_buff_addr256(a5)
* ファイルバッファを別に確保．
		adda.l	d1,a1

			add.l	d1,d1
			add.l	_src_fsize(a5),d1
			move.l	_buff_size(a5),d2

			cmp.l	d2,d1
			bhi	error				* ［少メモリさようなら］
read_adr_get256_ex_all:
			st.b	_gif_hispeed(a5)

			moveq.l	#3,d6
			move.l	_src_fsize(a5),-(sp)
			move.l	a1,-(sp)
			move.w	_file(a5),-(sp)
			DOS	_READ
			lea	10(sp),sp
			tst.l	d0
			bmi	error
		add.l	a1,d0
		move.l	d0,_enddata_addr(a5)

		move.w	_file(a5),-(sp)
		DOS	_CLOSE
		addq.l	#2,sp

			rts



read_adr_get256_normal:
		move.w	_line(a5),d0
		mulu.w	_colum(a5),d0
		addq.l	#1,d0
		and.w	#$fffe,d0
		move.l	d0,_buff_req(a5)

		add.l	_src_fsize(a5),d0

		movem.l	d1-d2/a0-a2,-(sp)
			move.l	d0,-(sp)
			move.l	d0,_buff_size(a5)
.if 1
			move.w	#2,-(sp)
			DOS	_MALLOC2
			addq.l	#6,sp
.else
			jbsr	_malloc
			addq.l	#4,sp
.endif
		movem.l	(sp)+,d1-d2/a0-a2

		tst.l	d0
		bmi	NO_MEMORY
		move.l	d0,_buff_addr(a5)
		move.l	_buff_req(a5),d2

		move.l	d0,a1			*     a1 ～ a1+d2 ＝ 仮想GRAM
		move.l	a1,_buff_addr256(a5)
		adda.l	d2,a1			*  a1+d2 ～       ＝ ファイル読み込み

		move.l	_buff_size(a5),d1

		cmp.l	d1,d2
		bhi	error			メモリが足りん

		moveq.l	#3,d6
		move.l	_src_fsize(a5),-(sp)
		move.l	a1,-(sp)
		move.w	_file(a5),-(sp)
		DOS	_READ
stopper:
		lea	10(sp),sp
		tst.l	d0
		bmi	error
		add.l	a1,d0
		move.l	d0,_enddata_addr(a5)

		move.w	_file(a5),-(sp)
		DOS	_CLOSE
		addq.l	#2,sp
		rts








* オリジナルの gif_load.has（コメント by Mitsuky）

.comment gif_comment */////////////////////////////////////////////////////////*

	GIF decode routine for X680x0

					original program v1.06 by Herzen


int	gif_header_read()

引数
	なし

戻り値
	  0	正常終了
	負数	エラー



int	gif_decode( short *buff )

引数
	なし

戻り値
	  0	正常終了
	負数	エラー


gif_comment *//////////////////////////////////////////////////////////////////*


gif_decode:
		movem.l	d3-d7/a3-a6,-(sp)
*		movea.l	a1,a6
*		suba.l	#FOR_PI256,a6
	lea	gifwork,a6
		clr.w	error_flag(a6)
		movea.l	_buff_addr256(a5),a4
		move.l	_buff_addr(a5),interlace_trans_addr(a6)
		bsr	expand
		movem.l	(sp)+,d3-d7/a3-a6
		rts


*------------------------------------------------------------------------------*
gif_header_read:
		movem.l	d3-d7/a3-a6,-(sp)
	lea	gifwork,a6
		bsr	gif_header_read_
		movem.l	(sp)+,d3-d7/a3-a6
		rts
gif_header_read_:
*------------------------------------------------------- ［ＧＩＦ識別記号］
*							  GIF signature
*							  ６バイト固定
		bsr	getc
		cmp.b	#'G',d0
		bne	error_
		bsr	getc
		cmp.b	#'I',d0
		bne	error_
		bsr	getc
		cmp.b	#'F',d0
		bne	error_
		bsr	getc
		cmp.b	#'8',d0
		bne	error_
		bsr	getc
		cmp.b	#'7',d0		* 87
		beq	@f
		cmp.b	#'9',d0		* 89
		bne	error_
@@:
		ext.w	d0
		move.w	d0,gif_ver(a6)
		bsr	getc
		cmp.b	#'a',d0
		bne	error_

*------------------------------------------------------- ［スクリーン情報］
*							  screen descriptor
*							  ７バイト固定

		bsr	getc		* スクリーンの幅
		move.b	d0,d1
		bsr	getc
		lsl.w	#8,d0
		move.b	d1,d0
		move.w	d0,sc_xsize(a6)

		bsr	getc		* スクリーンの高さ
		move.b	d0,d1
		bsr	getc
		lsl.w	#8,d0
		move.b	d1,d0
		move.w	d0,sc_ysize(a6)

		moveq.l	#0,d3
		bsr	getc
		btst.l	#7,d0		* グローバルカラーマップの有無
		beq	error_		* 無いのは邪道だ
		move.w	d0,d1
		lsr.w	#4,d1		* 
		and.w	#%111,d1	* 色解像度（color resolution）
		addq.w	#1,d1		* の有効ビット数
		move.w	d1,sc_cr(a6)	*

		and.w	#%111,d0	* color pixel bit
		addq.w	#1,d0		*
		move.w	d0,sc_pixel(a6)	* 1ピクセルのbit数

		bsr	getc
		and.w	#$00ff,d0
*		move.w	d0,bg_col(a6)	* 背景色(not used)

		bsr	getc		* アスペクト？（詳細不明）
		and.w	#$00ff,d0
		move.w	d0,aspect(a6)

*------------------------------------------------------- ［グローバルカラーマップ］
*							  global color map
*							  ７バイト固定

		move.w	sc_pixel(a6),d0
		bsr	palet_get	* パレット取得

		move.w	#0,animated_gif(a6)

*------------------------------------------------------- ［画像情報］
*							  image descriptor
*							  10バイト固定（イメージ分離記号 含）
*							  ただし，拡張ブロック除く

to_img_sep_lop:				* イメージ分離記号まで飛ばす
		bsr	getc
		cmp.b	#IMAGE_SEPARATOR,d0
		beq	img_inf_get
		cmp.b	#TRAILER,d0
		beq	ERROR_DESTROY
		cmp.b	#EXT_INTRODUCER,d0
		bne	ERROR_EXT
		bsr	getc	* 拡張機能番号
		cmp.b	#$F9,d0
		beq	graphic_ctrl_ext
		bsr	getc	* ブロックサイズ
ext_block_lop:
		moveq.l	#0,d1
		move.b	d0,d1
		subq.w	#1,d1
@@:		bsr	getc
		dbra	d1,@b
		bsr	getc
		tst.b	d0	* ブロックに続きはあるか？
		bne	ext_block_lop
		bra	to_img_sep_lop

graphic_ctrl_ext:
		bsr	getc	* ブロックサイズ(=4)
		bsr	getc	* Packed Flags
		move.l	d0,d1

		bsr	getc	* Delay time
		bsr	getc	* 
		bsr	getc	* Transparent Color Index
		btst.l	#0,d1	* 透明フラグ
		bne	gce1
		moveq.l	#$ff,d0
gce1:
		move.w	d0,_tpcolor(a5)	* 透明色
		bsr	getc	* Block Terminator
		bra	to_img_sep_lop



img_inf_get:
		bsr	getc		* 左上Ｘ座標（画像のＸ位置）
		move.b	d0,d1
		bsr	getc
		lsl.w	#8,d0
		move.b	d1,d0
		move.w	d0,_scolum(a5)
		bsr	getc		* 左上Ｙ座標（画像のＹ位置）
		move.b	d0,d1
		bsr	getc
		lsl.w	#8,d0
		move.b	d1,d0
		move.w	d0,_sline(a5)

		bsr	getc		* 画像の幅
		move.b	d0,d1
		bsr	getc
		lsl.w	#8,d0
		move.b	d1,d0
		move.w	d0,_colum(a5)
		bsr	getc		* 画像の高さ
		move.b	d0,d1
		bsr	getc
		lsl.w	#8,d0
		move.b	d1,d0
		move.w	d0,_line(a5)

		moveq.l	#31,d6
		bsr	getc		* ローカルカラーマップ，インターレス，色解像度
		clr.b	_gif_hispeed+1(a5)
		btst.l	#6,d0		* Interlaceか？
		sne.b	_gif_hispeed+1(a5)

		btst.l	#7,d0		* ローカルカラーマップを用いるか？
		beq	@f
		and.w	#%111,d0
		addq.w	#1,d0		* 1ピクセルあたりのbit数
		bsr	palet_get	* 再度取得
@@:

*------------------------------------------------------- ［ラスターデータ］
*							  Raster data

		bsr	getc		* pixel_bit
		and.w	#$ff,d0
		move.w	d0,img_CodeSize(a6)
		move.w	#8,_color(a5)	* 256色
		cmp.w	#4,d0
		bhi	@f
		move.w	#4,_color(a5)	* 16色
@@:
		moveq.l	#1,d1
		lsl.w	d0,d1
		ext.l	d1
		subq.l	#1,d1
		move.l	d1,img_BitMask(a6)

		move.w	_scolum(a5),d0
		add.w	_colum(a5),d0
		subq.w	#1,d0
		move.w	d0,_ecolum(a5)
		move.w	_sline(a5),d0
		add.w	_line(a5),d0
		subq.w	#1,d0
		move.w	d0,_eline(a5)

		moveq.l	#0,d0
		rts

error_:
		moveq.l	#-1,d0
		rts

*------------------------------------------------------------------------------
*

palet_get:
		moveq.l	#1,d2
		lsl.w	d0,d2
		subq.w	#1,d2

		lea.l	_pal_buf(a5),a0
palet_loop:
		bsr	getc		* R
		and.w	#$00f8,d0
		rol.w	#3,d0
		move.w	d0,d1
		bsr	getc		* G
		and.w	#$00f8,d0
		ror.w	#8,d0
		or.w	d0,d1
		bsr	getc		* B
		and.w	#$00f8,d0
		ror.w	#2,d0
		or.w	d0,d1
		move.w	d1,(a0)+
@@:		dbra	d2,palet_loop
		rts


*------------------------------------------------------------------------------
*
*  デコード部メイン
*
*  GIF's LZW 展開ルーチン
*

expand:
	move.l	a4,vgram(a6)
	move.w	_colum(a5),d0
	mulu.w	_line(a5),d0
	addq.l	#1,d0
	and.w	#$fffe,d0
	move.l	a4,d1
	add.l	d1,d0
	move.l	d0,chk_addr(a6)		* 仮想GRAMの最終アドレス

	moveq.l	#0,d3
	move.l	d3,d4
	move.l	d3,d5
	move.l	d3,d7

	move.l	img_BitMask(a6),d0
	move.w	d0,BitMask(a6)
	move.w	d0,d5
	swap.w	d5

	move.w	d3,Interlace(a6)
	tst.b	_gif_hispeed+1(a5)
	sne.b	Interlace+1(a6)

	moveq.l	#1,d6			* PreLen
	swap.w	d6			* PreLen → CodeSize
	move.w	img_CodeSize(a6),d6	* d6.w = CodeSize
	moveq.l	#1,d1
	lsl.w	d6,d1
	move.w	d1,ClearCode(a6)
	addq.w	#1,d1
	move.w	d1,d2			* d2.w = EOFCode
	move.w	d1,EOFCode(a6)
	addq.w	#1,d1
	move.w	d1,d7			* d7.w = FreeCode
	move.w	d1,FirstFree(a6)

	addq.w	#1,d6
	move.w	d6,InitCodeSize(a6)
	moveq.l	#1,d1
	lsl.w	d6,d1
	swap.w	d2
	move.w	d1,d2		* MaxCode
	move.w	d1,InitMaxCode(a6)
	subq.w	#1,d1
	move.w	d1,d5		* d5.w = ReadMask
	move.w	d1,InitReadMask(a6)

	movea.l	a1,a3		* a3 = RasterData

	bsr	RasterGet	* ラスタデータを取得．

	move.l	a5,-(sp)

	lea.l	Length(a6),a5

	move.w	d7,d0
	add.w	d0,d0
	lea.l	(a5,d0.w),a1		* 書き込み専用
	move.l	a1,restore_addrs(a6)
	add.w	d0,d0
	lea.l	(a6,d0.w),a2		* 書き込み専用
	move.l	a2,restore_addrs+4(a6)

* - decode start!!

while_lop:
	readCode
	cmp.w	EOFCode(a6),d1
	beq	while_end

		cmp.w	ClearCode(a6),d1
		bne	if1_else

			moveq.l	#1,d6
			swap.w	d6
			move.w	InitCodeSize(a6),d6
			move.w	InitMaxCode(a6),d2
			move.w	InitReadMask(a6),d5
			move.w	FirstFree(a6),d7
			readCode

			movea.l	restore_addrs(a6),a1
			movea.l	restore_addrs+4(a6),a2

			move.b	d1,(a4)+

		bra	while_lop
if1_else:
		cmp.l	chk_addr(a6),a4
		bhi	while_end

		swap.w	d5			* ReadMask     → BitMask
		swap.w	d3			* readCodeSize → FinChar
		swap.w	d6			* EOFCode      → OldCode

		cmp.w	d7,d1			* CurCode >= FreeCode
		bcs	next01
			move.w	d6,d0
			movea.l	a4,a0
			suba.w	d0,a0

			addq.w	#1,d6
			cmp.w	#4095,d7
			bhi	@f
			move.w	d6,(a1)+
			move.l	a0,(a2)+
@@:			move.b	(a0)+,(a4)+
			dbra	d0,@b
			bra	if1_else_cont
next01:
		cmp.w	d5,d1
		bls	next02

			move.w	d1,d0
			add.w	d0,d0
			move.w	(a5,d0.w),d3	* 今回
			add.w	d0,d0
			movea.l	(a6,d0.w),a0

		cmp.w	#4095,d7
		bhi	@f
			move.l	a4,d0
			move.w	d6,d1		* 前回の長さ
			ext.l	d1
			sub.l	d1,d0
			move.l	d0,(a2)+
			addq.w	#1,d1
			move.w	d1,(a1)+
@@:

			move.w	d3,d6
			subq.w	#1,d3
@@:			move.b	(a0)+,(a4)+
			dbra	d3,@b

			bra	if1_else_cont

next02:
			move.w	d1,d3

		cmp.w	#4095,d7
		bhi	@f
			move.l	a4,d0
			move.w	d6,d1	* 前回の長さ
			ext.l	d1
			sub.l	d1,d0
			move.l	d0,(a2)+
			addq.w	#1,d1
			move.w	d1,(a1)+
@@:
			move.w	#1,d6
			move.b	d3,(a4)+

*			bra	if1_else_cont

if1_else_cont:
		addq.w	#1,d7		* FreeCode

		swap.w	d5		* BitMask → ReadMask
		swap.w	d3		* FinChar → readCodeSize
		swap.w	d6		* OldCode → EOFCode

		cmp.w	d2,d7		* MaxCode, FreeCode
		bcs	while_lop
			cmp.w	#12,d6
			bcc	while_lop
				addq.w	#1,d6
				add.w	d2,d2
				moveq.l	#1,d1
				lsl.w	d6,d1
				subq.w	#1,d1
				move.w	d1,d5	* ReadMask
@@:
endif1:
	bra	while_lop
while_end:
	move.l	(sp)+,a5

	tst.w	Interlace(a6)
	bne	to_interlace

	moveq.l	#0,d0
	rts

*------------------------------------------------------------------------------
*
*  インターレス変換
*

to_interlace:
	movea.l	interlace_trans_addr(a6),a4	* 変換先
	movea.l	vgram(a6),a3			* 変換元
	moveq.l	#0,d3
	moveq.l	#0,d2
	move.w	_line(a5),d2
	swap.w	d2

	moveq.l	#0,d0
	move.l	d0,d1
	move.w	_colum(a5),d0
	move.w	d0,d1
	lsl.l	#3,d0		* 8倍
	sub.l	d1,d0
	move.l	d0,colum8(a6)

	moveq.l	#0,d0
	move.w	d1,d0
	lsl.l	#2,d0		* 4倍
	sub.l	d1,d0
	move.l	d0,colum4(a6)

	move.w	_line(a5),d1
	subq.w	#1,d1
@@:
	bsr	timatima
	bsr	address_chg

	dbra	d1,@b

	moveq.l	#0,d0
	rts


address_chg:
	move.w	d2,d0
	add.w	d0,d0
	jmp	pass_tbl_(pc,d0.w)
	rts

pass_tbl_:
	bra.s	pass0_
	bra.s	pass1_
	bra.s	pass2_
	bra.s	pass3_

pass0_:
	swap.w	d2
	addq.w	#8,d3
	cmp.w	d3,d2
	bhi.b	@f
	swap.w	d2
	addq.w	#1,d2
	move.w	#4,d3
		moveq.l	#0,d0
		move.w	_colum(a5),d0
		lsl.l	#2,d0		* ４倍
		movea.l	interlace_trans_addr(a6),a4
		adda.l	d0,a4
	rts
@@:
	swap.w	d2
	adda.l	colum8(a6),a4
	rts

pass1_:
	swap.w	d2
	addq.w	#8,d3
	cmp.w	d3,d2
	bhi.b	@f
	swap.w	d2
	addq.w	#1,d2
	move.w	#2,d3
		moveq.l	#0,d0
		move.w	_colum(a5),d0
		lsl.l	#1,d0		* ２倍
		movea.l	interlace_trans_addr(a6),a4
		adda.l	d0,a4
	rts
@@:
	swap.w	d2
	adda.l	colum8(a6),a4
	rts

pass3_:
	addq.w	#2,d3
	adda.w	_colum(a5),a4
	rts

pass2_:
	swap.w	d2
	addq.w	#4,d3
	cmp.w	d3,d2
	bhi.b	@f
	swap.w	d2
	addq.w	#1,d2
	move.w	#1,d3
		movea.l	interlace_trans_addr(a6),a4
		adda.w	_colum(a5),a4
	rts
@@:
	swap.w	d2
	adda.l	colum4(a6),a4
	rts

timatima:
	move.w	_colum(a5),d0
@@:
	sub.w	#256,d0
	bmi	@f
	bsr	tima
	bra	@b
@@:
	add.w	#256,d0
	move.w	#256,d4
	sub.w	d0,d4
	add.w	d4,d4
	jmp	tima(pc,d4.w)
	nop

tima:
	.rept 256
	move.b	(a3)+,(a4)+		* 030以降の人にとってはハタ迷惑なループ展開 :-)
	.endm
	rts


*------------------------------------------------------------------------------
*
*  ラスターデータ取得
*


RasterGet:
	movea.l	a1,a0
rc_lop:
	moveq.l	#0,d0
	move.b	(a1)+,d0
	beq	RasterCopy_end
	cmp.l	_enddata_addr(a5),a1
	bhi	ERROR_DESTROY
	cmp.w	#$fe,d0
	beq	rc_t
	subq.w	#1,d0
@@:	move.b	(a1)+,(a0)+
	dbra	d0,@b
	bra	rc_lop
RasterCopy_end:
	move.l	a1,next_readdata_addr(a6)
	rts

rc_t:
	.rept 254
	move.b	(a1)+,(a0)+		* 本当はここまでする必要はないのかも :-)
	.endm
	bra	rc_lop


*------------------------------------------------------------------------------
*
*  画像展開後，さらにデータがあるかどうか？
*
*  あるかどうか知るだけで，さらに展開する気はない :-)
*

next_data_chk:
		movea.l	next_readdata_addr(a6),a1
next_data_chk_lop:
		cmp.l	_enddata_addr(a5),a1
		bhi	終端記号がない
		move.b	(a1)+,d0
		cmp.b	#TRAILER,d0
		beq	データ終わり
		cmp.b	#IMAGE_SEPARATOR,d0
		beq	next_data
		cmp.b	#EXT_INTRODUCER,d0
		bne	ERROR_EXT
@@:
		addq.l	#1,a1		* 機能番号はとばす．
		moveq.l	#0,d0
		move.b	(a1)+,d0	* データサイズ
@@:		adda.w	d0,a1
		move.b	(a1)+,d0	* データサイズ，0 だったら終了
		bne	@b
		bra	next_data_chk_lop
データ終わり
		rts

next_data:
		addq.w	#1,animated_gif(a0)	* No.?? 枚目のGIFが存在する．
		rts


*------------------------------------------------------------------------------
*
*  misc.
*

getc:
		move.w	_file(a5),-(sp)
		.dc.w	$ff1b	* __GETC
		addq.l	#2,sp
		tst.l	d0
		bmi	ERROR_LOAD
		rts

* 手抜きの極地 :-)

ERROR_EXT:
ERROR_DESTROY:
ERROR_LOAD:
終端記号がない:
	moveq.l	#-1,d0
	rts

	.bss
gifwork:
	ds.b	__gifworkend
	.end
