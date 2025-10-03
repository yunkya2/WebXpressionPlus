#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "WebXpression.h"
#include "Httpfile.h"
#include "MicroConsole.h"



/* HTTPFILE を初期化する */
void InitHttpfile (HTTPFILE * h1)
{
	h1->url[0] = '\0';
	h1->scheme[0] = '\0';
	h1->hostname[0] = '\0';
	h1->path[0] = '\0';
	h1->fname[0] = '\0';
	h1->query[0] = '\0';
	h1->anchor[0] = '\0';

	h1->content_length = 0;
	h1->content_type[0] = '\0';
	h1->content = NULL;
	h1->xptext = NULL;
	h1->referer[0] = '\0';
}


#if	0
/* HTTPFILE をコピーする */
void CopyHttpfile (HTTPFILE * h1, HTTPFILE * h2)
{
	strcpy (h1->url, h2->url);
	strcpy (h1->scheme, h2->scheme);
	strcpy (h1->hostname, h2->hostname);
	strcpy (h1->path, h2->path);
	strcpy (h1->fname, h2->fname);
	strcpy (h1->query, h2->query);
	strcpy (h1->anchor, h2->anchor);

	h1->content_length = h2->content_length;
	strcpy (h1->content_type, h2->content_type);
	h1->content = h2->content;
	h1->xptext = h2->xptext;
}

#endif


/*
   URL を結合する
   h2 が現在の URL, h3 がクリックされた URL, h1 に値が返る
   h1 は InitHttpfile() で初期化しておくこと
   "./" や "../" もしっかり処理する
   例１）
   h2->url = http://www.mankai.co.jp/index.htm
   h3->url = mitsuky/index.htm の時
   h1->url = http://www.mankai.co.jp/mitsuky/index.htm が返る
   例２）
   h2->url = http://www.mankai.co.jp/mitsuky/index.htm
   h3->url = ../index.htm の時
   h1->url = http://www.mankai.co.jp/index.htm が返る
   例３）
   h2->url = http://www.mankai.co.jp/index.htm
   h3->url = http://www.softbank.co.jp/dosvstart/netx.htm の時
   h1->url = http://www.softbank.co.jp/dosvstart/netx.htm が返る
 */
void CatHttpfile (HTTPFILE * h1, HTTPFILE * h2, char *url)
{
	char c, *s, *d;
	signed short slash = 0;	/* path 中の '/' の数 */
	char *fname_start_s;	/* fname の最初のコピー元 */
	char *path_end_d;	/* path の最後のコピー先 */
	char exit_flag = 0;
	char *scheme_str[] =
	{"http://", "file://"};
	enum {
		SCHEME_HTTP = 0,
		SCHEME_FILE,
	};
	short scheme_type;


	/* scheme によって分岐 */
	for (scheme_type = 0; scheme_type < sizeof (scheme_str) / sizeof (char *); scheme_type++) {
		if (!strnicmp (url, scheme_str[scheme_type], strlen (scheme_str[scheme_type])))
			break;
	}
	switch (scheme_type) {
	case SCHEME_HTTP:
		/* 絶対パス指定だった場合 */
		strcpy (h1->scheme, "http://");
		h1->port = 80;
		/* hostname をコピー */
		s = url + 7;
		d = h1->hostname;
		exit_flag = 0;
		do {
			c = *d++ = *s++;
			switch (c) {
			case '\0':
				--s;
				exit_flag = !0;
				break;
			case ':':
				while (*s++ != '/');	/* debug */
				--s;
				break;
			case '/':
				--s;
				*(d - 1) = '\0';
				exit_flag = !0;
				break;
			default:
				break;
			}
		} while (!exit_flag);
		d = h1->path;
		break;

	case SCHEME_FILE:
		strcpy (h1->scheme, "file://");
		s = url + 7;
		d = h1->path;
		break;

	default:		/* パス以降のみ */
#if	1
		if (h2 == NULL) {
			McPuts ("CatHttpfile() : url がフルパスでないのに h2 が NULL です"
				"（WebXpression のバグです\n");
		}
#endif
		strcpy (h1->scheme, h2->scheme);
		strcpy (h1->hostname, h2->hostname);
		h1->port = h2->port;
		/* まず h2->path を h1->path にコピー */
		s = h2->path;
		d = h1->path;
		for (;;) {
			c = *d++ = *s++;
			if (c == '\0')
				break;
			if (c == '/')
				slash++;	/* '/' の数を数える */
		}
		--d;
		s = url;
		break;
	}

	/* 次に url をコピー */
	fname_start_s = s;
	path_end_d = d;

	/* '/' で始まる URL の場合 */
	if (*s == '/') {
		slash = 0;
		path_end_d = d = h1->path;
	}
	exit_flag = 0;
	do {
		c = *d++ = *s++;
		switch (c) {
		case '\0':
			strcpy (h1->fname, fname_start_s);
			*path_end_d = '\0';
			exit_flag = !0;
			break;

		case '/':
			slash++;
			fname_start_s = s;
			path_end_d = d;
			break;

		case '.':
			if (*s == '/') {	/* "./" の場合 */
				s++;
				fname_start_s = s;
				--d;
				path_end_d = d;
			}
			if ((*s == '.') && (*(s + 1) == '/')) {		/* "../" の場合 */
				s += 2;
				fname_start_s = s;
				if (slash >= 2) {
					int i;
					d = h1->path;
					/* パスを逆昇る */
					for (i = 0; i < slash - 1; i++)
						while (*d++ != '/');
					path_end_d = d;
					--slash;
				} else {
					/* ここに来る時はパスがおかしい */
					/*（ルートディレクトリより上に行こうとしている） */
					path_end_d = d = h1->path + 1;	/* 一応ルートに設定 */
				}
			}
			break;

		case '?':
			/* strncpy() は末尾に 0 を付けてくれない事に注意 */
			strncpy (h1->fname, fname_start_s, (int) (s - fname_start_s - 1));
			h1->fname[s - fname_start_s - 1] = '\0';
			*path_end_d = '\0';
			d = h1->query;
			*d++ = '?';
			while (c = *d++ = *s++) {
				if (c == '#') {
					*(d - 1) = '\0';
					d = h1->anchor;
					strcpy (d, s);
				}
			}
			exit_flag = !0;
			break;

		case '#':
			/* strncpy() は末尾に 0 を付けてくれない事に注意 */
			strncpy (h1->fname, fname_start_s, (int) (s - fname_start_s));
			h1->fname[s - fname_start_s - 1] = '\0';
			*path_end_d = '\0';
			d = h1->anchor;
			*d++ = '#';
			strcpy (d, s);
			exit_flag = !0;
			break;

		default:
			break;
		}
	} while (!exit_flag);

	strcpy (h1->url, h1->scheme);
	strcat (h1->url, h1->hostname);
	if ((scheme_type == SCHEME_HTTP) && (h1->port != 80)) {
		char temp_str[7];
		sprintf (temp_str, ":%d", h1->port);
		strcat (h1->url, temp_str);
	}
	strcat (h1->url, h1->path);
	strcat (h1->url, h1->fname);
	if (*h1->query)
		strcat (h1->url, h1->query);
	if (*h1->anchor)
		strcat (h1->url, h1->anchor);

	return;

}



#ifdef DEMO

int main (int argc, char *argv[])
{
	HTTPFILE _h1, _h2, *h1 = &_h1, *h2 = &_h2;
	char url[256];

	InitHttpfile (h1);
	InitHttpfile (h2);

//      strcpy (h2->scheme, "http://");
	//      strcpy (h2->hostname, "www.mankai.co.jp");
	//      strcpy (h2->path, "/mitsuky/");
	//      strcpy (h2->fname, "index.htm");

//      strcpy (url, "IMG/9907.GIF");
	strcpy (url, "file://c:/source/index.htm");

	CatHttpfile (h1, h2, url);

	printf (
		       "url      = %s\n"
		       "scheme   = %s\n"
		       "hostname = %s\n"
		       "path     = %s\n"
		       "fname    = %s\n"
		       "query    = %s\n"
		       "anchor   = %s\n"
		       "port     = %d\n"
		       ,h1->url, h1->scheme, h1->hostname, h1->path, h1->fname, h1->query, h1->anchor, h1->port
		);

	return (0);
}

#endif
