*	cut_disp.s	.CUT ファイル展開
*
*		メモリ上にべた読みした .CUT ファイルを展開します。
*		line_bytes は X68000 の TEXTVRAM に展開する場合、128 を指定して下さい。
*		　　〃　　 に 0 を指定すると特例として line_bytes を .CUT のＸサイズから
*		算出します（”詰めて”展開する）。
*
*	コール
*		move.l	#line_bytes,-(sp)	* １ラインのバイト数（ =0 だと詰めて展開）
*		pea.l	TEXTVRAM		* 展開するアドレス
*		move.l	cut_ptr,-(sp)		* ベタ読みした .CUT ファイルのアドレス
*		jsr	_cut_disp
*		lea.l	12(sp),sp
*	返り値
*		なし


	.xdef	_cut_disp


	.text
	.even

_cut_disp:		* .CUT ファイル展開

	movem.l	d1-d7/a0-a6,-(sp)

			* （link 命令を省略して引き数を受ける技 ← a6 も使うので）
	movea.l	4+7*4+7*4(sp),a0	* a0.l = ベタ読みした.CUT ファイルのアドレス
	movea.l	8+7*4+7*4(sp),a5	* a5.l = 書き込むアドレス

	move.l	12+7*4+7*4(sp),d0
	bne	not_line_bytes_0
			* １ラインのバイト数が０だった場合
	move.w	48(a0),d0	* d0.w = x_size
	subq.w	#1,d0
	lsr.w	#3,d0
	addq.w	#1,d0
not_line_bytes_0:
	move.w	d0,a6		* a6.w = １ラインのバイト数

	move.w	48(a0),d0	* d0.w = x_size
	subq.w	#1,d0
	lsr.w	#6,d0
	addq.w	#1,d0
	movea.w	d0,a1		* a1.w = フラグのバイト数

	move.w	48(a0),d6	* d6.w = x_size
	subq.w	#1,d6
	lsr.w	#3,d6		* d6.w = x_loop-1

	move.w	a6,d4
	subq.w	#1,d4
	sub.w	d6,d4		* d4.w = a5.l に加算する数

	move.w	50(a0),d7
	subq.w	#1,d7		* d7.w = y_size-1

	lea.l	52(a0),a2	* a2.l = データ本体

	move.w	a6,d0
	neg.w	d0
	movea.w	d0,a6		* a6.w = -(１ラインのバイト数)


****	****
			* １番上のラインを展開
	moveq.l	#0,d0
	move.b	(a2)+,d0
	lea.l	-1(a2,d0.w),a4	* a4.l = 次の a2.l

	cmpi.b	#1,d0
	bne	disp_1st_not_fill
			* １ラインの総バイト数が１だった場合
	move.w	d6,d5
disp_1st_fill:
	clr.b	(a5)+
	dbra	d5,disp_1st_fill
	movea.l	a2,a3
	bra	disp_1st_next

disp_1st_not_fill:
	lea.l	(a2,a1.w),a3	* a3.l = 非圧縮の部分のビットイメージデータ列

	move.w	d6,d5		* d5.w = x_loop
	moveq.l	#1,d3		* d3.w = フラグ更新カウンタ
disp_1st_x_loop:
	ror.b	d3
	bcc	disp_1st_not_next_flag
	move.b	(a2)+,d2	* d2.b = フラグ
disp_1st_not_next_flag:
	add.b	d2,d2
	bcc	disp_1st_flag_0
			* フラグが１の時
	move.b	(a3)+,(a5)+
	dbra	d5,disp_1st_x_loop
	bra	disp_1st_next
disp_1st_flag_0:		* フラグが０の時
	clr.b	(a5)+
	dbra	d5,disp_1st_x_loop

disp_1st_next:
	movea.l	a4,a2
	lea.l	(a5,d4.w),a5
	subq.w	#1,d7
	bcs	disp_exit



****	****
disp_y_loop:
	moveq.l	#0,d0
	move.b	(a2)+,d0
	lea.l	-1(a2,d0.w),a4	* a4.l = 次の a2.l

	cmpi.b	#1,d0
	bne	disp_not_fill

			* １ラインの総バイト数が１だった場合
	move.w	d6,d5
disp_fill:
	move.b	(a5,a6.w),(a5)+
	dbra	d5,disp_fill
	movea.l	a2,a3
	bra	disp_y_next


disp_not_fill:
	lea.l	(a2,a1.w),a3	* a3.l = 非圧縮の部分のビットイメージデータ列

	move.w	d6,d5
	moveq.l	#1,d3		* d3.w = フラグ更新カウンタ
disp_x_loop:
	ror.b	d3
	bcc	disp_not_next_flag
	move.b	(a2)+,d2	* d2.b = フラグ
disp_not_next_flag:
	add.b	d2,d2
	bcc	disp_flag_0
			* フラグが１の時
	move.b	(a5,a6.w),d0
	move.b	(a3)+,d1
	eor.b	d1,d0
	move.b	d0,(a5)+
	dbra	d5,disp_x_loop
	bra	disp_y_next

disp_flag_0:		* フラグが０の時
	move.b	(a5,a6.w),(a5)+
	dbra	d5,disp_x_loop
disp_y_next:
	movea.l	a4,a2
	lea.l	(a5,d4.w),a5
	dbra	d7,disp_y_loop
disp_exit:

	moveq.l	#0,d0
	movem.l	(sp)+,d1-d7/a0-a6
	rts

