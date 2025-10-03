/* テキストファイルを Xpression 形式に変換 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <x68k/dos.h>
#include "WebXpression.h"
#include "Httpfile.h"
#include "Html2Xpression.h"
#include "MicroConsole.h"
#include "Config.h"

/* 折り返すドット数 */
#define WRAP_DOT	(short)512
#define LINE_Y	16

#define YOYUU	1024



XPTEXT *Plain2Xpression (HTTPFILE * httpfile, XPTEXT * old_xptext)
{
	XPTEXT *xptext;		/* 返り値 */
	LINE_PTR *l;		/* 現在処理している行テーブル */
	unsigned short width;	/* ドット数 */
	unsigned char *t1, *t2;	/* 現在処理している文字 */
	unsigned char *t1e, *t2e;	/* t1,t2 の末尾 */
	unsigned char *t1_old;	/* 「処理した文字を読まなかった事にする」用 */
	unsigned char *t2t;	/* その行の先頭の t2 */
	unsigned int t2_size = 0;	/* t2 のサイズ */
	unsigned char font_size = 6;	/* 半角文字の大きさ */
	unsigned short org_line = 0;	/* 元の HTML の何行目だったか */
	short h;

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

#define XPTEXT_BUFFER_YOYUU	32768
	/* これ位あれば足りるかな？ */
	t2_size = httpfile->content_length * 2 + XPTEXT_BUFFER_YOYUU;

	/* とりあえず固定サイズで確保 */
	xptext->text = malloc (t2_size);
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
		}
	}
	/* 要するに *t1 から *t2 に整形しながらコピーしていくわけだ */
	t1 = httpfile->content;	/* テキストファイル本体へのポインタ */
	t1e = t1 + httpfile->content_length;	/* t1 が t1e に達したら終了 */
	t2 = xptext->text;	/* xptext バッファへのポインタ */
	t2e = t2 + t2_size - YOYUU;
	/* t2 が t2e に達したら終了（バッファ不足） */
	xptext->line = 0;
	xptext->anchor_table_max = 0;


	l = xptext->line_ptr;

	xptext->link_table = NULL;
	xptext->link_table_buffer = NULL;
	xptext->image_table = NULL;
	xptext->link_table_max = 0;
	xptext->image_table_max = 0;

	xptext->current_line = 0;

	for (h = 0; h < 7; h++)
		html_color[h] = config_color[h];

	/* 行ループ */
	do {
		width = 0;
		t2t = t2;

		/* 桁ループ */
		do {
			unsigned char c;	/* 処理する文字 */

			t1_old = t1;
			c = *t1++;

			if (c == 0x0d) {
				org_line++;
				if (*t1 == 0x0a)
					t1++;
				break;
			}
			if (c == 0x0a) {
				org_line++;
				break;
			}
			if (c == 0x09) {
				if (width + font_size * 8 > WRAP_DOT) {
					t1 = t1_old;	/* 読まなかったことにする */
					break;
				} else {
					*t2++ = c;
					width = (width + font_size * 8) / (short) (font_size * 8) * (short) (font_size * 8);
					continue;
				}
			}
			if (c < ' ') {
				//*t2++ = c;
				//width += font_size;
				break;
			}
			if (c == '`') {
				if (width + font_size > WRAP_DOT) {
					t1 = t1_old;	/* 読まなかったことにする */
					break;
				}
				*t2++ = c;	/* '`' は '``' に */
				*t2++ = c;
				width += font_size;
				continue;
			}
			if (c < 0x80) {
				/* １バイト文字の場合 */
				if (width + font_size > WRAP_DOT) {
					t1 = t1_old;	/* 読まなかったことにする */
					break;
				}
				*t2++ = c;
				width += font_size;
				continue;
			} else {
				if ((c >= 0xa0) && (c <= 0xdf)) {
					/* 半角カナの場合 */
					if (width + font_size > WRAP_DOT) {
						t1 = t1_old;		/* 読まなかったことにする */
						break;
					}
					*t2++ = c;
					continue;
				} else {
					/* 漢字の場合 */
					if ((width + font_size * 2) > WRAP_DOT) {
						t1 = t1_old;	/* 読まなかったことにする */
						break;
					}
					*t2++ = c;
					*t2++ = *t1++;
					width += font_size * 2;
					continue;
				}
			}
		} while ((t1 < t1e) && (t2 < t2e));	/* 桁ループ終了 */
		*t2++ = '\0';

		l->start_dot = 0;
		l->org_line = org_line;
		l++->ptr = t2t;
		xptext->line++;	/* 行数 */
	} while ((t1 < t1e) && (t2 < t2e));	/* 行ループ終了 */
	l->ptr = NULL;


	/* 余分に確保したメモリブロックを切り捨てる */
	xptext->text = realloc (xptext->text, (int) t2 - (int) (xptext->text) + 1);
	xptext->line_ptr = realloc (xptext->line_ptr, sizeof (LINE_PTR) * (xptext->line));

	return (xptext);
}
