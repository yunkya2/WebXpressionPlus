*	gifl.inc



* オリジナルの gif_load.h（コメント by Mitsuky）

*
*	GIF decode routine include file.
*
*				  v1.06 programmed 1996 by Herzen


IMAGE_SEPARATOR	=	$2c
EXT_INTRODUCER	=	$21
TRAILER		=	$3b

FOR_PI256	=	65536+256

	* 高速コード読み取りマクロ
readCode	.macro
	move.b	2(a3),d1		* 12
	swap.w	d1			* 4
	move.b	1(a3),-(sp)		* 16
	move.w	(sp)+,d1		* 8
	move.b	(a3),d1			* 8

	lsr.l	d3,d1			* 8+2n
	add.w	d6,d3			* 8

	move.w	d3,d0			* 4
	lsr.w	#3,d0			* 12
	adda.w	d0,a3			* 8
	and.w	#%111,d3		* 8

	and.w	d5,d1			* ReadMask,d1
		.endm


	.offset 0
__gifwork:
Address:
		.ds.l	4096
Length:
		.ds.w	4096
**
gif_ver:	.ds.w	1
sc_xsize:	.ds.w	1
sc_ysize:	.ds.w	1
sc_cr:		.ds.w	1	* 色解像度 color resolution 
sc_pixel:	.ds.w	1
bg_col:		.ds.w	1
aspect:		.ds.w	1
img_CodeSize:	.ds.w	1
img_BitMask:	.ds.w	1
animated_gif:	.ds.w	1
**
ClearCode:	.ds.w	1
EOFCode:	.ds.w	1
FirstFree:	.ds.w	1
InitCodeSize:	.ds.w	1
InitMaxCode:	.ds.w	1
InitReadMask:	.ds.w	1
**
colum8:		.ds.l	1
colum4:		.ds.l	1
**
chk_addr:	.ds.l	1
restore_addrs:	.ds.l	1*2

;	.text

;	.offset 0
vgram:		.ds.l	1
		.ds.b	4
one:		.ds.w	1
OutCode_tmp:	.ds.w	1
colum:		.ds.l	1
line:		.ds.l	1
colum2:		.ds.l	1
XC:		.ds.w	1
YC:		.ds.w	1
oldYC:		.ds.w	1
Pass:		.ds.w	1
ptr:		.ds.l	1
Interlace:	.ds.w	1
misc:		.ds.w	1
MaxCode:	.ds.w	1
OldCode:	.ds.w	1
BitMask:	.ds.w	1
FinChar:	.ds.w	1
InCode:		.ds.w	1
prelen:		.ds.w	1
interlace_trans_addr:	.ds.l	1
next_readdata_addr:	.ds.l	1
error_flag:	.ds.w	1

__gifworkend:




* オリジナルの work.h（コメント by Mitsuky）

	.offset 0

_works:
_buff_addr:
	.ds.l	1
_buff_addr256:
	.ds.l	1
_tmp_buff_addr:
	.ds.l	1
_enddata_addr:
	.ds.l	1
_gif_hispeed:
	.ds.w	1
_srcfile:
	.ds.l	1
_src_fsize:
	.ds.l	1
_buff_size:
	.ds.l	1
_buff_req:
	.ds.l	1

_color:
	.ds.w	1	* 16色表示か，256色表示か

_colum:			* X SIZE
	.ds.w	1
_line:			* Y SIZE
	.ds.w	1
_scolum:		* Start X
	.ds.w	1
_sline:			* Start Y
	.ds.w	1
_ecolum:		* End   X
	.ds.w	1
_eline:			* End   X
	.ds.w	1


_file:
	.ds.w	1

_tpcolor:		* 透明色
	.ds.w	1

_pal_buf:
	.ds.b	256*2

_tmp:
	.ds.b	1024*2
_endworks:

	.text
