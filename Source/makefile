#	makefile for WebXpression.x

CC	= gcc
#CFLAGS	=
CFLAGS	= -O -fomit-frame-pointer -fstrength-reduce -fforce-mem -fforce-addr -fcombine-regs
AS	= has
LD	= hlk
INC	=
LIBS	= libc.a libgnu.a libdos.a libiocs.a libnetwork.a libether.a
LZH	= WebX046

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

#vpath	%.c	./;FuncUrl/;WebCache/;GetFile/;
# .c ファイルはカレント又はここで指定したディレクトリに


WebXpression.x:	WebXpression.o Html2Xpression.o Plain2Xpression.o Httpfile.o \
		DrawText.o put.o micro_console.o Image.o History.o \
		GetFile.o  Config.o Date2Date.o \
		WebCache.o \
		cut_disp.o hook.o gifl.o compress.o MpuCache.o data.o
	$(LD) $^ -o $@ -l $(LIBS)

WebXpression.o:	WebXpression.c WebXpression.h Httpfile.h Html2Xpression.h Image.h History.h MicroConsole.h Config.h WebCache.h DrawText.h GetFile.h Data.h
Html2Xpression.o:	Html2Xpression.c WebXpression.h HttpFile.h Html2Xpression.h Image.h MicroConsole.h Config.h Jis2sjis.h Entity.h WebCache.h
Plain2Xpression.o:	Plain2Xpression.c WebXpression.h HttpFile.h Html2Xpression.h MicroConsole.h Config.h
Image.o:	Image.c WebXpression.h HttpFile.h MicroConsole.h Config.h Image.h Compress.h WebCache.h gifl.h MpuCache.h
History.o:	History.c History.h Config.h

DrawText.o:	DrawText.s WebXpression.inc
put.o:	put.s

GetFile.o:	GetFile.c WebXpression.h MicroConsole.h Httpfile.h GetFile.h WebCache.h
	$(CC) $(CFLAGS) -DWEBXPRESSION -c $<

Config.o:	Config.c WebXpression.h Config.h
Date2Date.o:	Date2Date.c

WebCache.o:	WebCache.c WebXpression.h Httpfile.h WebCache.h
	$(CC) $(CFLAGS) -DWEBXPRESSION -c $<

Httpfile.o:	Httpfile.c WebXpression.h Httpfile.h

cut_disp.o:	cut_disp.s
hook.o:	hook.s
gifl.o:	gifl.s gifl.inc
micro_console.o:	micro_console.s WebXpression.inc
compress.o:	compress.s
MpuCache.o:	MpuCache.s MpuCache.h
data.o:	data.s
	$(AS) -y $<

dist:
	LHA a -t ../$(LZH)

