#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WEBXPRESSION
extern unsigned char check_local_link;
#include "WebXpression.h"
#include "MicroConsole.h"
#endif
#ifdef ROBOGET
#include "../RoboGet.h"
#endif
#ifdef WEBCM
#include "../WebCM.h"
#endif

#include "Httpfile.h"
#include "WebCache.h"


/* URL とテンポラリファイル名の対応構造体 */
typedef struct _temptable {
	struct _temptable *next;	/* 片方向リスト */
	char hash;		/* ハッシュ値 */
	char type;		/* Content-Type */
	int number;		/* テンポラリファイル名（の数値部） */
	int length;		/* ファイルサイズ */
	short year;		/* 0...32767 */
	char mon;		/* 1...12 */
	char day;		/* 1...31 */
	char hour;		/* 0...23 */
	char min;		/* 0...59 */
	char sec;		/* 0...59 */
	char access;		/* アクセスフラグ */
	char url[0];		/* url（GCC に依存した表記であることに注意） */
} TEMPTABLE;


static char *content_type_str[] =
{"text/html", "text/plain", "text/plain",
 "image/gif", "image/jpeg", "image/png", "image/bmp",
 "application/zip", "application/x-lzh", "application/x-gzip",
 "application/pdf",
 "application/octet-stream", "unknown/unknown"};
static char *ext_type_str[] =
{"HTM", "TXT", "DOC",
 "GIF", "JPG", "PNG", "BMP",
 "ZIP", "LZH", "TGZ",
 "PDF",
 "DAT", "DAT"};


static TEMPTABLE *tt_top = NULL, *tt_end = NULL;
static int cache_sum = 0;	/* 現在キャッシュしているファイル数 */
static int abs_max = 0;		/* Cxxxxxxx の最大値+1 */
static int abs_min = -1;	/* Cxxxxxxx の最小値 */
static char cachedir[256];	/* 環境変数 WEBCACHE */
static char cache_changed;	/* キャッシュの内容を１回でも変更したか？ */


/* url からハッシュ値を求める */
/* 要するに文字列から一意な数値が得られればいいので簡単なアルゴリズムで */
static char Url2Hash (char *url)
{
	char c = 0, t, *p = url;

	while (t = *p++) {
		if (t == '#')	/* anchor は無視 */
			break;
		if ((t >= 'A') && (t <= 'Z'))	/* 念のため大文字化 */
			t |= 0x20;
		c ^= t;		/* xor */
	}

	return (c);
}


/*
   同一の URL のファイルかどうか調べる
   アンカー '#' 以降は無視するところが strcmp と違う
   "http://www.mankai.co.jp/index.html" と
   "http://www.mankai.co.jp/index.html#whatsnew" を同一とみなすため
 */
static int IsSameUrlFile (char *url1, char *url2)
{
	char c, c2;
	char *p1 = url1, *p2 = url2;

	while (c = *p1++) {
		if (c == '#')	/* url1 のアンカーに達した */
			return (0);
		if (c != *p2++) {
			if (*(p2 - 1) == '#')
				return (0);
			else
				return (!0);
		}
	}
    /* url1 の末尾に達した */
	c2 = *p2++;
	if ((c2) && (c2 != '#'))
		return (!0);	/* この時点で url2 の末尾でなく、アンカーでもない場合 */
	return (0);
}


/* ノード用のメモリを確保 */
static TEMPTABLE *TTAlloc (char *url)
{
	return (malloc (sizeof (TEMPTABLE) + strlen (url) + 1));
}


/* ノードをリストの末尾に１つ追加 */
static void TTInsert (TEMPTABLE * tt)
{
	if (tt_end == NULL) {
	    /* ノードが１つもない場合 */
		tt_top = tt;
	} else {
		tt_end->next = tt;
	}
	tt_end = tt;
	tt->next = NULL;
	cache_changed = !0;
}


/* ノードを１つ削除 */
static void TTDelete (TEMPTABLE * tt, TEMPTABLE * tt_b)
{
	if (tt->next == NULL) {
		if (tt == tt_top) {
		    /* １つしかないノードを削除する場合 */
			tt_top = NULL;
			tt_end = NULL;
		} else {
		    /* 末尾のノードの場合 */
			tt_end = tt_b;
			tt_end->next = NULL;
		}
	} else {
		if (tt == tt_top) {
		    /* 先頭のノードを削除する場合 */
			tt_top = tt->next;
		} else {
			tt_b->next = tt->next;
		}
	}
	free (tt);
	cache_changed = !0;
}



int WCInit (void)
{
	FILE *fp;
	char temp_str[1024];

	{
		char *t;
		t = getenv ("WEBCACHE");
		if (t)
			strcpy (cachedir, t);
		else
			*cachedir = '\0';
	}

	strcpy (temp_str, cachedir);
	strcat (temp_str, "WebCache.env");
	if ((fp = fopen (temp_str, "r")) != NULL) {
		while (fgets (temp_str, 1024, fp) != NULL) {
			int year, mon, day, hour, min, sec;
			int number, length;
			char url[256], ext[4], type[39];
			TEMPTABLE *tt;
			char c;

		    /* WebCache.env は正常なファイルだと信用しきっています（汗） */
			sscanf (temp_str, "C%7d.%s %4d/%2d/%2d %2d:%2d:%2d %d %s %s\n",
				&number, ext, &year, &mon, &day, &hour, &min, &sec,
				&length, url, type);
			tt = TTAlloc (url);
			if (tt) {
				TTInsert (tt);
				tt->hash = Url2Hash (url);
				tt->number = number;
				tt->length = length;
				tt->year = year;
				tt->mon = mon;
				tt->day = day;
				tt->hour = hour;
				tt->min = min;
				tt->sec = sec;
				tt->access = 0;
				for (c = 0; c < sizeof (ext_type_str) / 4 - 1; c++) {
					if (!strcmp (type, content_type_str[c]))
						break;
				}
				tt->type = c;
				strcpy (tt->url, url);

				if (number > abs_max)
					abs_max = number;
				if (number < abs_min)
					abs_min = number;
				cache_sum++;
				abs_max++;
			} else {
				printf ("WebCache 用メモリが確保できません\n");
				fclose (fp);
				return (-1);
			}
		}
		fclose (fp);
	} else {
		if ((fp = fopen (temp_str, "w")) != NULL)
			fclose (fp);
	}
	cache_changed = 0;
	return (0);
}



/* httpfile->url で指定したファイルがキャッシュに存在するか */
/* 存在するなら httpfile に各種情報と cache_fname（フルパス）が返る */
int WCExist (HTTPFILE * httpfile, char *cache_fname)
{
	TEMPTABLE *tt = tt_top;
	short h;
	char *scheme_str[] =
	{"http://", "https://", "file://"};

    /* scheme によって分岐 */
	for (h = 0; h < sizeof (scheme_str) / sizeof (char *); h++) {
		if (!strnicmp (httpfile->url, scheme_str[h], strlen (scheme_str[h])))
			break;
	}
	switch (h) {
	case 0:		/* http:// */
	case 1:		/* https:// */
		{
			char hash;

			hash = Url2Hash (httpfile->url);
			while (tt) {
			    /* まずハッシュ値で高速に検索 */
				if ((tt->hash == hash)
				    && !IsSameUrlFile (tt->url, httpfile->url)) {
					sprintf (cache_fname, "%sC%07d.%s", cachedir, tt->number, ext_type_str[tt->type]);
					strcpy (httpfile->content_type, content_type_str[tt->type]);
					httpfile->content_length = tt->length;
					(httpfile->time_stamp).tm_year = tt->year - 1900;
					(httpfile->time_stamp).tm_mon = tt->mon - 1;
					(httpfile->time_stamp).tm_mday = tt->day;
					(httpfile->time_stamp).tm_hour = tt->hour;
					(httpfile->time_stamp).tm_min = tt->min;
					(httpfile->time_stamp).tm_sec = tt->sec;
				    /* 起動後にアクセスしたか？ */
					if (tt->access == 0)
						return (WC_INCACHE);
					else
						return (WC_INCACHE2);
				}
				tt = tt->next;
			}
		}
		break;
	case 2:		/* file:// */
	default:
		{
			FILE *fp = NULL;

			if (!strnicmp (httpfile->url, "file://", 7))
				strcpy (cache_fname, httpfile->url + 7);
			else
				strcpy (cache_fname, httpfile->url);

			if ((check_local_link == 0) || ((fp = fopen (cache_fname, "rb")) != NULL)) {
				char c;
				char *p;
				char ext[256];

				if (fp != NULL) {
					fseek (fp, 0, SEEK_END);
					httpfile->content_length = ftell (fp);
					fseek (fp, 0, SEEK_SET);
					fclose (fp);
				}
			    /* マルチピリオド非対応・・・ */
				ext[0] = '\0';
				p = &httpfile->url[7];
				while (c = *p++) {
					if (c == '.') {
						strcpy (ext, p);
						break;
					}
				}
				for (c = 0; c < sizeof (ext_type_str) / 4 - 1; c++) {
					if (!stricmp (ext, ext_type_str[c]))
						break;
				}
				strcpy (httpfile->content_type, content_type_str[c]);
				return (WC_LOCAL);
			} else {
				*cache_fname = '\0';
				httpfile->content_length = 0;
				*httpfile->content_type = '\0';
			}
		}
		break;
	}
	return (WC_NON);
}



/* アクセスフラグをセット */
int WCSetAccess (HTTPFILE * httpfile)
{
	TEMPTABLE *tt = tt_top;
	char hash;

	if (strcmp (httpfile->scheme, "http://") != 0 && strcmp (httpfile->scheme, "https://") != 0)
		return (-1);	/* http:// でなければ帰る */

	hash = Url2Hash (httpfile->url);
	while (tt) {
		if (tt->hash == hash) {		/* まずハッシュ値で高速に検索 */
			if (!IsSameUrlFile (tt->url, httpfile->url)) {
				tt->access = !0;
				return (0);
			}
		}
		tt = tt->next;
	}
	return (0);
}



/* httpfile->url で指定したファイルをキャッシュに登録する */
/* cache_fname（フルパス）が返る */
int WCInsertUrl (HTTPFILE * httpfile, char *cache_fname)
{
	char c;
	TEMPTABLE *tt;
	short h;
	char *scheme_str[] =
	{"http://", "https://", "file://"};

    /* scheme によって分岐 */
	for (h = 0; h < sizeof (scheme_str) / sizeof (char *); h++) {
		if (!strnicmp (httpfile->url, scheme_str[h], strlen (scheme_str[h])))
			break;
	}
	switch (h) {
	case 0:		/* http:// */
	case 1:		/* https:// */
		for (c = 0; c < sizeof (ext_type_str) / 4 - 1; c++) {
			if (!strcmp (httpfile->content_type, content_type_str[c]))
				break;
		}

		tt = TTAlloc (httpfile->url);
		if (tt == NULL) {
			McPuts ("※ メモリが足りません（WebCache 用のメモリが確保できません）\n");
			return (-1);	/* メモリ不足 */
		}
	    /* もし既にキャッシュに存在するならそれを削除 */
	    /* （ファイルが更新された時） */
		{
			char temp_cache_fname[256];
			HTTPFILE _t_httpfile, *t_httpfile = &_t_httpfile;	/* ヘッダが返る */

			InitHttpfile (t_httpfile);
			CatHttpfile (t_httpfile, NULL, httpfile->url);

			if (WCExist (t_httpfile, temp_cache_fname)) {
				McDbPrintf ("%s を削除します\n", temp_cache_fname);
				WCDeleteUrl (t_httpfile);
				remove (temp_cache_fname);
			}
		}
		TTInsert (tt);
		tt->hash = Url2Hash (httpfile->url);
		tt->type = c;
		tt->number = abs_max;
		tt->year = httpfile->time_stamp.tm_year + 1900;
		tt->mon = httpfile->time_stamp.tm_mon + 1;
		tt->day = httpfile->time_stamp.tm_mday;
		tt->hour = httpfile->time_stamp.tm_hour;
		tt->min = httpfile->time_stamp.tm_min;
		tt->sec = httpfile->time_stamp.tm_sec;
		tt->access = !0;
		tt->length = httpfile->content_length;
		strcpy (tt->url, httpfile->url);

		sprintf (cache_fname, "%sC%07d.%s", cachedir, tt->number, ext_type_str[tt->type]);
		cache_sum++;
		abs_max++;
		break;
	case 2:		/* file:// */
	default:
	    /* file:// の場合はキャッシュに登録せず cache_fname だけ返す */
		{
			FILE *fp;
			strcpy (cache_fname, httpfile->url + 7);

			if ((fp = fopen (cache_fname, "rb")) != NULL) {
				char c;
				char *p;
				char ext[256];

				fclose (fp);

			    /* マルチピリオド非対応・・・ */
				ext[0] = '\0';
				p = &httpfile->url[7];
				while (c = *p++) {
					if (c == '.') {
						strcpy (ext, p);
						break;
					}
				}
				for (c = 0; c < sizeof (ext_type_str) / 4 - 1; c++) {
					if (!stricmp (ext, ext_type_str[c]))
						break;
				}
				strcpy (httpfile->content_type, content_type_str[c]);
				return (!0);
			} else {
				*cache_fname = '\0';
				return (0);
			}
		}
		break;
	}
	return (0);
}



int WCDeleteUrl (HTTPFILE * httpfile)
{
	int r = -1;
	char h;
	TEMPTABLE *tt = tt_top, *tt_b = NULL;

	h = Url2Hash (httpfile->url);
	while (tt) {
		if (tt->hash == h) {
			if (!IsSameUrlFile (tt->url, httpfile->url)) {
				TTDelete (tt, tt_b);
				r = 0;
				break;
			}
		}
		tt_b = tt;	/* １つ前のノード */
		tt = tt->next;
	}
	return (r);
}



int WCSave (void)
{
	FILE *fp;
	char temp_fname[256];
	TEMPTABLE *tt = tt_top;

	if (!cache_changed)	/* キャッシュ内容に変更がない場合 */
		return (0);

    /* WebCache.env を更新 */
	strcpy (temp_fname, cachedir);
	strcat (temp_fname, "WebCache.env");
	if ((fp = fopen (temp_fname, "w")) != NULL) {
		while (tt) {
			int year, mon, day, hour, min, sec;
			year = tt->year;
			mon = tt->mon;
			day = tt->day;
			hour = tt->hour;
			min = tt->min;
			sec = tt->sec;

			fprintf (fp, "C%07d.%s %04d/%02d/%02d %02d:%02d:%02d %d\t%s\t%s\n",
				 tt->number, ext_type_str[tt->type],
				 year, mon, day, hour, min, sec,
				 tt->length, tt->url, content_type_str[tt->type]);
			tt = tt->next;
		}
		fclose (fp);
	} else {
		return (-1);
	}
	return (0);
}



int WCTini (void)
{
#if	0
	printf ("WebCache.env を保存しています\n");
#endif
	return (WCSave ());
}
