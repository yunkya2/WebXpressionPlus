*	DOS/IOCS コールをフックする
*	JPEGED.R 対策

	.include	IOCSCALL.MAC
	.xdef	_new_CONCTRL
	.xdef	_new_B_KEYINP,_new_B_KEYSNS,_new_BITSNS
	.xdef	_new_MS_INIT,_new_MS_CUROF,_new_MS_CURST,_new_SKEY_MOD

*	DOS コール

_new_CONCTRL:
	moveq.l	#0,d0
	rts



*	IOCS コール

_new_MS_INIT:
_new_MS_CUROF:
	rts

_new_B_KEYINP:
_new_B_KEYSNS:
_new_BITSNS:
_new_SKEY_MOD:
_new_MS_CURST:
	moveq.l	#0,d0
	rts

