/* WebXpression.c */
/* Not for communication, only for xpression... */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <x68k/dos.h>
#include <x68k/iocs.h>

#define GLOBAL_DEFINE		/* グローバル変数を確保する */
#include "WebXpression.h"
#include "Httpfile.h"
#include "Html2Xpression.h"
#include "Image.h"
#include "History.h"
#include "MicroConsole.h"
#include "Config.h"
#include "WebCache.h"
#include "DrawText.h"
#include "GetFile.h"
#include "data.h"

extern int cut_disp (void *, void *, int);

/* スタックサイズとヒープサイズを指定 */
int _stack_size = 32 * 1024;
int _heap_size = 256 * 1024;

static unsigned char t_option = 0;

static int old_screen_mode;
static int old_fnkmod;
static char address_book[92 + 7];	/* "file://" 込みのアドレス帳フルパスファイル名 */

static char mouse_repeat = 0;
static char scbar_drag = 0;

#define TEXTVRAM	0xe00000

#define STR_DISP_COMPLETE	"──────────── 表示完了 ─\n"
static void Init2 (void);
static void Tini2 (void);


static EVENTREC _eventrec;	/* こちらは使わず */
EVENTREC *eventrec = &_eventrec;	/* こっちでアクセス */


enum {
	MAIN_ERROR_NON = 0, MAIN_ERROR_GETFILE, MAIN_ERROR_XPTEXT, MAIN_ERROR_RECHECK, MAIN_ERROR_SUCCESS
};

enum {
	MSLRUP = 0, MSLDOWN, MSRDOWN, MSLRDOWN
};


static void usage (void)
{
	puts (
		     "WWW ブラウザ WebXpression.x ver0.46\n"
		     "		programmed by Mitsuky <FreeSoftware>\n"
		     "Hyper Text Transfer Protocol に従って HTML ファイルを表示します\n"
		     "ネットワーク上のファイルを表示する場合、TCP/IP ドライバが必要です\n"
		     "使用法 : WebXpression [option] [URL]\n"
		     "[option]\n"
		     "	-Cファイル名 : .cnf ファイルを指定\n"
		     "	-D : デバッグモード\n"
		     "URL 未指定時にはアドレス帳を表示します\n"
		     "パス名はローカルファイル時も '/' で区切って指定して下さい\n"
		);
}



/* [INTERRUPT] が押されたらここに飛んでくる */
static void InterruptAbort (void)
{
	Tini2 ();

	exit (1);		/* 終了しちゃう */
}



/* -d オプション用 */
static void DumpXptext (XPTEXT * xptext)
{
	LINE_PTR *l = xptext->line_ptr;
	int i;
	for (i = 0; i < xptext->line; i++) {
		printf ("%s****%hd\n", l->ptr, l->start_dot);
		l++;
	}
	printf ("リンクテーブルを表示します\n");
	for (i = 0; i < xptext->link_table_max; i++) {
		printf ("リンク L%04d : %s :", i, (xptext->link_table)[i]);
		if ((xptext->link_table)[i].in_cache == 0)
			printf ("ない\n");
		else
			printf ("存在\n");
	}
	DispImageList ();
}


/* アドレス帳に追加 */
static void AddAddress (HTTPFILE * httpfile)
{
	FILE *fp;

    /* file:// を飛ばすから [7] */
	if ((fp = fopen (&address_book[7], "a+")) != NULL) {
		fprintf (fp, "<LI><A HREF=\"%s\">%s</A>\n", httpfile->url, httpfile->xptext->title);
		fclose (fp);
		McPuts ("アドレス帳に登録しました\n");
	} else {
		McPuts ("※ アドレス帳に書き込めません\n");
	}
}



/* キー／マウスのボタンが離されるまで待つ */
void WaitReleaseAll (void)
{
	int i;
	do {
		for (i = 0; i < 15; i++)
			if (_iocs_bitsns (i))
				break;
	} while ((i != 15) || (_iocs_ms_getdt () & 0xffff));
}



/* マイクロコンソール用 printf() （要 stdarg.h） */
void McPrintf (const char *format,...)
{
	char temp_str[1024];
	va_list ap;

	va_start (ap, format);
	vsprintf (temp_str, format, ap);
	McPuts (temp_str);
	va_end (ap);

	return;
}



/* デバッグモード用の printf() （要 stdarg.h） */
void McDbPrintf (const char *format,...)
{
	char temp_str[1024];
	va_list ap;

	if (d_option) {
		va_start (ap, format);
		vsprintf (temp_str, format, ap);
		McPuts (temp_str);
		va_end (ap);
	}
	return;
}



/* GetFile() での中断チェック */
int AbortCheckGetFile (void)
{
	int ret = GF_SUCCESS;
	int k;
#define KEY_ESC		0x02
	k = _iocs_bitsns (0);
	if (k & KEY_ESC)
		ret = GF_ABORT_ESC;

#define KEY_BREAK	0x02
	k = _iocs_bitsns (0x0c);
	if (k & KEY_BREAK)
		ret = GF_ABORT_BREAK;

	return (ret);
}



/* マウスを読んでイベントを返す */
static void CheckMouseOnPanel (EVENTREC * eventrec)
{
	char type = EVENT_IDLE;
	char pat = eventrec->old_mouse_pat;
	signed short mx = eventrec->mouse_x, my = eventrec->mouse_y;	/* 高速化のため */

	typedef struct {
		signed short x0, y0, x1, y1;
		char left_event, right_event;
	} BUTTON;

	BUTTON button[] =
	{
	  256 + 292, 96, 256 + 292 + 59, 96 + 15, EVENT_SCROLL_FORWARD, EVENT_SCROLL_BACKWARD,
		256 + 292, 96 + 16, 256 + 292 + 64, 96 + 15 + 16, EVENT_PAGE_FORWARD, EVENT_PAGE_BACKWARD,
	     256 + 292, 96 + 36, 256 + 292 + 64, 96 + 15 + 36, EVENT_PAGE_TOP, EVENT_PAGE_END,
	 256 + 448, 96, 256 + 488 + 64, 96 + 15, EVENT_SET_CONFIG_COLOR, EVENT_SET_HTML_COLOR,
		256 + 448, 312, 256 + 448 + 64, 312 + 15, EVENT_QUIT, EVENT_QUIT,
		-1, -1, -1, -1, EVENT_IDLE, EVENT_IDLE	/* 終了コード */
	};
	BUTTON *bp;

    /* マウスのボタンの状態によって分岐 */
	switch (eventrec->mouse_button) {
	case MSLRUP:
		pat = PAT_POINTER;
		type = EVENT_IDLE;
		break;

	case MSLDOWN:
		bp = button;
		while (bp->x0 >= 0) {
			if ((mx >= bp->x0) && (my >= bp->y0) && (mx < bp->x1) && (my < bp->y1)) {
				type = bp->left_event;
				break;
			}
			bp++;
		}
		break;

	case MSRDOWN:
		bp = button;
		while (bp->x0 >= 0) {
			if ((mx >= bp->x0) && (my >= bp->y0) && (mx < bp->x1) && (my < bp->y1)) {
				type = bp->right_event;
				break;
			}
			bp++;
		}
		break;

	case MSLRDOWN:
		pat = PAT_POINTER;
		type = EVENT_IDLE;
		break;
	}


	eventrec->type = type;
	eventrec->mouse_pat = pat;

	if (eventrec->old_mouse_pat != pat)
		_iocs_ms_sel (pat);

	return;
}



/* マウスを読んでイベントを返す・スクロールバー上でクリックされた場合 */
static void CheckMouseOnScbar (EVENTREC * eventrec)
{
	char type = EVENT_IDLE;
	char pat = eventrec->old_mouse_pat;
	int line = eventrec->check_xptext->line;
	int current_line = eventrec->check_xptext->current_line;

	pat = PAT_POINTER;
#define DISP_Y	32
	switch (eventrec->mouse_button) {
	case MSLRUP:
		mouse_repeat = 0;
		break;

	case MSLDOWN:
		if (line > DISP_Y) {
			int scbar_y;
			scbar_y = CalcScbarY( current_line,line - DISP_Y);
			if (eventrec->mouse_y < scbar_y) {
			    /* サムより上でクリックされた */
				if (mouse_repeat == (key_repeat_1st + key_repeat_2nd))
					mouse_repeat = key_repeat_1st;
				if ((mouse_repeat == 0) || (mouse_repeat == key_repeat_1st)) {
					int y;
					y = CalcScbarLine(eventrec->mouse_y , line - DISP_Y);
					/* １ページ戻るとマウスカーソルを追い越してしまうか？ */
					if (current_line - DISP_Y >= y ){
						type = EVENT_PAGE_BACKWARD;
					}else{
						type = EVENT_PAGE_JUMP;
						eventrec->check_xptext->current_line = y;
					}
				}
				mouse_repeat++;
			} else {
				if (eventrec->mouse_y >= scbar_y + 16) {
				    /* サムより下でクリックされた */
					if (mouse_repeat == (key_repeat_1st + key_repeat_2nd))
						mouse_repeat = key_repeat_1st;
					if ((mouse_repeat == 0) || (mouse_repeat == key_repeat_1st)) {
						int y;
						y = CalcScbarLine(eventrec->mouse_y , line - DISP_Y);
						/* １ページ進むとマウスカーソルを追い越してしまうか？ */
						if (current_line + DISP_Y < y ){
							type = EVENT_PAGE_FORWARD;
						}else{
							type = EVENT_PAGE_JUMP;
							eventrec->check_xptext->current_line = y;
						}
					}
					mouse_repeat++;
				} else {
				    /* サムがクリックされた */
					scbar_drag = !0;
				}
			}
		}
		break;

	default:
		break;
	}
	eventrec->type = type;
	eventrec->mouse_pat = pat;

	if (eventrec->old_mouse_pat != pat)
		_iocs_ms_sel (pat);

	return;
}



/* マウスを読んでイベントを返す・テキスト上でクリックされた場合 */
static void CheckMouseOnText (EVENTREC * eventrec)
{
	char type = EVENT_IDLE;
	char pat = eventrec->old_mouse_pat;

    /* マウスのボタンの状態によって分岐 */
	switch (eventrec->mouse_button) {
	case MSLRUP:
		mouse_repeat = 0;
		eventrec->link_num = CheckLink ((eventrec->mouse_x << 16 | eventrec->mouse_y), eventrec->check_xptext);
		if (eventrec->link_num >= 0) {
			pat = PAT_POINTER;
		} else {
			pat = PAT_NO_SCROLL;
		}
		type = EVENT_IDLE;
		break;

	case MSLDOWN:
		if (eventrec->shift) {
		    /* シフトキーが押されていたらページ移動 */
		    /* キーリピート処理 */
			if (mouse_repeat == (key_repeat_1st + key_repeat_2nd))
				mouse_repeat = key_repeat_1st;
			if ((mouse_repeat == 0) || (mouse_repeat == key_repeat_1st)) {
				pat = PAT_SCROLL_FF;
				type = EVENT_PAGE_FORWARD;
			}
			mouse_repeat++;
		} else {
		    /* 前回のイベントによって分岐 */
			switch (eventrec->type) {
			case EVENT_SCROLL_FORWARD:
			    /* マウスが下にドラッグされた？ */
				if (eventrec->mouse_y > eventrec->old_mouse_y) {
					pat = PAT_SCROLL_FF;
					type = EVENT_SCROLL_FASTFORWARD;
				} else {
					pat = PAT_SCROLL_F;
					type = EVENT_SCROLL_FORWARD;
				}
				break;
			case EVENT_SCROLL_FASTFORWARD:
			    /* マウスが上にドラッグされた？ */
				if (eventrec->mouse_y < eventrec->old_mouse_y) {
					pat = PAT_SCROLL_F;
					type = EVENT_SCROLL_FORWARD;
				} else {
					pat = PAT_SCROLL_FF;
					type = EVENT_SCROLL_FASTFORWARD;
				}
				break;
			default:
				eventrec->link_num = CheckLink ((eventrec->mouse_x << 16 | eventrec->mouse_y), eventrec->check_xptext);
				if (eventrec->link_num >= 0) {
					pat = PAT_POINTER;
					type = EVENT_LINK;
				} else {
					pat = PAT_SCROLL_F;
					type = EVENT_SCROLL_FORWARD;
				}
				break;
			}
		}
		break;

	case MSRDOWN:
		if (eventrec->shift) {
		    /* キーリピート処理 */
			if (mouse_repeat == (key_repeat_1st + key_repeat_2nd))
				mouse_repeat = key_repeat_1st;
			if ((mouse_repeat == 0) || (mouse_repeat == key_repeat_1st)) {
				pat = PAT_SCROLL_FB;
				type = EVENT_PAGE_BACKWARD;
			}
			mouse_repeat++;
		} else {
		    /* 前回のイベントによって分岐 */
			switch (eventrec->type) {
			case EVENT_SCROLL_BACKWARD:
			    /* マウスが上にドラッグされた？ */
				if (eventrec->mouse_y < eventrec->old_mouse_y) {
					pat = PAT_SCROLL_FB;
					type = EVENT_SCROLL_FASTBACKWARD;
				} else {
					pat = PAT_SCROLL_B;
					type = EVENT_SCROLL_BACKWARD;
				}
				break;
			case EVENT_SCROLL_FASTBACKWARD:
			    /* マウスが下にドラッグされた？ */
				if (eventrec->mouse_y > eventrec->old_mouse_y) {
					pat = PAT_SCROLL_B;
					type = EVENT_SCROLL_BACKWARD;
				} else {
					pat = PAT_SCROLL_FB;
					type = EVENT_SCROLL_FASTBACKWARD;
				}
				break;
			default:
				pat = PAT_SCROLL_B;
				type = EVENT_SCROLL_BACKWARD;
				break;
			}
		}
		break;

	case MSLRDOWN:
		type = EVENT_RETURN;
		pat = PAT_RET;
		break;
	}

	eventrec->type = type;
	eventrec->mouse_pat = pat;

	if (eventrec->old_mouse_pat != pat)
		_iocs_ms_sel (pat);

	return;
}



/* キーを読んでイベントを返す */
static void CheckKey (EVENTREC * eventrec)
{
	char type = EVENT_IDLE;
	int k;

#define KEY_BREAK	0x02
#define KEY_F1		0x08
#define KEY_F2		0x10
#define KEY_F3		0x20
	k = _iocs_bitsns (0x0c);
	if (k & KEY_BREAK)
		type = EVENT_QUIT;
	if (k & KEY_F1) {
		if (eventrec->shift)
			type = EVENT_EDIT;
		else
			type = EVENT_PAGE_END;
	}
	if (k & KEY_F2)
		type = EVENT_PAGE_TOP;
	if (k & KEY_F3)
		type = EVENT_DUMP;

#define KEY_CLR		0x80
#define KEY_DOWN	0x40
#define KEY_RIGHT	0x20
#define KEY_UP		0x10
#define KEY_LEFT	0x08
#define KEY_UNDO	0x04
#define KEY_ROLLDOWN	0x02
#define KEY_ROLLUP		0x01
	k = _iocs_bitsns (7);
	if (k & KEY_LEFT)
		type = EVENT_SCROLL_BACKWARD;
	if (k & KEY_UP)
		type = EVENT_SCROLL_FASTBACKWARD;
	if (k & KEY_RIGHT)
		type = EVENT_SCROLL_FORWARD;
	if (k & KEY_DOWN)
		type = EVENT_SCROLL_FASTFORWARD;
	if (k & KEY_ROLLUP)
		type = EVENT_PAGE_FORWARD;
	if (k & KEY_ROLLDOWN)
		type = EVENT_PAGE_BACKWARD;

#define KEY_HOME	0x40
	k = _iocs_bitsns (6);
	if (k & KEY_HOME)
		type = EVENT_ADDRESSBOOK;

#define KEY_TOUROKU	0x08
#define KEY_XF1		0x20
	k = _iocs_bitsns (0x0a);
	if (k & KEY_TOUROKU)
		type = EVENT_TOUROKU;

#define KEY_ESC		0x02
#define KEY_1		0x04
	k = _iocs_bitsns (0);
	if (k & KEY_1) {
		if (eventrec->shift)
			type = EVENT_SHELL;
	}
	eventrec->type = type;

	return;
}



/* イベントを取得する */
static void GetEvent (void)
{
	unsigned int ms_pos, ms_getdt;

#define MS_BUTTON_LEFT	0xff00
#define MS_BUTTON_RIGHT	0x00ff

	WaitVdisp ();

	ms_pos = _iocs_ms_curgt ();
	ms_getdt = _iocs_ms_getdt ();

	eventrec->old_mouse_x = eventrec->mouse_x;
	eventrec->old_mouse_y = eventrec->mouse_y;
	eventrec->old_mouse_pat = eventrec->mouse_pat;
	eventrec->mouse_x = (ms_pos >> 16);
	eventrec->mouse_y = (ms_pos & 0xffff);

	{
		short b = 0;
		int k;

		if (ms_getdt & MS_BUTTON_LEFT)
			b += MSLDOWN;
		if (ms_getdt & MS_BUTTON_RIGHT)
			b += MSRDOWN;
		eventrec->mouse_button = b;

	    /* [SHIFT]/[CTRL] キーに関しては常にチェック */
#define KEY_SHIFT		0x01
#define KEY_CTRL		0x02
		k = _iocs_bitsns (0x0e);
		eventrec->shift = (k & KEY_SHIFT);
		eventrec->ctrl = (k & KEY_CTRL);
	}

	if (scbar_drag) {
		if (eventrec->mouse_button == MSLDOWN) {
			if (eventrec->mouse_y != eventrec->old_mouse_y) {
				eventrec->check_xptext->current_line = CalcScbarLine(eventrec->mouse_y , eventrec->check_xptext->line - DISP_Y);
				eventrec->type = EVENT_PAGE_JUMP;
				return;
			}else{
				eventrec->type = EVENT_IDLE;
				return;
			}
		} else {
			scbar_drag = 0;
		}
	}

    /* マウスのＸ座標によって分岐 */
#define SCBAR_X	(512+16)	/* スクロールバーの左端Ｘ座標 */
#define PANEL_X	(512+16+16)	/* パネルの左端Ｘ座標 */
	if (eventrec->mouse_x < SCBAR_X) {
		CheckMouseOnText (eventrec);
	} else {
		if (eventrec->mouse_x < PANEL_X)
			CheckMouseOnScbar (eventrec);
		else
			CheckMouseOnPanel (eventrec);
	}

	if (eventrec->type == EVENT_IDLE)
		CheckKey (eventrec);	/* マウス操作がなかった時だけキーをチェック */

	return;
}



/* HTTPFILE を入れ換える */
static void SwapHttpfile (HTTPFILE ** h1, HTTPFILE ** h2)
{
	HTTPFILE *ht = *h1;

	*h1 = *h2;
	*h2 = ht;
}



/* メインルーチン : 終了するまでここでループ */
static int ShowHtml (char *in_url)
{
	HTTPFILE _httpfile[2];
	HTTPFILE *httpfile = &_httpfile[0], *old_httpfile = &_httpfile[1];

	int temp_line = 0;
	unsigned short save_counter = 0;
	char quit_all = 0;	/* 非０でプログラム終了 */

	InitHttpfile (httpfile);
	InitHttpfile (old_httpfile);
	CatHttpfile (httpfile, NULL, in_url);

    /* httpfile->url を解析して表示する */
	do {
		char event_exit = 0;
		char main_error = MAIN_ERROR_NON;
		char load_complete;
		char refresh_counter = 0;	/* refresh_rate を越えるごとに再整形 */
		char idle_rate = 0;	/* IDLE イベントを処理する間隔 */
		struct iocs_time lap_time;
		int gf_ret;	/* GetFile() の返り値 */

		McPrintf ("%s を読み込みます\n", httpfile->url);
		gf_ret = GetFile (httpfile);
		if (gf_ret != GF_SUCCESS) {
			McDbPuts ("ShowHtml() : 読み込めませんでした\n");
			main_error = MAIN_ERROR_GETFILE;
		} else {
			char recheck_flag = !0;

			McDbPuts ("● １パス目\n");
			lap_time = _iocs_ontime ();
			refresh_counter = 0;

			main_error = MAIN_ERROR_XPTEXT;		/* 成功すれば _SUCCESS に */

		    /* <A HREF> で .GIF/.JPG が指定された時は */
		    /* .HTM ファイルをメモリ上に作成 */
			if ((!strcmp (httpfile->content_type, "image/gif")) ||
			    (!strcmp (httpfile->content_type, "image/jpeg"))) {
				char temp_content[1024];

				recheck_flag = 0;
				free (httpfile->content);		/* 読んだ画像は捨てる */
				httpfile->content = NULL;
				sprintf (temp_content, "<HTML><HEAD><TITLE>%s</TITLE></HEAD><BODY><BR><CENTER><IMG SRC=\"%s\"></CENTER></BODY></HTML>\r\n\0",
					 httpfile->fname, httpfile->url);
				httpfile->content_length = strlen (temp_content);
			    /* まだこのエラーチェック正しくない */
				httpfile->content = malloc (httpfile->content_length);
				if ((int) httpfile->content == 0) {
					McPuts ("※ メモリが足りません（ HTTPFILE 用のメモリが確保できません）\n");
				}
				strcpy (httpfile->content_type, "text/html");
				strcpy (httpfile->content, temp_content);
				main_error = MAIN_ERROR_SUCCESS;
			}
		    /* <A HREF> で .DOC/.TXT が指定された時 */
			if (!strcmp (httpfile->content_type, "text/plain")) {
				McDbPuts ("テキスト整形中...\n");
				recheck_flag = 0;
				if (Html2Sjis (httpfile) < 0) {
				    /* エラー処理まだ */
					McPuts ("※ SJIS への変換に失敗しました\n");
				}
				if ((httpfile->xptext = Plain2Xpression (httpfile, NULL)) == NULL)
					McPuts ("※ テキストの整形に失敗しました\n");
				else
					main_error = MAIN_ERROR_SUCCESS;
				McDbPuts ("整形終了\n");
			}
		    /* <A HREF> で .HTM が指定された時 */
			if (!strcmp (httpfile->content_type, "text/html")) {
				McDbPuts ("テキスト整形中...\n");
				recheck_flag = 0;
				if (Html2Sjis (httpfile) < 0) {
				    /* エラー処理まだ */
					McPuts ("※ SJIS への変換に失敗しました\n");
				}
				if ((httpfile->xptext = Html2Xpression (httpfile)) == NULL)
					McPuts ("※ テキストの整形に失敗しました\n");
				else
					main_error = MAIN_ERROR_SUCCESS;
				McDbPuts ("整形終了\n");
			}
		    /* .HTM や .DOC/.TXT 以外を読み込んだ場合（.Lzh 等） */
			if ((main_error == MAIN_ERROR_XPTEXT) && (recheck_flag)
			    && (old_httpfile->xptext != NULL)) {
				ReCheckLinkTable (old_httpfile);
				McDbPuts ("ここまでもＯＫ\n");
				main_error = MAIN_ERROR_RECHECK;
			}
		}

		_dos_c_locate (0, 30);

		switch (main_error) {
		case MAIN_ERROR_GETFILE:
			if (old_httpfile->xptext == NULL) {
				return (-1);	/* 表示できるテキストが全くない */
			} else {
				SwapHttpfile (&httpfile, &old_httpfile);
				temp_line = httpfile->xptext->current_line;	/* 表示する行数 */
			}
			break;
		case MAIN_ERROR_XPTEXT:
		case MAIN_ERROR_RECHECK:
			free (httpfile->content);
			httpfile->content = NULL;
			if (old_httpfile->xptext == NULL) {
				return (-1);	/* 表示できるテキストが全くない */
			} else {
				InitHttpfile (httpfile);
				SwapHttpfile (&httpfile, &old_httpfile);
				temp_line = httpfile->xptext->current_line;	/* 表示する行数 */
			}
			break;
		case MAIN_ERROR_SUCCESS:
		default:
			break;
		}

	    /* old_httpfile を捨てる */
	    /* マウスカーソルの下にリンクがあるかどうかチェックするのを一瞬禁止 */
		if (old_httpfile->xptext) {
			FreeXptext2 (old_httpfile->xptext);
			FreeXptext (old_httpfile->xptext);
			old_httpfile->xptext = NULL;
		}
		if (old_httpfile->content) {
			free (old_httpfile->content);
			old_httpfile->content = NULL;
		}
		InitHttpfile (old_httpfile);
		eventrec->check_xptext = httpfile->xptext;

		if (++save_counter > webcache_save) {
			WCSave ();
			save_counter = 0;
		}
		httpfile->xptext->current_line = temp_line;	/* 表示する行数 */
		DrawTextAll (httpfile->xptext);
		load_complete = LI_CONTINUE_NOT_LOAD;

		_dos_c_locate (0, 30);

	    /* イベントループ */
		do {
			GetEvent ();	/* イベントを取得 */

			switch (eventrec->type) {
			case EVENT_IDLE:	/* 操作なし */
#define IDLE_RATE_MAX	10
				if ((load_complete >= LI_CONTINUE_NOT_LOAD) && (idle_rate++ > IDLE_RATE_MAX)) {
					McDbPuts ("● ２パス目\n");
					idle_rate = 0;
					load_complete = LoadImage (httpfile);
					switch (load_complete) {
					case LI_COMPLETE_NOT_LOAD:
						if (t_option) {
							struct iocs_time lap_time2 = _iocs_ontime ();
							McPrintf ("描画時間 = %d\n", lap_time2.sec - lap_time.sec);
						}
						McPuts (STR_DISP_COMPLETE);
						break;
					case LI_COMPLETE_LOAD:
						if (t_option) {
							struct iocs_time lap_time2 = _iocs_ontime ();
							McPrintf ("描画時間 = %d\n", lap_time2.sec - lap_time.sec);
						}
						McPuts (STR_DISP_COMPLETE);
						refresh_counter++;	/* 強制的に再整形 */
						save_counter++;
						break;
					case LI_CONTINUE_NOT_LOAD:
						break;
					case LI_CONTINUE_LOAD:
						refresh_counter++;
						save_counter++;
						break;
					}

					if ((refresh_counter >= refresh_rate)
					    || ((load_complete <= LI_COMPLETE_LOAD) && (refresh_counter > 0))) {
						XPTEXT *old_xptext = httpfile->xptext;

						McDbPuts ("テキスト再整形中...\n");
						refresh_counter = 0;
						if ((httpfile->xptext = Html2Xpression (httpfile)) == NULL) {
						    /* エラー処理まだ */
							McPuts ("再整形失敗\n");
						} else {
							McDbPuts ("再整形終了\n");
						}
						FreeXptext (old_xptext);
						DrawTextAll (httpfile->xptext);

						eventrec->check_xptext = httpfile->xptext;
					}
					if (save_counter > webcache_save) {
						WCSave ();
						save_counter = 0;
					}
				}
				break;

			case EVENT_SCROLL_FORWARD:
				ScrollForward (httpfile->xptext);
				break;

			case EVENT_SCROLL_BACKWARD:
				ScrollBackward (httpfile->xptext);
				break;

			case EVENT_SCROLL_FASTFORWARD:
				ScrollFastForward (httpfile->xptext);
				break;

			case EVENT_SCROLL_FASTBACKWARD:
				ScrollFastBackward (httpfile->xptext);
				break;

			case EVENT_PAGE_FORWARD:
				PageForward (httpfile->xptext);
				break;

			case EVENT_PAGE_BACKWARD:
				PageBackward (httpfile->xptext);
				break;

			case EVENT_PAGE_TOP:
				PageTop (httpfile->xptext);
				break;

			case EVENT_PAGE_END:
				PageEnd (httpfile->xptext);
				break;

			case EVENT_PAGE_JUMP:
				PageJump (httpfile->xptext);
				break;

			case EVENT_LINK:
				{
				    /* 指定された URL（フルパスでないかもしれない） */
					char *new_url = (httpfile->xptext->link_table)[eventrec->link_num].url;
#if	0
					if (!load_complete) {
						image_list_ptr = image_list_top;
						load_complete = !0;
					}
#endif
				    /* 以下 httpfile->url に新しい URL を設定する処理 */
				    /* URL が "#foo" （ファイル名なし）か？ */
					if (*new_url == '#') {
						McDbPuts ("#foo です\n");
						httpfile->xptext->current_line = SearchAnchor (httpfile->xptext, new_url);	/* 表示する行数 */
						DrawTextAll (httpfile->xptext);
					} else {
					    /* 現在表示中の httpfile は old_httpfile へ */
						SwapHttpfile (&httpfile, &old_httpfile);

						InitHttpfile (httpfile);
						CatHttpfile (httpfile, old_httpfile, (old_httpfile->xptext->link_table)[eventrec->link_num].url);
						AddHistory (httpfile->url, old_httpfile->url, old_httpfile->xptext->current_line);
						strcpy (httpfile->referer, old_httpfile->url);
						event_exit = !0;
					}
					temp_line = 0;
					WaitReleaseAll ();
				}
				break;

			case EVENT_RETURN:
				{
					char *t;

					if ((t = BeforeHistory (httpfile->url, &temp_line)) != NULL) {
						event_exit = !0;
					    /* 現在表示中の httpfile は old_httpfile へ */
						SwapHttpfile (&httpfile, &old_httpfile);
						CatHttpfile (httpfile, NULL, t);
					}
					WaitReleaseAll ();
				}
				break;

			case EVENT_DUMP:
				if (d_option) {
					DumpXptext (httpfile->xptext);
					event_exit = !0;
					quit_all = !0;
				}
				WaitReleaseAll ();
				break;

			case EVENT_TOUROKU:	/* [登録] でアドレス帳に登録 */
				AddAddress (httpfile);
				WaitReleaseAll ();
				break;

			case EVENT_ADDRESSBOOK:	/* アドレス帳へジャンプ */
			    /* 現在表示中の httpfile は old_httpfile へ */
				SwapHttpfile (&httpfile, &old_httpfile);

				InitHttpfile (httpfile);
				CatHttpfile (httpfile, NULL, address_book);
				AddHistory (httpfile->url, old_httpfile->url, old_httpfile->xptext->current_line);
				event_exit = !0;

				temp_line = 0;
				WaitReleaseAll ();
				break;

			case EVENT_SET_CONFIG_COLOR:
				SetConfigColor ();
				WaitReleaseAll ();
				break;

			case EVENT_SET_HTML_COLOR:
				SetHtmlColor ();
				WaitReleaseAll ();
				break;

			case EVENT_EDIT:	/* [SHIFT]+[F1] でエディタを起動 */
				temp_line = 0;
				WaitReleaseAll ();
				Tini2 ();
				{
					char temp_str[256];
					sprintf (temp_str, &text_editor[0],
						 ((httpfile->xptext->line_ptr)[httpfile->xptext->current_line]).org_line);
					strcat (temp_str, " ");
					if (!strnicmp (httpfile->scheme, "file://", 7))
						strcat (temp_str, (httpfile->url + 7));		/* file:// を飛ばす */
					else
						strcat (temp_str, httpfile->url);
					_toslash (temp_str);
					system (temp_str);	/* テキストエディタ起動 */
				}
				Init2 ();
				WaitReleaseAll ();
				event_exit = !0;
				break;

			case EVENT_SHELL:	/* [SHIFT]+[1] でシェルを起動 */
				Tini2 ();
				system ("");	/* シェル起動 */
				Init2 ();
				DrawTextAll (httpfile->xptext);
				WaitReleaseAll ();
				break;

			default:
				McDbPuts ("ShowHtml() : eventrec->type が変です\n");
			    /* ここに break がないのに注意 */

			case EVENT_QUIT:
				event_exit = !0;
				quit_all = !0;
				break;
			}
		} while (!event_exit);
	} while (!quit_all);


	if (httpfile->xptext) {
		FreeXptext2 (httpfile->xptext);
		FreeXptext (httpfile->xptext);
		httpfile->xptext = NULL;
	}
	if (httpfile->content) {
		free (httpfile->content);
		httpfile->content = NULL;
	}
	return (0);
}



/* 起動時及び子プロセス起動後に呼ばれる初期化ルーチン */
static void Init2 (void)
{
	int i;
	int sp;
	int palet_table[] =
	{
		0, 1, 2, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
	};

	unsigned short crtc_data[8] =
	{
		0x89, 0x0e, 0x1c, 0x7c, 0x237, 0x5, 0x028, 0x228
	};
	unsigned short *crtc_r0 = (unsigned short *) 0xe80000;
	unsigned short *crtc_r20 = (unsigned short *) 0xe80028;
	unsigned short *vctrl_r1 = (unsigned short *) 0xe82400;
	unsigned short *vctrl_r2 = (unsigned short *) 0xe82500;
	unsigned short *vctrl_r3 = (unsigned short *) 0xe82600;
	unsigned short *p;
	struct iocs_patst *mouse_pat_table[8] =
	{
		&mouse_pat0, &mouse_pat1, &mouse_pat2, &mouse_pat3,
		&mouse_pat4, &mouse_pat5, &mouse_pat6, &mouse_pat7
	};

	_dos_c_width (5);	/* DOS レベルでは 512x512 65536 色モード */
	_iocs_g_clr_on ();
	_dos_c_curoff ();
	_dos_c_locate (0, 30);
	old_fnkmod = _dos_c_fnkmod (-1);
	_dos_c_fnkmod (3);
	_iocs_ms_init ();
	_iocs_skey_mod (0, 0, 0);
	for (i = 0; i < 8; i++)
		_iocs_ms_patst (i, mouse_pat_table[i]);
	_iocs_ms_sel (0);
	_iocs_ms_curon ();
	_iocs_ms_limit (7, 0, 768 - 8, 512);
	_iocs_tgusemd (0, 2);
	_iocs_tgusemd (1, 2);

	sp = _iocs_b_super (0);
	for (i = 0, p = crtc_r0; i < 8; i++)
		*p++ = crtc_data[i];
	*crtc_r20 = 0x0316;	/* 768x512 65536 色正方形モードに */
	*vctrl_r1 = 0x0003;
	*vctrl_r2 = 0x32e4;	/* 0x31e4 だとダメ？ */
	*vctrl_r3 = 0x002f;
	_iocs_b_super (sp);

	for (i = 0; i < 16; i++)
		_iocs_tpalet2 (i, (int) config_color[palet_table[i]]);
	DrawBack ();

	sp = _iocs_b_super (0);
	cut_disp (&WebPage2, (void *) (TEXTVRAM + (512 + 8 + 8) / 8 + 2 * 0x20000), 128);
	cut_disp (&WebPage3, (void *) (TEXTVRAM + (512 + 8 + 8) / 8 + 3 * 0x20000), 128);
	_iocs_b_super (sp);

	_dos_intvcs (0xfff1, InterruptAbort);	/* _CTRLVC */
	_dos_intvcs (0xfff2, InterruptAbort);	/* _ERRJVC */
}



/* 起動時に１度だけ呼ばれる初期化ルーチン */
static int Init (void)
{
	old_screen_mode = _dos_c_width (-1);
    //printf ("WebXpression 起動中...\n");
	GetFileInit ();
	McInit ();
    //printf ("WebCache.env を読み込んでいます\n");
	if (WCInit () < 0)
		return (-1);
	InitHtml2Xpression ();
	if (InitHistory () < 0)
		return (-1);
	Init2 ();

	return (0);
}



/* 終了時及び子プロセス起動前に呼ばれる終了ルーチン */
static void Tini2 (void)
{
	ClearText ();
	_iocs_tgusemd (0, 3);
	_iocs_tgusemd (1, 3);
	_iocs_skey_mod (-1, 0, 0);
	_iocs_ms_init ();
	_iocs_ms_curof ();
	_dos_c_width (old_screen_mode);
	_dos_kflushio (0xff);	/* キーバッファをクリア */
	_dos_c_fnkmod (old_fnkmod);
	_dos_c_curon ();
}



/* 終了時に１度だけ呼ばれる終了ルーチン */
static int Tini (void)
{
	Tini2 ();
	WCTini ();

	return (0);
}



int main (int argc, char *argv[])
{
	int i;
	int slash_flag = 0;
	char *fname = NULL;
	char *cnf_fname = "WebXpression.cnf";
	int exit_code = 0;
	char temp_fname[92 + 7];

	d_option = 0;

	{
		char *temp;

		temp = getenv ("SLASH");
		if ((temp != NULL) && (*temp == '/')) {
			slash_flag = 1;
		}
	}

	for (i = 1; i < argc; i++) {
		if (('-' == *argv[i]) || ((slash_flag == 0) && ('/' == *argv[i]))) {
			switch (*(argv[i] + 1)) {

			case 'c':
			case 'C':
				cnf_fname = argv[i] + 2;
				break;

			case 'd':
			case 'D':
				d_option = !0;
				break;

			case 't':
			case 'T':
				t_option = !0;
				break;

			default:
				usage ();
				return (-1);
			}
		} else {
			fname = argv[i];
		}
	}

	mypsp = _dos_getpdb ();
	strcpy (address_book, "file://");
	strcat (address_book, mypsp->exe_path);
	_addlastsep (address_book);
	strcat (address_book, "AddressBook.htm");
	_toslash (address_book + 7);

	if (fname == NULL) {
		fname = address_book;
	} else {
		if (strnicmp (fname, "http://", 7) != 0 && strnicmp (fname, "https://", 8) != 0) {
		    /* ローカルファイルなら */
			strcpy (temp_fname, "file://");
			if (!strnicmp (fname, "file://", 7))
				_fullpath (temp_fname + 7, fname + 7, 92);	/* フルパスに */
			else
				_fullpath (temp_fname + 7, fname, 92);	/* フルパスに */
			fname = temp_fname;
			_toslash (fname + 7);
			puts (fname);
		}
	}
	if (InitConfig (cnf_fname) < 0)
		return (-1);

	if (!Init ()) {
		McPuts ("──- Welcome to WebXpression ───\n");
		if (inetd_version < 0) {
			McPuts ("▼ TCP/IP ドライバが常駐していません\n"
				"ローカルファイル／WebCache ファイルのみ閲覧が可能です\n"
				"──────────────────\n");
		}
		exit_code = ShowHtml (fname);
	}
	Tini ();

	if (exit_code < 0) {
		printf ("\n%s が見つかりません\n", fname);
		return (-1);
	}
	return (0);
}
