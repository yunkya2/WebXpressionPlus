/* HTML ファイルを Xpression 形式に変換 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x68k/dos.h>
#include "WebXpression.h"
#include "Httpfile.h"
#include "Html2Xpression.h"
#include "Image.h"
#include "MicroConsole.h"
#include "Config.h"
#include "Jis2sjis.h"
#include "Entity.h"
#include "WebCache.h"
#include "iconv_mini.h"

/* 折り返すドット数 */
#define WRAP_DOT	((short)512)
#define LINE_Y	16

#define ALIGN_LEFT		0
#define ALIGN_RIGHT		1
#define ALIGN_CENTER	2

#define YOYUU	1024



/* 前行構造体 (Before Line Buffer) */
typedef struct _blb {
	struct _blb *before_ptr;	/* 前の構造体へのポインタ */
	struct _blb *next_ptr;	/* 次の構造体へのポインタ */
	unsigned char text[1024];	/* テキスト本体 */
	unsigned char *ptr;	/* テキストへのポインタ */
	unsigned short start_dot;	/* 左端から何ドット目から表示開始するか */
	unsigned short font_size;
	unsigned short width;
} BLB;


typedef struct _work {
	unsigned char *_t1;
	unsigned char *_t2;

	unsigned char *t1e, *t2e;	/* t1,t2 の末尾 */
	unsigned char *t1_old;	/* 「処理した文字を読まなかった事にする」用 */
	unsigned char *t2t;	/* その行の先頭の t2 */
	unsigned int t2_size;	/* t2 のサイズ */

	unsigned short width;	/* ドット数 */
	unsigned char space_flag;	/* 直前が半角スペースだったか */

	unsigned char align;	/* 行揃えモード */
	unsigned char tag_center;
	unsigned char tag_href;
	unsigned char tag_pre;
	unsigned char tag_ol;
	unsigned char tag_ul;
#if	0
	unsigned char tag_bold = 0;
	unsigned char tag_italic = 0;
	unsigned char tag_underline = 0;
	unsigned char font_type = 0;
#endif

	char reset_align;
	unsigned char tag_p_align;
	unsigned short tag_list_no;
	unsigned short font_size;	/* 半角文字の大きさ */
	unsigned char *tag_head;	/* <head>の次のアドレス */
	unsigned char *tag_title;	/* <title>の次のアドレス */

	char nl_flag;		/* 改行するなら = !0 */
	char not_read_flag;	/* 文字を読まなかった事にするなら= !0 */
	char reset_line_ptr;	/* =!0:行ポインタを初期化する */

	unsigned char pass;	/* =0:１パス目 =1:２パス目 */

	unsigned char font_size_stack[256];
	unsigned char font_size_stackptr;

	char *link_table_buffer_ptr;

	unsigned short ffifo_ptr;
	char ffifo[256];

#define WORK_STR_SIZE	256
    /* 汎用文字列バッファ */
    /* ２バイトコードを考慮して２バイト余分に取る */
	char str[WORK_STR_SIZE + 2];

#define WORK_ATTR_STR_SIZE	256
	signed short tag_no;
	signed short attr_no;
	char attr_str[WORK_ATTR_STR_SIZE + 1];

	HTTPFILE *httpfile;
	XPTEXT *xptext;
} WORK;

#include "Tag.h"

enum {
	QUOTE_NON = 0, QUOTE_SINGLE, QUOTE_DOUBLE
};

/* 日本語文字コード */
enum {
	K_JIS = 0, K_SJIS, K_EUC, K_UTF8,
};

static BLB *blb_top, *blb_end;
static char tag_href_str[256];

static int entity_len[sizeof (entity_str) / sizeof (char *) - 1];



void InitHtml2Xpression (void)
{
	short h;
	for (h = 0; h < (sizeof (entity_str) / sizeof (char *) - 1); h++)
		entity_len[h] = strlen (entity_str[h]);
}



/* 文字コードを判定する */
int GetCharset (HTTPFILE * httpfile)
{
	register unsigned char *t1 asm ("a4");	/* 現在処理している文字（転送元） */
	unsigned char *t1e;	/* t1 の末尾 */
	register unsigned char c asm ("d7");	/* 処理する文字 */
	int is_jis = 0;
	int is_sjis = 0;
	int is_euc = 0;
	int is_utf8 = 0;
#define WINNERS_POINT	8	/* is_xxx がこの得点を越えたら打ち切り */
	unsigned char temp_charset = K_SJIS;	/* 文字コード */
	unsigned char jis_kanji = 0;	/* JIS 用 ASCII(=0) or KANJI(=!0) */
	int ret;		/* 返り値 */

	t1 = httpfile->content;	/* html ファイル本体へのポインタ */
	t1e = t1 + httpfile->content_length;	/* t1 が t1e に達したら終了 */

    /* 桁ループ */
	do {
		c = *t1++;

	    /* 0x1b を見つけたら JIS と推測 */
		if (c == 0x1b) {	/* ESC */
			unsigned char c1 = *(t1 + 1);

			if (*t1 == '(') {
				if ((c1 == 'B') || (c1 == 'J')) {	/* 漢字 OUT */
					jis_kanji = 0;
					t1 += 2;
					temp_charset = K_JIS;
					if (++is_jis > WINNERS_POINT)
						break;	/* 判定終了 */
				}
			}
			if (*t1 == '$') {
				if ((c1 == '@') || (c1 == 'B')) {	/* 漢字 IN */
					jis_kanji = !0;
					t1 += 2;
					temp_charset = K_JIS;
					if (++is_jis > WINNERS_POINT)
						break;	/* 判定終了 */
				}
			}
			continue;	/* 次の文字へ */
		}
		/* UTF-8 エンコードされている文字を見つけたら UTF-8 と推測 */
		if (c >= 0xc2 && c <= 0xdf) {
			if (t1[0] >= 0x80 && t1[0] <= 0xbf) {	// 2バイト文字
				temp_charset = K_UTF8;
				if (++is_utf8 > WINNERS_POINT)
					break;	/* 判定終了 */
				t1++;
				continue;
			}
		} else if (c >= 0xe0 && c <= 0xef) {		// 3バイト文字
			if ((t1[0] >= 0x80 && t1[0] <= 0xbf) &&
				(t1[1] >= 0x80 && t1[1] <= 0xbf)) {
				temp_charset = K_UTF8;
				if (++is_utf8 > WINNERS_POINT)
					break;	/* 判定終了 */
				t1 += 2;
				continue;
			}
		}
	    /* 0x81~0x9f を見つけたら SJIS と推測 */
		if ((c >= 0x81) && (c <= 0x9f)) {
			temp_charset = K_SJIS;
			if (++is_sjis > WINNERS_POINT)
				break;	/* 判定終了 */
		}
	    /* 0xa1~0xcf を見つけたら EUC と推測 */
		if ((c >= 0xa1) && (c <= 0xfe)) {
			unsigned char c1 = *(t1 + 1);
			if ((c1 >= 0xa1) && (c1 <= 0xfe)) {
				temp_charset = K_EUC;
				if (++is_euc > WINNERS_POINT)
					break;	/* 判定終了 */
			}
		}
		if ((c == 0x0d) || (c == 0x0a)) {
			jis_kanji = 0;
			continue;	/* 次の文字へ */
		}
		switch (temp_charset) {
		case K_UTF8:
			break;		/* UTF-8 はスキップ処理済み */
		case K_SJIS:
			if ((c >= 0x80) && ((c < 0xa0) || (c > 0xdf)))
				t1++;	/* 漢字の２バイト目をスキップ */
			break;
		case K_EUC:
			if (c >= 0x80)
				t1++;	/* 漢字の２バイト目をスキップ */
			break;
		case K_JIS:
			if (jis_kanji)
				t1++;	/* 漢字の２バイト目をスキップ */
			break;
		}
	} while (t1 < t1e);

    /* is_sjis, is_jis, is_euc の中で一番大きなものを文字コードとする */
	if (is_utf8 > is_sjis && is_utf8 > is_jis && is_utf8 > is_euc) {
		ret = K_UTF8;
	} else if (is_sjis > is_jis) {
	    /* SJIS か EUC */
		if (is_sjis > is_euc)
			ret = K_SJIS;
		else
			ret = K_EUC;
	} else {
	    /* JIS か EUC */
		if (is_jis > is_euc)
			ret = K_JIS;
		else
			ret = K_EUC;
	}
#if	0
	printf ("文字コード:%d,%d,%d\n", is_sjis, is_jis, is_euc);
#endif
	return (ret);
}



/* httpfile->content を SJIS に変換 */
/* httpfile->content_length も書き替える */
int Html2Sjis (HTTPFILE * httpfile)
{
	unsigned char *new_html;

	register unsigned char *t1 asm ("a4");	/* 現在処理している文字（転送元） */
	register unsigned char *t2 asm ("a5");	/* 〃              （転送先） */
	register unsigned char c asm ("d7");	/* 処理する文字 */
	unsigned char *t1e, *t2e;	/* t1,t2 の末尾 */
	unsigned int t2_size;	/* t2 のサイズ */

	unsigned char temp_charset;	/* 文字コード */


	temp_charset = GetCharset (httpfile);
	if (temp_charset == K_SJIS) {
		McDbPuts ("SJIS と判定\n");
		return (0);
	}
#define SJIS_TEXT_BUFFER_YOYUU 32768
	t2_size = httpfile->content_length + SJIS_TEXT_BUFFER_YOYUU;

    /* とりあえず固定サイズで確保 */
	new_html = malloc (t2_size);
	if ((int) new_html == 0) {
		McPuts ("※ メモリが足りません（SJIS 変換バッファ用のメモリが確保できません）\n");
		return (-1);
	}
	t1 = httpfile->content;	/* html ファイル本体へのポインタ */
	t1e = t1 + httpfile->content_length;	/* t1 が t1e に達したら終了 */
	t2 = new_html;		/* SJIS テキストバッファへのポインタ */
	t2e = t2 + t2_size - YOYUU;
    /* t2 が t2e に達したら終了（バッファ不足） */

	if (temp_charset == K_UTF8) {
		/* UTF-8 の時 */
		McDbPuts ("UTF-8 と判定\n");
		/* バッファ内をUTF-8文字列と見なしてSJISに変換する */
		unsigned char *inbuf = t1;
		size_t inbytesleft = t1e - t1;
		unsigned char *outbuf = t2;
		size_t outbytesleft = t2e - t2;
		do {
			int res = iconv_u2s((char **)&inbuf, &inbytesleft, (char **)&outbuf, &outbytesleft);
			if (res < 0) {		/* 変換エラーの文字はスキップする */
				inbuf++;
				inbytesleft--;
			}
		} while (inbuf < t1e && outbuf < t2e);
		t2 = outbuf;
	} else if (temp_charset == K_JIS) {
	    /* JIS の時 */
		unsigned char jis_kanji = 0;	/* JIS 用 ASCII(=0) or KANJI(=!0) */
		McDbPuts ("JIS と判定\n");
	    /* 桁ループ */
		do {
			c = *t1++;

		    /* 漢字 IN/OUT フラグの処理 */
			if (c == 0x1b) {	/* ESC */
				unsigned char c1 = *(t1 + 1);
				switch (*t1) {
				case '(':
					if ((c1 == 'B') || (c1 == 'J')) {	/* 漢字 OUT */
						jis_kanji = 0;
						t1 += 2;
					}
					break;
				case '$':
					if ((c1 == '@') || (c1 == 'B')) {	/* 漢字 IN */
						jis_kanji = !0;
						t1 += 2;
					}
					break;
				default:
					break;
				}
				continue;	/* 次の文字へ */
			}
			if (!jis_kanji) {
				*t2++ = c;
			} else {
			    /* 漢字の場合 */
				if ((c == 0x0d) || (c == 0x0a)) {
					jis_kanji = 0;
					continue;
				}
				Jis2sjis (c, *t1++, t2);
			}
		} while ((t1 < t1e) && (t2 < t2e));
	} else {
	    /* EUC の時 */
		McDbPuts ("EUC と判定\n");
	    /* 桁ループ */
		do {
			c = *t1++;

			if (c < 0x80) {
				*t2++ = c;
			} else {
			    /* 漢字の場合 */
				Jis2sjis (((c & 0x7f)), (*t1++ & 0x7f), t2);
			}
		} while ((t1 < t1e) && (t2 < t2e));
	}

	free (httpfile->content);		/* 変換前の HTML は捨てる */
	httpfile->content_length = (int) (t2 - new_html);
    /* 余分に確保したメモリブロックを切り捨てる */
	new_html = realloc (new_html, httpfile->content_length);
	httpfile->content = new_html;

#if	0
	{
		FILE *fp;
		if ((fp = fopen ("TEMP.DOC", "wb")) != NULL) {
			fwrite (httpfile->content, sizeof (char), httpfile->content_length, fp);
			fclose (fp);
		}
	}
#endif

	return (0);
}



/* その文字が HTML 上での「スペース」か */
/* （タブや改行もスペースとして扱う） */
static inline char IsHtmlSpace (unsigned char c)
{
	if ((c == ' ') || (c == 0x09) || (c == 0x0a) || (c == 0x0d))
		return (!0);
	else
		return (0);
}



/* スペース・タブ・改行を読み飛ばす */
/* 返り値 =0:正常終了 / =!0:異常終了 */
/* w->_t1 : 次の文字 */
static char SkipSpace (WORK * w)
{
	unsigned char c;
	unsigned char *t1e = w->t1e;

	do {
		c = *w->_t1++;
		if (w->_t1 > t1e) {
			McDbPuts ("SkipSpace():テキストの末尾に達しました\n");
			return (!0);	/* テキストの末尾に達した */
		}
	} while (IsHtmlSpace (c));

	w->_t1--;
	return (0);
}



/* '>' の次まで読み飛ばす */
/* 返り値 =0:正常終了 / =!0:異常終了 */
/* w->_t1 : 次の文字 */
static char SkipLt (WORK * w)
{
	unsigned char c;
	unsigned char *t1, *t1e = w->t1e;

	t1 = w->_t1;
	while (1) {
		c = *t1++;
		if (t1 > t1e) {
			w->_t1 = t1;
			return (!0);	/* テキストの末尾に達した */
		}
		if (c == '>')
			break;
	}

	w->_t1 = t1;
	return (0);
}



/*
   w->t1 から t2 へ文字列をコピーする・文字数制限付き
   クオーティングされていてもＯＫ
   返り値 =0:正常終了 / =!0:異常終了
 */
static char StrncpyQuoted (char *d, WORK * w, int n)
{
	unsigned char c;
	unsigned char *t1, *t2 = d, *t2e = d + n;
	char quote_flag = QUOTE_NON;

	t1 = w->_t1;

	c = *t1;
	switch (c) {
	case '\'':
		quote_flag = QUOTE_SINGLE;
		t1++;
		break;
	case '"':
		quote_flag = QUOTE_DOUBLE;
		t1++;
		break;
	default:
		quote_flag = QUOTE_NON;
		break;
	}

	while (1) {
		c = *t1++;
		if (c < 0x80) {
		    /* １バイト文字の場合 */
			switch (quote_flag) {
			case QUOTE_NON:
				if (IsHtmlSpace (c)) {
					*t2 = '\0';
					w->_t1 = t1;
					if (SkipSpace (w))
						return (-1);
					else
						return (0);
				}
				if (c == '>') {
					*t2 = '\0';
					w->_t1 = t1 - 1;	/* '>' の上を指すように */
					return (0);
				}
				break;
			case QUOTE_SINGLE:
				if (c == '\'') {
					*t2 = '\0';
					w->_t1 = t1;
					if (SkipSpace (w))
						return (-1);
					else
						return (0);
				}
				break;
			case QUOTE_DOUBLE:
				if (c == '"') {
					*t2 = '\0';
					w->_t1 = t1;
					if (SkipSpace (w))
						return (-1);
					else
						return (0);
				}
				break;
			}
			*t2++ = c;
		} else {
			if ((c >= 0xa0) && (c <= 0xdf)) {
			    /* 半角カナ */
				*t2++ = c;
			} else {
			    /* 漢字 */
				*t2++ = c;
				*t2++ = *t1++;
			}
		}

	    /* 文字列バッファがあふれたか？（クオーティングが閉じてないとか） */
		if (t2 >= t2e) {
		    /* テキストを読み飛ばす処理 */
		    /* テキストの末尾に達しない限り正常終了として扱う */
		    /* （当然文字列は途中までしかコピーされていないけど） */
			while (1) {
				unsigned char *t1e = w->t1e;
				c = *t1++;

				if (t1 > t1e) {
					w->_t1 = t1;
					return (!0);	/* テキストの末尾に達した */
				}
				switch (quote_flag) {
				case QUOTE_NON:
					if (IsHtmlSpace (c)) {
						w->_t1 = t1;
						if (SkipSpace (w))
							return (-1);
						else
							return (0);
					}
					if (c == '>') {
						w->_t1 = t1 - 1;
						if (SkipSpace (w))
							return (-1);
						else
							return (0);
					}
					break;
				case QUOTE_SINGLE:
					if (c == '\'') {
						w->_t1 = t1;
						if (SkipSpace (w))
							return (-1);
						else
							return (0);
					}
					break;
				case QUOTE_DOUBLE:
					if (c == '"') {
						w->_t1 = t1;
						if (SkipSpace (w))
							return (-1);
						else
							return (0);
					}
					break;
				}
			}
		}
	}
	return (0);		/* ここには来ないハズ */
}



/*
   文字列を読み飛ばす
   クオーティングされていてもＯＫ
   返り値 =0:正常終了 / =!0:異常終了
 */
static char SkipQuoted (WORK * w, int n)
{
	unsigned char c;
	unsigned char *t1;
	char quote_flag = QUOTE_NON;
	int counter = 0;

	t1 = w->_t1;

	c = *t1;
	switch (c) {
	case '\'':
		quote_flag = QUOTE_SINGLE;
		t1++;
		break;
	case '"':
		quote_flag = QUOTE_DOUBLE;
		t1++;
		break;
	default:
		quote_flag = QUOTE_NON;
		break;
	}

	while (1) {
		c = *t1++;
		if (c < 0x80) {
		    /* １バイト文字の場合 */
			switch (quote_flag) {
			case QUOTE_NON:
				if (IsHtmlSpace (c)) {
					w->_t1 = t1;
					if (SkipSpace (w))
						return (-1);
					else
						return (0);
				}
				if (c == '>') {
					w->_t1 = t1 - 1;	/* '>' の上を指すように */
					return (0);
				}
				break;
			case QUOTE_SINGLE:
				if (c == '\'') {
					w->_t1 = t1;
					if (SkipSpace (w))
						return (-1);
					else
						return (0);
				}
				break;
			case QUOTE_DOUBLE:
				if (c == '"') {
					w->_t1 = t1;
					if (SkipSpace (w))
						return (-1);
					else
						return (0);
				}
				break;
			}
		} else {
			if ((c >= 0xa0) && (c <= 0xdf)) {
			    /* 半角カナ */
			} else {
			    /* 漢字 */
				t1++;
			}
		}
	    /* 文字列バッファがあふれたか？（クオーティングが閉じてないとか） */
		counter++;
		if (counter >= n) {
		    /* テキストを読み飛ばす処理 */
		    /* テキストの末尾に達しない限り正常終了として扱う */
		    /* （当然文字列は途中までしかコピーされていないけど） */
			while (1) {
				unsigned char *t1e = w->t1e;
				c = *t1++;

				if (t1 > t1e) {
					w->_t1 = t1;
					return (!0);	/* テキストの末尾に達した */
				}
				switch (quote_flag) {
				case QUOTE_NON:
					if (IsHtmlSpace (c)) {
						w->_t1 = t1;
						if (SkipSpace (w))
							return (-1);
						else
							return (0);
					}
					if (c == '>') {
						w->_t1 = t1 - 1;
						if (SkipSpace (w))
							return (-1);
						else
							return (0);
					}
					break;
				case QUOTE_SINGLE:
					if (c == '\'') {
						w->_t1 = t1;
						if (SkipSpace (w))
							return (-1);
						else
							return (0);
					}
					break;
				case QUOTE_DOUBLE:
					if (c == '"') {
						w->_t1 = t1;
						if (SkipSpace (w))
							return (-1);
						else
							return (0);
					}
					break;
				}
			}
		}
	}
	return (!0);		/* ここには来ないハズ */
}



/*
   t1 の指すアトリビュートを読み飛ばす
   w->_t1 はタグの先頭を指していること（スペース不可）
   <A HREF="foo" NAME="aaa">
   _　La         Lb   w->_t1 が a の時、w->_t1 = b となる
   返り値 =0:正常終了 / =!0:異常終了
   w->_t1 は '>'、もしくは次のアトリビュートの上を指している（スペース読み飛ばし済み）
 */
static unsigned char SkipAttrNo (WORK * w)
{
	unsigned char *t1, *t1e = w->t1e;

	t1 = w->_t1;
	while (1) {
		unsigned char c1;

		c1 = *t1++;
		if (t1 > t1e) {
			w->_t1 = t1;
			return (!0);	/* テキストの末尾に達した */
		}
	    /* 比較対象の末尾に達した */
		if (IsHtmlSpace (c1)) {
		    /* 値なしの場合（<BR ALL> 等） */
			w->_t1 = t1;
			SkipSpace (w);
			return (0);
		}
		if (c1 == '>') {
		    /* 値なしの場合（<BR> 等） */
			w->_t1 = t1 - 1;
			return (0);
		}
		if (c1 == '=') {
		    /* 値ありの場合（<A HREF="foo"> 等） */
			w->_t1 = t1;
			if (SkipQuoted (w, 256))
				return (-1);
			else
				return (0);
		}
	}

	return (!0);		/* ここには来ないハズ */
}



/*
   t1 の指すアトリビュートを w->attr_no に返す
   アトリビュートに値があれば w->attr_str に設定する
   t1 の指す文字列 = 大文字／小文字可 ／ str の指す文字列 = 大文字のみ
   w->_t1 はタグの先頭を指していること（スペース不可）
   <A HREF="foo" NAME="aaa">
   _　La         Lb   w->_t1 が a の時、w->attr = ATTR_A_HREF、
   w->attr_str = foo（クオーテーションは削除されてコピー）、w->_t1 = b となる
   返り値 =0:正常終了 / =!0:異常終了
   w->_t1 は '>'、もしくは次のアトリビュートの上を指している（スペース読み飛ばし済み）
 */
static unsigned char GetAttrNo (WORK * w, unsigned char **str)
{
	unsigned char **s = str;	/* 比較対象の文字列のポインタテーブル */
	signed short no = 0;

	w->attr_no = -1;
	w->attr_str[0] = '\0';

	do {
		unsigned char *t1, *t1e = w->t1e, *t2 = *s;

		t1 = w->_t1;
		while (1) {
			unsigned char c1, c2;

			c1 = *t1++;
			if (t1 > t1e) {
				w->_t1 = t1;
				return (!0);	/* テキストの末尾に達した */
			}
			c2 = *t2++;
			if (c2) {
				if ((c1 != c2) && ((c1 & 0xdf) != c2))
					break;	/* 違ったら次の比較対象へ */
			} else {
			    /* 比較対象の末尾に達した */
				if (IsHtmlSpace (c1)) {
				    /* 値なしの場合（<BR ALL> 等） */
					w->attr_no = no;
					w->_t1 = t1;
					SkipSpace (w);
					return (0);
				}
				if (c1 == '>') {
				    /* 値なしの場合（<BR> 等） */
					w->attr_no = no;
					w->_t1 = t1 - 1;
					return (0);
				}
				if (c1 == '=') {
				    /* 値ありの場合（<A HREF="foo"> 等） */
					w->attr_no = no;
					w->_t1 = t1;
					if (StrncpyQuoted (w->attr_str, w, 256))
						return (-1);
					else
						return (0);
				}
				break;
			}
		}
		no++;
	} while (*(++s) != NULL);

	SkipAttrNo (w);

	w->attr_no = -1;
	return (0);
}



/*
   t1 の指すタグを w->tag_no に返す
   t1 の指す文字列 = 大文字／小文字可 ／ str の指す文字列 = 大文字のみ
   w->_t1 はタグの先頭を指していること（スペース不可）
   返り値 =0:正常終了 / =!0:異常終了
   w->_t1 は '>'、もしくはアトリビュートの上を指している（スペース読み飛ばし済み）
 */
static unsigned char GetTagNo (WORK * w)
{
	unsigned char **s = tag_str;	/* 比較対象の文字列のポインタテーブル */
	signed short no = 0;

	w->tag_no = -1;

	do {
		unsigned char *t1, *t1e = w->t1e, *t2 = *s;

		t1 = w->_t1;
		while (1) {
			unsigned char c1, c2;

			c1 = *t1++;
			if (t1 > t1e) {
				w->_t1 = t1;
				return (!0);	/* テキストの末尾に達した */
			}
			c2 = *t2++;
			if (c2) {
				if ((c1 != c2) && ((c1 & 0xdf) != c2))
					break;	/* 違ったら次の比較対象へ */
			} else {
			    /* 比較対象の末尾に達した */
				if (IsHtmlSpace (c1)) {
					w->tag_no = no;
					w->_t1 = t1;
					if (SkipSpace (w))
						return (!0);	/* テキストの末尾に達した */
					else
						return (0);
				}
				if (c1 == '>') {
					w->tag_no = no;
					w->_t1 = t1 - 1;	/* t1 が '>' の上を指すように */
					return (0);
				}
				break;
			}
		}
		no++;
	} while (*(++s) != NULL);

    /* 未対応タグの時の読み飛ばし処理 */
	{
		unsigned char c1;

		while (1) {
			c1 = *w->_t1++;
			if (w->_t1 > w->t1e) {
				return (!0);	/* テキストの末尾に達した */
			}
			if (c1 < 0x80) {
			    /* １バイト文字の場合 */
				if (c1 == '>') {
					w->_t1--;	/* t1 が '>' の上を指すように */
					break;
				}
			} else {
				if ((c1 >= 0xa0) && (c1 <= 0xdf)) {
				    /* 半角カナ */
				} else {
				    /* 漢字 */
					w->_t1++;
				}
			}
		}
	}

	return (0);
}



/* "#ffffff" のような色指定を 16bit 色に */
static unsigned short Str2Color (char *p)
{
	unsigned short col = 0;
	short h;
	unsigned char shift_rgb[3] =
	{
		6, 11, 1
	};

	p++;
	for (h = 0; h < 3; h++) {
		unsigned char c;
		unsigned short t1;

		c = *p++;
		if ((c >= '0') && (c <= '9'))
			t1 = (unsigned short) (c - '0');
		else
			t1 = (unsigned short) (((c & 0xdf) - 'A' + 10) & 0x0f);

		c = *p++;
		if (((c >= '8') && (c <= '9')) || ((c & 0xdf) >= 'A') && ((c & 0xdf) >= 'F'))
			col |= (((t1 * 2 + 1)) << shift_rgb[h]);
		else
			col |= (((t1 * 2)) << shift_rgb[h]);
	}
	if (col == 0)
		col = 1;	/* 0x0000 だとまずいので */

	return (col);
}



/* 末尾にノード(BLB)を１つ追加する */
static BLB *InsertBLB (signed short nest,
	    unsigned char c, unsigned char c1, unsigned short font_size, unsigned short width,
		       short image_table_max, int hh, int y_offset)
{
	BLB *t_ptr = blb_top;
	signed short h = nest;

    /* リストを検索してあればそのポインタ、なければ作成してそのポインタを返す */
	while (--h != 0)
		t_ptr = t_ptr->next_ptr;

	if (t_ptr == NULL) {
	    /* なかったので作成 */
		t_ptr = malloc (sizeof (BLB));
		if (t_ptr == NULL) {
			McPuts ("InsertBLB() : メモリが足りません\n");
			return (NULL);
		} else {
			if (blb_top == NULL) {
			    /* ノード０個の所に追加 */
				blb_top = t_ptr;
				blb_end = t_ptr;
				t_ptr->before_ptr = NULL;
				t_ptr->next_ptr = NULL;
			} else {
			    /* 末尾にノードを追加 */
				(blb_end)->next_ptr = t_ptr;
				t_ptr->before_ptr = blb_end;
				t_ptr->next_ptr = NULL;
				blb_end = t_ptr;
			}
			*(t_ptr->text) = '\0';
			t_ptr->ptr = t_ptr->text;
			t_ptr->start_dot = 0;
			t_ptr->font_size = 0;
			t_ptr->width = 0;
		}
	}
	if ((c) || (c1)) {
	    /* 大きい文字の前行処理 */
		if (!c1) {
		    /* １バイト文字 */
			if ((t_ptr->font_size == font_size) && (t_ptr->width + t_ptr->font_size == width)) {
			    /* 前回と同じフォントサイズの連続する文字なら */
				*t_ptr->ptr++ = c;
				t_ptr->width = width;
			} else {
				t_ptr->font_size = font_size;
				t_ptr->width = width;
				t_ptr->ptr += sprintf (t_ptr->ptr, "`D%dS%d`", width, font_size);
				*t_ptr->ptr++ = c;
			}
		} else {
		    /* ２バイト文字 */
			if ((t_ptr->font_size == font_size) && (t_ptr->width + t_ptr->font_size * 2 == width)) {
			    /* 前回と同じフォントサイズの連続する文字なら */
				*t_ptr->ptr++ = c;
				*t_ptr->ptr++ = c1;
				t_ptr->width = width;
			} else {
				t_ptr->font_size = font_size;
				t_ptr->width = width;
				t_ptr->ptr += sprintf (t_ptr->ptr, "`D%dS%d`", width, font_size);
				*t_ptr->ptr++ = c;
				*t_ptr->ptr++ = c1;
			}
		}
	} else {
	    /* 大きいイメージの前行処理 */
		t_ptr->width = width;
		if (*tag_href_str) {	/* リンクがあるか？ */
			t_ptr->ptr += sprintf (t_ptr->ptr, "`D%d`%s`G%d,%d,%x``lu`",
					  width, tag_href_str, image_table_max, hh, y_offset);
		} else {
			t_ptr->ptr += sprintf (t_ptr->ptr, "`D%dG%d,%d,%xU`",
					       width, image_table_max, hh, y_offset);
		}
	}
	*t_ptr->ptr = '\0';

	return (t_ptr);
}



/* 末尾のノード(BLB)を削除する（実際には削除せず、切り出して返り値とする） */
static BLB *DeleteBLB (void)
{
	BLB *t_ptr = blb_end;

	if (blb_top == NULL)
		return (NULL);

	if (blb_end->before_ptr == NULL) {
	    /* １つしかないノードを削除する */
		blb_top = NULL;
		blb_end = NULL;
	} else {
		(blb_end->before_ptr)->next_ptr = NULL;
		blb_end = blb_end->before_ptr;
	}
	return (t_ptr);
}



static void TagBr (WORK * w)
{
	w->nl_flag = !0;	/* 桁ループを抜ける */
}

static void TagA (WORK * w)
{
	while (*w->_t1 != '>') {
		GetAttrNo (w, attr_a_str);

		switch (w->attr_no) {
		case ATTR_A_HREF:
			if (w->pass == 0) {
			    /* 初回の解析なら */
				char temp_fname[256];

				((w->xptext->link_table)[w->xptext->link_table_max]).url = w->link_table_buffer_ptr;
				strcpy (w->link_table_buffer_ptr, w->attr_str);

			    /* "#foo"（ファイル名なし）か？ */
				if (w->attr_str[0] == '#') {
				    /* 必ずキャッシュに存在する（だって同一ファイル中） */
					(w->xptext->link_table)[w->xptext->link_table_max].in_cache = !0;
				} else {
					HTTPFILE _t_httpfile, *t_httpfile = &_t_httpfile;

					InitHttpfile (t_httpfile);
					CatHttpfile (t_httpfile, w->httpfile, w->attr_str);

					if (WCExist (t_httpfile, temp_fname) > WC_NON)
						(w->xptext->link_table)[w->xptext->link_table_max].in_cache = !0;
					else
						(w->xptext->link_table)[w->xptext->link_table_max].in_cache = 0;
#if 0
					McDbPrintf ("url = %s", t_httpfile->url);
					if ((w->xptext->link_table)[w->xptext->link_table_max].in_cache == 0)
						McDbPrintf ("ない\n");
					else
						McDbPrintf ("存在\n");
#endif
				}
				while (*w->link_table_buffer_ptr++);
			}
			sprintf (tag_href_str, "`L%dU`", w->xptext->link_table_max);
			w->_t2 += sprintf (w->_t2, "%s", tag_href_str);
			w->tag_href = !0;
			w->xptext->link_table_max++;
			break;

		case ATTR_A_NAME:
			strcpy (((w->xptext->anchor_table)[w->xptext->anchor_table_max]).anchor, w->attr_str);
			((w->xptext->anchor_table)[w->xptext->anchor_table_max]).line = w->xptext->line;
			w->xptext->anchor_table_max++;
			break;

		default:
			break;
		}
	}
}

static void Tag_A (WORK * w)
{
	strcpy (w->_t2, "`lu`");
	w->_t2 += 4;
	w->tag_href = 0;
	*tag_href_str = '\0';
}

static void TagImg (WORK * w)
{
	HTTPFILE *httpfile = w->httpfile;
	XPTEXT *xptext = w->xptext;

	unsigned short temp_x = 0, temp_y = 0;
	char insert_now = 0;	/* 今回ノードを追加したか？ */
	IMAGE_LIST *t_ptr;

	if (w->pass == 0)
		(xptext->image_table)[xptext->image_table_max].image_list = NULL;

	while (*w->_t1 != '>') {
		char img_src_flag = 0;

		GetAttrNo (w, attr_img_str);
//McDbPrintf("***attr_no=%d, attr_str=%s\n",w->attr_no,w->attr_str);

		switch (w->attr_no) {
		case ATTR_IMG_SRC:
			img_src_flag = !0;

			if (w->pass == 0) {
				char *p = strchr(w->attr_str, '&');
				if (p) {
					/* & があれば終端にする */
					*p = '\0';
				}

			    /* 初回の解析なら */
				char temp_fname[256];
				IMAGE_LIST *t_ptr;
				HTTPFILE _t_httpfile, *t_httpfile = &_t_httpfile;

				InitHttpfile (t_httpfile);
				CatHttpfile (t_httpfile, httpfile, w->attr_str);

			    /* イメージリストに存在するか */
			    /* debug まだ query を削除して検索しない */
				if ((t_ptr = SearchImageNode (t_httpfile)) == NULL) {
				    /* 存在しない場合 */
					insert_now = !0;
				    /* debug エラーチェックまだ */
					if ((t_ptr = InsertImageNode (t_httpfile)) == NULL) {
						McDbPrintf ("IMG_SRC:イメージノードが追加できません\n");
					}
				}
				(xptext->image_table)[xptext->image_table_max].image_list = t_ptr;
				if (WCExist (t_httpfile, temp_fname) > WC_NON)
					(xptext->image_table)[xptext->image_table_max].in_cache = !0;
				else
					(xptext->image_table)[xptext->image_table_max].in_cache = 0;
			}
			break;

		case ATTR_IMG_ALT:
			break;

		case ATTR_IMG_HEIGHT:
		    /* 初回の解析なら */
			if (w->pass == 0) {
				if (image_compress)
					temp_y = (atoi (w->attr_str) + 1) / 2;
				else
					temp_y = atoi (w->attr_str);
			}
			break;

		case ATTR_IMG_WIDTH:
			if (w->pass == 0) {
			    /* 初回の解析なら */
				if (image_compress)
					temp_x = (atoi (w->attr_str) + 1) / 2;
				else
					temp_x = atoi (w->attr_str);
			}
			break;

		default:
			break;
		}
	}
	t_ptr = (xptext->image_table)[xptext->image_table_max].image_list;
#if	1
	if (t_ptr == NULL) {
	    /* アトリビュート SRC が無い */
		McPuts ("アトリビュート SRC の無い <IMG> タグです\n");
		McDbPuts ("t_ptr が NULL です\n");
	}
#endif

	if ((insert_now) && (temp_x != 0) && (temp_y != 0)) {
	    /* 今回ノードを追加したのなら */
		t_ptr->x = temp_x;
		t_ptr->y = temp_y;
	}
	if ((t_ptr->data == NULL) && ((t_ptr->x == 0) && (t_ptr->y == 0))) {
	    /* データメモリ上になく、かつサイズが不明の場合 */
		if ((short) (w->width + w->font_size * 4) > WRAP_DOT) {
		    /* 読まなかったことにする */
			w->not_read_flag = !0;
			t_ptr->count--;
			w->nl_flag = !0;
			return;
		} else {
			strcpy (w->_t2, "[絵]");
			w->_t2 += 4;
			w->width += w->font_size * 4;
		}
	} else {
		short y;
		int h = 1, hh, y_offset;

		(xptext->image_table)[xptext->image_table_max].disp_x = t_ptr->x;
	    /* 画像が右端からはみ出てしまうか？ */
		if ((w->width + t_ptr->x) > WRAP_DOT) {
			if (w->width > 0) {
			    /* 送り禁則して解決 */
				w->not_read_flag = !0;
				t_ptr->count--;
				w->nl_flag = !0;	/* 次の行へ */
				return;
			} else {
			    /* どうしても画面に収まらない */
				if (t_ptr->x > WRAP_DOT)
					(xptext->image_table)[xptext->image_table_max].disp_x = WRAP_DOT;
			}
		}
		y = t_ptr->y;
		if (t_ptr->y < 16) {
			hh = 16 - y;
			y_offset = 0;
		} else {
			hh = 0;
			y_offset = t_ptr->x * 2 * (t_ptr->y - 16);
		}

		w->_t2 += sprintf (w->_t2, "`G%d,%d,%x`", xptext->image_table_max, hh, y_offset);
		while ((y -= LINE_Y) > 0) {
			y_offset -= t_ptr->x * 2 * 16;
			if (y_offset < 0) {
				y_offset = 0;
				hh = 16 - y;
			} else {
				hh = 0;
			}

			InsertBLB (h++, 0, 0, 0,
				   w->width, xptext->image_table_max, hh, y_offset);
		}
		w->width += (xptext->image_table)[xptext->image_table_max].disp_x;
	}
	xptext->image_table_max++;
}

static void TagFrameset (WORK * w)
{
	w->_t2 += sprintf (w->_t2, "※ このページはフレームが使われています");
	w->nl_flag = !0;	/* 桁ループを抜ける */
}

static void TagFrame (WORK * w)
{
	HTTPFILE *httpfile = w->httpfile;
	XPTEXT *xptext = w->xptext;

	while (*w->_t1 != '>') {
		GetAttrNo (w, attr_frame_str);

		switch (w->attr_no) {
		case ATTR_FRAME_SRC:
			if (w->pass == 0) {
			    /* 初回の解析なら */
				char temp_fname[256];
				HTTPFILE _t_httpfile, *t_httpfile = &_t_httpfile;

				((xptext->link_table)[xptext->link_table_max]).url = w->link_table_buffer_ptr;

				InitHttpfile (t_httpfile);
				CatHttpfile (t_httpfile, httpfile, w->attr_str);
				strcpy (w->link_table_buffer_ptr, t_httpfile->url);
				if (WCExist (t_httpfile, temp_fname) > WC_NON)
					(xptext->link_table)[xptext->link_table_max].in_cache = !0;
				else
					(xptext->link_table)[xptext->link_table_max].in_cache = 0;
				while (*w->link_table_buffer_ptr++);
			}
		    //w->ffifo[w->ffifo_ptr++] = 'f';
			sprintf (tag_href_str, "`L%dU`%-.80s`lu`", xptext->link_table_max, ((xptext->link_table)[xptext->link_table_max]).url);
			w->_t2 += sprintf (w->_t2, "%s", tag_href_str);
		    //w->width += strlen (tag_href_str) * 6;
			break;

		case ATTR_FRAME_NAME:
		default:
			break;
		}
	}
	xptext->link_table_max++;
	w->nl_flag = !0;	/* 桁ループを抜ける */
}

static void TagHr (WORK * w)
{
	w->ffifo[w->ffifo_ptr++] = 'h';
	w->nl_flag = !0;	/* 桁ループを抜ける */
}

static void TagP (WORK * w)
{
	w->tag_p_align = !0;
	if (!w->tag_center)
		w->align = ALIGN_LEFT;

	while (*w->_t1 != '>') {
		GetAttrNo (w, attr_p_str);

		switch (w->attr_no) {
		case ATTR_P_ALIGN:
			if (!stricmp (w->attr_str, "center"))
				w->align = ALIGN_CENTER;
			if (!stricmp (w->attr_str, "left"))
				w->align = ALIGN_LEFT;
			break;

		default:
			break;
		}
	}
	w->nl_flag = !0;	/* 桁ループを抜ける */
}

static void Tag_P (WORK * w)
{
	w->tag_p_align = 0;
	w->nl_flag = !0;	/* 桁ループを抜ける */
	if (!w->tag_center)
		w->reset_align = !0;
}

static void TagH1 (WORK * w)
{
	char *t2 = w->_t2;

	w->font_size_stack[w->font_size_stackptr++] = w->font_size;
	w->font_size = 8;
	t2 += sprintf (t2, "`S%d`", w->font_size);
	w->nl_flag = !0;

	w->_t2 = t2;
}

static void Tag_H1 (WORK * w)
{
	char *t2 = w->_t2;

	if (w->font_size_stackptr)	/* ネストが正しければ */
		w->font_size = w->font_size_stack[--w->font_size_stackptr];
	t2 += sprintf (t2, "`S%d`", w->font_size);
	w->nl_flag = !0;

	w->_t2 = t2;
}

static void TagH3 (WORK * w)
{
	char *t2 = w->_t2;

	w->font_size_stack[w->font_size_stackptr++] = w->font_size;
	w->font_size = 6;
	t2 += sprintf (t2, "`S%d`", w->font_size);
	w->nl_flag = !0;

	w->_t2 = t2;
}

static void Tag_H3 (WORK * w)
{
	char *t2 = w->_t2;

	if (w->font_size_stackptr)	/* ネストが正しければ */
		w->font_size = w->font_size_stack[--w->font_size_stackptr];
	t2 += sprintf (t2, "`S%d`", w->font_size);
	w->nl_flag = !0;

	w->_t2 = t2;
}
static void TagTitle (WORK * w)
{
	w->tag_title = w->_t1 + 1;
}

static void Tag_Title (WORK * w)
{
	char *t2 = w->_t2;
	XPTEXT *xptext = w->xptext;

	if (w->tag_title != NULL) {
	    /* <title></title> 間をコピー */
		int title_size = (size_t) (w->t1_old - w->tag_title);

		if (title_size > 64)
			title_size = 64;
	    /* strncpy() は末尾に '\0' を付けてくれない事に注意 */
		strncpy (w->xptext->title, w->tag_title, title_size);
		w->xptext->title[title_size] = '\0';

	    /* ポインタをリセット */
		t2 = xptext->text;
		w->t2e = t2 + w->t2_size - YOYUU;
		w->width = 0;
	} else {
		McDbPuts ("<title> がないのに </title> があります\n");
	}
	w->_t2 = t2;
}

static void TagHead (WORK * w)
{
	char *t1 = w->_t1;

	w->tag_head = t1;
}

static void Tag_Head (WORK * w)
{
	char *t2 = w->_t2;
	XPTEXT *xptext = w->xptext;

    /* </head> を検出した時はポインタをリセット */
	if (w->tag_head != NULL) {
		t2 = w->t2t = xptext->text;
		w->t2e = t2 + w->t2_size - YOYUU;
		w->width = 0;
		w->reset_line_ptr = !0;
		xptext->line = 0;
	}
	w->_t2 = t2;
}

static void TagCenter (WORK * w)
{
	w->align = ALIGN_CENTER;
	w->tag_center = !0;
}

static void Tag_Center (WORK * w)
{
	w->tag_center = 0;
	w->reset_align = !0;
	w->nl_flag = !0;
}

static void TagScript (WORK * w)
{
	unsigned char *t1 = w->_t1;
	unsigned char *t2 = w->_t2;

	do {
		while ((*t1++ != '/') && (t1 < w->t1e));
		if (!strnicmp (t1, "script", 6)) {
			t1 += 6;
			w->nl_flag = !0;
			break;
		}
	} while ((t1 < w->t1e) && (t2 < w->t2e));

	w->_t1 = t1;
	w->_t2 = t2;
}

static void Tag_Script (WORK * w)
{
}

static void TagStyle (WORK * w)
{
	unsigned char *t1 = w->_t1;
	unsigned char *t2 = w->_t2;

	do {
		while ((*t1++ != '/') && (t1 < w->t1e));
		if (!strnicmp (t1, "style", 5)) {
			t1 += 5;
			w->nl_flag = !0;
			break;
		}
	} while ((t1 < w->t1e) && (t2 < w->t2e));

	w->_t1 = t1;
	w->_t2 = t2;
}

static void Tag_Style (WORK * w)
{
}

static void TagPre (WORK * w)
{
	w->tag_pre = !0;
}

static void Tag_Pre (WORK * w)
{
	w->tag_pre = 0;
}

static void TagOl (WORK * w)
{
	w->tag_ol = !0;
	w->tag_list_no = 1;
}

static void Tag_Ol (WORK * w)
{
	w->tag_ol = 0;
	w->nl_flag = !0;	/* 桁ループを抜ける */
}

static void TagUl (WORK * w)
{
	w->tag_ul = !0;
	w->tag_list_no = 1;
}

static void Tag_Ul (WORK * w)
{
	w->tag_ul = 0;
	w->nl_flag = !0;	/* 桁ループを抜ける */
}

static void TagLi (WORK * w)
{
	w->ffifo[w->ffifo_ptr++] = 'l';
	w->nl_flag = !0;
}

static void TagTable (WORK * w)
{
	w->nl_flag = !0;
}

static void Tag_Td (WORK * w)
{
	char *t2 = w->_t2;

	if (w->width + 6 * 8 > WRAP_DOT) {
		w->not_read_flag = !0;
		w->nl_flag = !0;
	} else {
		*t2++ = 0x09;	/* tab */
		w->width = (w->width + 6 * 8) / (short) (6 * 8) * (short) (6 * 8);
	}

	w->_t2 = t2;
}

static void Tag_Tr (WORK * w)
{
	w->nl_flag = !0;
}

static void TagDt (WORK * w)
{
	w->nl_flag = !0;
}

static void TagDd (WORK * w)
{
	w->ffifo[w->ffifo_ptr++] = 'd';
	w->nl_flag = !0;
}

static void TagInput (WORK * w)
{
	while (*w->_t1 != '>') {
		GetAttrNo (w, attr_input_str);

		switch (w->attr_no) {
		case ATTR_INPUT_TYPE:
#if	0
		case ATTR_INPUT_TYPE_RADIO:
			t2 += sprintf (t2, "`E1`");
			break;
		case ATTR_INPUT_TYPE_CHECKBOX:
			t2 += sprintf (t2, "`E0`");
			break;
		case ATTR_INPUT_TYPE_RESET:
			t2 += sprintf (t2, "`E0`");
			break;
		case ATTR_INPUT_TYPE_SUBMIT:
			t2 += sprintf (t2, "`E0`");
			break;
		default:
			t2 += sprintf (t2, "`E0`");
			break;
#endif
		default:
			break;
		}
	}
}

static void TagBody (WORK * w)
{
	while (*w->_t1 != '>') {
		signed short no = -1;

		GetAttrNo (w, attr_body_str);

		switch (w->attr_no) {
		case ATTR_BODY_TEXT:
			no = 1;
			break;

		case ATTR_BODY_LINK:
			no = 3;
			break;

		case ATTR_BODY_BGCOLOR:
			no = 0;
			break;

		case ATTR_BODY_ALINK:
		case ATTR_BODY_VLINK:
		default:
			break;
		}
		if ((no >= 0) && (w->pass == 0)) {
			if (w->attr_str[0] == '#')
				html_color[no] = Str2Color (w->attr_str);
			else
				html_color[no] = 1;	/* debug */
		}
	}
}

static void TagComment (WORK * w)
{
}



/*

   httpfile->content の指す HTML を解析する
   返り値は新しく確保した XPTEXT 構造体へのポインタ
   （元から入っていた httpfile->xptext は参照のみで変更しない）
 */
XPTEXT *Html2Xpression (HTTPFILE * httpfile)
{
	XPTEXT *xptext;		/* 返り値 */
	LINE_PTR *l;		/* 現在処理している行テーブル */
#if	1
	register unsigned char *t1 asm ("a4");	/* 現在処理している文字（転送元） */
	register unsigned char *t2 asm ("a5");	/* 〃              （転送先） */
	register unsigned char c asm ("d7");	/* 処理する文字 */
#else
	unsigned char *t1;	/* 現在処理している文字（転送元） */
	unsigned char *t2;	/* 〃              （転送先） */
	unsigned char c;	/* 処理する文字 */
#endif
	unsigned short org_line;	/* 元の HTML の何行目だったか */

	WORK _w;
	register WORK *w asm ("a3") = &_w;

	w->align = ALIGN_LEFT;	/* デフォルトは左揃え */
	w->tag_center = 0;
	w->tag_href = 0;
	w->tag_pre = 0;
	w->tag_ol = 0;
	w->tag_ul = 0;
	w->reset_align = 0;
	w->tag_p_align = 0;
	w->tag_list_no = 0;
	w->tag_head = NULL;
	w->tag_title = NULL;
	w->font_size = 6;
	w->ffifo_ptr = 0;
	w->t2_size = 0;
	w->space_flag = 0;	/* 直前が半角スペースだったか */
	w->font_size_stackptr = 0;
	org_line = 0;		/* 元の HTML の何行目だったか */


	xptext = malloc (sizeof (XPTEXT));
	if (xptext == NULL) {
		McPuts ("※ メモリが足りません（ XPTEXT 用のメモリが確保できません）\n");
		return (NULL);
	};
	xptext->text = NULL;
	xptext->line_ptr = NULL;
	xptext->anchor_table = NULL;
	xptext->link_table_max = 0;
	xptext->image_table_max = 0;

	*(xptext->title) = '\0';
	strcpy (xptext->title, "タイトル未設定");
	if (!httpfile->xptext)	/* １パス目か？ */
		w->pass = 0;
	else
		w->pass = 1;


    /* 毎回確保するワーク */

#define XPTEXT_BUFFER_YOYUU	32768
    /* これ位あれば足りるかな？ */
	w->t2_size = httpfile->content_length * 2 + XPTEXT_BUFFER_YOYUU;

    /* とりあえず固定サイズで確保 */
	xptext->text = malloc (w->t2_size);
	if ((int) xptext->text == 0) {
		McPuts ("※ メモリが足りません（テキストバッファ用のメモリが確保できません）\n");
		FreeXptext (xptext);
		return (NULL);
	} else {
		xptext->line_ptr = malloc (sizeof (LINE_PTR) * line_table_size);
		if ((int) xptext->line_ptr == 0) {
			McPuts ("※ メモリが足りません（行頭リスト用メモリが確保できません）\n");
			FreeXptext (xptext);
			return (NULL);
		} else {
			xptext->anchor_table = malloc (sizeof (ANCHOR_TABLE) * anchor_table_size);
			if ((int) xptext->anchor_table == 0) {
				McPuts ("※ メモリが足りません（アンカーテーブル用のメモリが確保できません）\n");
				FreeXptext (xptext);
				return (NULL);
			}
		}
	}
    /* 要するに *t1（べた読みした .HTM）から *t2 に整形しながらコピーしていくわけだ */
	t1 = httpfile->content;	/* html ファイル本体へのポインタ */
	w->t1e = t1 + httpfile->content_length;		/* t1 が w->t1e に達したら終了 */
	t2 = xptext->text;	/* xptext バッファへのポインタ */
	w->t2e = t2 + w->t2_size - YOYUU;	/* t2 が w->t2e に達したら終了（バッファ不足） */
	xptext->line = 0;
	xptext->anchor_table_max = 0;


	l = xptext->line_ptr;

	if (w->pass == 0) {
	    /* 初回の解析なら */
		short h;

		xptext->link_table = NULL;
		xptext->link_table_buffer = NULL;
		xptext->image_table = NULL;

	    /* とりあえず固定サイズで確保 */
		xptext->link_table = malloc (sizeof (LINK_TABLE) * link_table_size);
		if ((int) xptext->link_table == 0) {
			McPuts ("※ メモリが足りません（リンクテーブル用のメモリが確保できません）\n");
			FreeXptext (xptext);
			return (NULL);
		} else {
			xptext->link_table_buffer = malloc (sizeof (unsigned char) * link_table_buffer_size);
			if ((int) xptext->link_table_buffer == 0) {
				McPuts ("※ メモリが足りません（リンクテーブルバッファ用のメモリが確保できません）\n");
				free (xptext->link_table);
				FreeXptext (xptext);
				return (NULL);
			} else {
				xptext->image_table = malloc (sizeof (IMAGE_TABLE) * image_table_size);
				if ((int) xptext->image_table == 0) {
					McPuts ("※ メモリが足りません（イメージテーブル用のメモリが確保できません）\n");
					free (xptext->link_table_buffer);
					free (xptext->link_table);
					FreeXptext (xptext);
					return (NULL);
				}
			}
		}
		w->link_table_buffer_ptr = xptext->link_table_buffer;

		xptext->current_line = 0;

		for (h = 0; h < 7; h++)
			html_color[h] = config_color[h];

	    /* debug バグっても NULL ポインタで止まるように */
		for (h = 0; h < image_table_size; h++)
			(xptext->image_table)[h].image_list = NULL;
	} else {
	    /* ２回目以降の解析なら以下の値は前回の解析結果を引き継ぐ */
		xptext->link_table = httpfile->xptext->link_table;
		xptext->link_table_buffer = httpfile->xptext->link_table_buffer;
		xptext->image_table = httpfile->xptext->image_table;
		w->link_table_buffer_ptr = NULL;
		xptext->current_line = httpfile->xptext->current_line;
	}
	w->httpfile = httpfile;
	w->xptext = xptext;


    /* 行ループ */
	do {
		blb_top = NULL;
		blb_end = NULL;
		w->width = 0;
		w->t2t = t2;

		w->space_flag = 0;
		w->reset_line_ptr = 0;

		if (w->ffifo_ptr) {
			switch (w->ffifo[--w->ffifo_ptr]) {
			case 'h':
				strcpy (t2, "`S08`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
				w->width = 64 * 8;
				t2 += 69;
				break;
			case 'l':
				if (w->tag_ol) {	/* 番号付きリスト */
					t2 += sprintf (t2, "%hd:", w->tag_list_no);
					w->width += w->font_size * 4;
					if (++w->tag_list_no > 999)
						w->tag_list_no = 1;
				} else {
					if (w->tag_ul) {	/* 番号無しリスト */
						t2 += sprintf (t2, "・");
						w->width += w->font_size * 2;
					}
				}
				break;
			case 'd':	/* タグ <dd> */
				*t2++ = 0x09;
				w->width = (w->width + 6 * 8) / (short) (6 * 8) * (short) (6 * 8);
				break;
			default:
				break;
			}
			w->ffifo_ptr = 0;	/* いらないハズなんだが・・・ */
		}
		if (w->font_size != 6)	/* 前行から大きい文字が続いていれば */
			t2 += sprintf (t2, "`S%d`", w->font_size);
		if (w->tag_href)	/* 前行からタグが続いていれば */
			t2 += sprintf (t2, "%s", tag_href_str);
		*tag_href_str = '\0';

	    /* 桁ループ */
		do {
			w->t1_old = t1;
			c = *t1++;

			if (c == '<') {		/* タグ発見！ */
				w->nl_flag = 0;
				w->not_read_flag = 0;

			    /* '<' の後にスペースがあれば読み飛ばす */
				w->_t1 = t1;
				if (SkipSpace (w)) {
				    /* エラー処理まだ */
					t1 = w->_t1;
				}
			    /* t1 の指すタグを tag_no に返す */
				if (GetTagNo (w)) {
				    /* エラー処理まだ */
					t1 = w->_t1;
				}
				t1 = w->_t1;

			    /* ここに来た時点で t1 はタグの次（スペースがあれば読み飛ばす）を
			       指している。"<A HREF=" だったら 'H'、"<BR>" だったら '>'。 */

			    /* タグによって分岐（関数へのポインタってヤツ） */
				if (w->tag_no >= 0) {
#if 0
					McDbPrintf ("TAG : %s\n", tag_str[w->tag_no]);
#endif
					w->_t1 = t1;
					w->_t2 = t2;
					FuncTag[w->tag_no] (w);
					t1 = w->_t1;
					t2 = w->_t2;
				}
			    /* 読まなかったことにしたか？ */
				if (w->not_read_flag) {
					t1 = w->t1_old;
				} else {
					w->_t1 = t1;
					SkipLt (w);
					t1 = w->_t1;
				}
				if (w->reset_line_ptr) {
					l = xptext->line_ptr;
				}
				if (w->nl_flag)
					break;
				else
					continue;
			}
		    /* 文字エンティティの処理 */
			if (c == '&') {
			    /* t1 の指す文字エンティティを entity_no に返す */
				short entity_no = 0;
				char **s = entity_str;
				int *l = entity_len;
				do {
					if (!strnicmp (*s, t1, *l)) {
						char t = *(t1 + *l);
						if ((t == ' ') || (t == 0x09) || (t == ';') || (t == 0x0a) || (t == 0x0d))
							break;
					}
					s++;
					l++;
					entity_no++;
				} while (*s != NULL);
			    /* エンティティによって分岐 */
				switch (entity_no) {
				case ENTITY_LT:
				case ENTITY_GT:
				case ENTITY_AMP:
				case ENTITY_QUOT:
				case ENTITY_NBSP:
					t1 += (strlen (entity_str[entity_no]) + 1);
					c = entity_char[entity_no];
					break;
				default:
				    /* どれでもなかった場合そのまま表示 */
					break;
				}
			}
			if (c == 0x09)
				c = ' ';

		    /* 連続するスペース(0x20)は１つにまとめる */
			if ((c == ' ') && (!w->tag_pre)) {
				if (w->space_flag)	/* 直前がスペースだったか */
					continue;	/* 次の文字へ */
				w->space_flag = !0;
			} else {
				w->space_flag = 0;
			}

			if (c == 0x0d) {
				org_line++;
				if (*t1 == 0x0a)
					t1++;
				if (!w->tag_pre)
					continue;	/* 次の文字へ */
				else
					break;	/* 次の行へ */
			}
			if (c == 0x0a) {
				org_line++;
				if (!w->tag_pre)
					continue;	/* 次の文字へ */
				else
					break;	/* 次の行へ */
			}
			if (c < ' ') {
			    //*t2++ = c;    /* 0x20以下の文字は無視 */
				continue;
			}
			if (c == '`') {		/* Xpression形式文字列のメタキャラクタ */
				if (w->width + w->font_size > WRAP_DOT) {
					t1 = w->t1_old;		/* 読まなかったことにする */
					break;
				}
				*t2++ = c;
				if (w->font_size * 2 > LINE_Y)
					InsertBLB (1, c, 0, w->font_size, w->width, 0, 0, 0);
				w->width += w->font_size;
				*t2++ = c;
				if (w->font_size * 2 > LINE_Y)
					InsertBLB (1, c, 0, w->font_size, w->width, 0, 0, 0);
				w->width += w->font_size;
				continue;
			}
			if (c < 0x80) {
			    /* １バイト文字の場合 */
				if (w->width + w->font_size > WRAP_DOT) {
					t1 = w->t1_old;		/* 読まなかったことにする */
					break;
				}
				*t2++ = c;
				if (w->font_size * 2 > LINE_Y)
					InsertBLB (1, c, 0, w->font_size, w->width, 0, 0, 0);
				w->width += w->font_size;
				continue;
			} else {
				if ((c >= 0xa0) && (c <= 0xdf)) {
				    /* 半角カナの場合 */
					if (w->width + w->font_size > WRAP_DOT) {
						t1 = w->t1_old;		/* 読まなかったことにする */
						break;
					}
					*t2++ = c;
					if (w->font_size * 2 > LINE_Y)
						InsertBLB (1, c, 0, w->font_size, w->width, 0, 0, 0);
					w->width += w->font_size;
					continue;
				} else {
				    /* 漢字の場合 */
					unsigned char c1;
					if ((w->width + w->font_size * 2) > WRAP_DOT) {
						t1 = w->t1_old;		/* 読まなかったことにする */
						break;
					}
					*t2++ = c;
					c1 = *t2++ = *t1++;
					if (w->font_size * 2 > LINE_Y)
						InsertBLB (1, c, c1, w->font_size, w->width, 0, 0, 0);
					w->width += w->font_size * 2;
					continue;
				}
			}
		} while ((t1 < w->t1e) && (t2 < w->t2e));	/* 桁ループ終了 */
		*t2++ = '\0';

		if ((t1 > w->t1e) || (t2 > w->t2e))
			break;


		if (w->align == ALIGN_CENTER)
			l->start_dot = (WRAP_DOT - w->width) / 2;
		else
			l->start_dot = 0;
		l->org_line = org_line;

		if (w->reset_align) {
			w->align = ALIGN_LEFT;
			w->reset_align = 0;
		}
		if (blb_top == NULL) {
			l++->ptr = w->t2t;
			xptext->line++;		/* 行数 */
		} else {	/* 前行処理が必要な場合 */
			unsigned char temp[1024];
			int len1;	/* 現在行の文字数 */
			unsigned short s = l->start_dot;
			BLB *t_ptr;

			len1 = (int) (t2 - w->t2t);
			strncpy (temp, w->t2t, len1);	/* 現在行を退避 */

			t2 = w->t2t;
			while ((t_ptr = DeleteBLB ()) != NULL) {
				l->ptr = t2;
				strcpy (t2, t_ptr->text);
				t2 += strlen (t_ptr->text);
				*t2++ = '\0';
				l->start_dot = s;
				l->org_line = org_line;
				xptext->line++;		/* 行数 */
				l++;
				free (t_ptr);
			}
			l->ptr = t2;
			strcpy (t2, temp);
			t2 += len1;
			*t2++ = '\0';
			l->start_dot = s;
			l->org_line = org_line;
			xptext->line++;		/* 行数 */
			l++;
		}
	} while ((t1 < w->t1e) && (t2 < w->t2e) && (xptext->line < line_table_size));
    /* 行ループ終了 */

	l->ptr = NULL;


	if (t2 >= w->t2e)
		McPuts ("※ テキスト解析バッファが足りません\n");

	if ((xptext->line >= line_table_size))
		McPuts ("※ 行頭テーブルが足りません\n");


    /* 余分に確保したメモリブロックを切り捨てる */
	xptext->text = realloc (xptext->text, (int) t2 - (int) (xptext->text) + 1);
	xptext->line_ptr = realloc (xptext->line_ptr, sizeof (LINE_PTR) * (xptext->line));
	if (w->pass == 0) {	/* 初回の解析なら */
		xptext->link_table_buffer = realloc (xptext->link_table_buffer,
			       sizeof (char) * ((int) (w->link_table_buffer_ptr - xptext->link_table_buffer)) + 1);
		xptext->link_table = realloc (xptext->link_table, sizeof (LINK_TABLE) * (xptext->link_table_max + 1));
		xptext->image_table = realloc (xptext->image_table, sizeof (IMAGE_TABLE) * (xptext->image_table_max + 1));
	}
	return (xptext);
}



/* リンクテーブルの in_cache を更新する */
void ReCheckLinkTable (HTTPFILE * httpfile)
{
	HTTPFILE _t_httpfile, *t_httpfile = &_t_httpfile;
	char temp_fname[256];
	int i;

	for (i = 0; i < httpfile->xptext->link_table_max; i++) {
		InitHttpfile (t_httpfile);
		CatHttpfile (t_httpfile, httpfile, httpfile->xptext->link_table[i].url);
		if (WCExist (t_httpfile, temp_fname) > WC_NON)
			(httpfile->xptext->link_table)[i].in_cache = !0;
		else
			(httpfile->xptext->link_table)[i].in_cache = 0;
	}
}



/* アンカーを検索して行数を返す */
int SearchAnchor (XPTEXT * xptext, char *anchor)
{
	int i;

	for (i = 0; i < xptext->anchor_table_max; i++) {
	    /* anchor+1 なのは '#' を飛ばすため */
		if (!stricmp (((xptext->anchor_table)[i]).anchor, anchor + 1))
			return (((xptext->anchor_table)[i]).line);
	}
	return (0);
}



/* 毎回確保するワークを開放 */
void FreeXptext (XPTEXT * xptext)
{
	if (xptext) {
		if (xptext->text)
			free (xptext->text);
		if (xptext->line_ptr)
			free (xptext->line_ptr);
		if (xptext->anchor_table)
			free (xptext->anchor_table);

	    /* link_table, link_table_buffer, image_table はここで捨ててはいけない。理由はWe
	       bXpression.c の FreeXptext() 呼び出し部を見ること（複数回解析を行う度にここが
	       呼ばれるため） */

		free (xptext);
	}
}



void FreeXptext2 (XPTEXT * xptext)
{
	short i;

	if (xptext != NULL) {
		if (xptext->link_table != NULL)
			free (xptext->link_table);
		if (xptext->link_table_buffer != NULL)
			free (xptext->link_table_buffer);
		if (xptext->image_table != NULL) {
			for (i = 0; i < xptext->image_table_max; i++) {
				IMAGE_LIST *t_ptr;
				t_ptr = (xptext->image_table)[i].image_list;
				t_ptr->count--;		/* リンクカウントを１つ下げる */
			}
			free (xptext->image_table);
		}
	}
}
