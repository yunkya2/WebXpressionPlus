TARGET = WebXpressionPlus.x

all: $(TARGET)

CROSS_COMPILE ?= m68k-xelf-
CC = $(CROSS_COMPILE)gcc
LD = $(CC)
CFLAGS += -O3 -g -Iiconv_mini -DICONV_ONLY_U2S -DGIT_REPO_VERSION=\"$(GIT_REPO_VERSION)\"
LIBS += -lsocket

GIT_REPO_VERSION=$(shell git describe --tags --always)

AXTLS_DIR = axtls
AXTLS_INCLUDE = axtls-include

CFLAGS += -I$(AXTLS_INCLUDE) -I$(AXTLS_DIR)/ssl -I$(AXTLS_DIR)/crypto
$(AXTLS_DIR)/%.o: CFLAGS += -Wno-all

OBJS = WebXpression.o Html2Xpression.o Plain2Xpression.o Httpfile.o \
		DrawText.o put.o micro_console.o Image.o History.o \
		GetFile.o  Config.o Date2Date.o \
		WebCache.o \
		cut_disp.o hook.o gifl.o Compress.o MpuCache.o data.o \
		supplementary.o \
		iconv_mini/iconv_mini.o \
		$(addprefix $(AXTLS_DIR)/, \
		ssl/asn1.o \
		ssl/loader.o \
		ssl/tls1.o \
		ssl/tls1_svr.o \
		ssl/tls1_clnt.o \
		ssl/x509.o \
		crypto/aes.o \
		crypto/bigint.o \
		crypto/crypto_misc.o \
		crypto/hmac.o \
		crypto/md5.o \
		crypto/rsa.o \
		crypto/sha1.o \
		crypto/sha256.o \
		)

WebXpressionPlus.x:	$(OBJS)
	$(LD) $^ -o $@ $(LIBS)
	cp $@ ..

WebXpression.o:	WebXpression.c WebXpression.h Httpfile.h Html2Xpression.h Image.h History.h MicroConsole.h Config.h WebCache.h DrawText.h GetFile.h data.h
Html2Xpression.o:	Html2Xpression.c WebXpression.h Httpfile.h Html2Xpression.h Image.h MicroConsole.h Config.h Jis2sjis.h Entity.h WebCache.h
Plain2Xpression.o:	Plain2Xpression.c WebXpression.h Httpfile.h Html2Xpression.h MicroConsole.h Config.h
Image.o:	Image.c WebXpression.h Httpfile.h MicroConsole.h Config.h Image.h Compress.h WebCache.h gifl.h MpuCache.h
History.o:	History.c History.h Config.h

DrawText.o:	DrawText.s WebXpression.inc
GetFile.o:	GetFile.c WebXpression.h MicroConsole.h Httpfile.h GetFile.h WebCache.h
Config.o:	Config.c WebXpression.h Config.h
Date2Date.o:	Date2Date.c
WebCache.o:	WebCache.c WebXpression.h Httpfile.h WebCache.h
Httpfile.o:	Httpfile.c WebXpression.h Httpfile.h

GetFile.o WebCache.o: CFLAGS += -DWEBXPRESSION

ASFLAGS = -w

%.o: %.s *.inc *.MAC
	run68 HAS060.X $(ASFLAGS) $<
	x68k2elf.py $@ $@.elf
	mv $@.elf $@

data.o: ASFLAGS += -y

clean:
	-rm -f $(TARGET) $(OBJS) *.elf

.PHONY: all clean

