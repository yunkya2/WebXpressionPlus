*	MPU キャッシュの制御
*		68000機で呼んでもＯＫ（何もせずに帰る）
*		但し _GetMpuType は１回以上呼んでおくこと

	.include	IOCSCALL.MAC

	.xdef	_GetMpuType,_GetMpuCacheMode,_FlushMpuCache,_SetMpuCacheMode


	.ifndef	_SYS_STAT
_SYS_STAT	.equ	$ac
	.endif


	.text
	.even

_GetMpuType:		* MPU の種類を得る　起動時に１回呼ぶこと
			* out : d0.l = MPU の種類
			*		68000 = 0, 68020 = 2, …, 68060 = 6
			* IOCS _SYS_STAT が使えない環境（MC68000機）を念頭において
			* $0cbc から MPU の種類を得る
	movem.l	a0-a1,-(sp)

	lea.l	$0cbc.w,a1
	IOCS	_B_BPEEK

	andi.l	#$f,d0
	move.l	d0,mpu_type

	movem.l	(sp)+,a0-a1
	rts

****************************************************************
_GetMpuCacheMode:	* 現在の MPU キャッシュのモードを取得する
			* out : d0.l =  bit0:命令キャッシュの状態
			*		bit1:データキャッシュの状態

	tst.l	mpu_type	* 68000?
	beq	GetMpuCacheNoCache

	movem.l	d1,-(sp)
	moveq.l	#1,d1
	IOCS	_SYS_STAT
	movem.l	(sp)+,d1

GetMpuCacheRts:
	rts


GetMpuCacheNoCache:
	moveq.l	#0,d0
	bra	GetMpuCacheRts

****************************************************************
_FlushMpuCache:		* MPU キャッシュをフラッシュする
	tst.l	mpu_type	* 68000?
	beq	FlushMpuCacheRts

	movem.l	d1,-(sp)
	moveq.l	#3,d1
	IOCS	_SYS_STAT
	movem.l	(sp)+,d1

FlushMpuCacheRts:
	rts


****************************************************************
_SetMpuCacheMode:	* MPU キャッシュのモードを設定する
			* in  : 8(a6).l = bit0:命令キャッシュの状態
			*		  bit1:データキャッシュの状態
			* out : d0.l =  bit0:前の命令キャッシュの状態
			*		bit1:前のデータキャッシュの状態

	link	a6,#0
	tst.l	mpu_type	* 68000?
	beq	SetMpuCacheNoCache

	movem.l	d1-d2,-(sp)
	moveq.l	#4,d1
	move.l	8(a6),d2
	IOCS	_SYS_STAT
	movem.l	(sp)+,d1-d2

SetMpuCacheRts:
	unlk	a6
	rts


SetMpuCacheNoCache:
	moveq.l	#0,d0
	bra	SetMpuCacheRts

****************************************************************

	.bss
	.even
mpu_type:	.ds.l	1
