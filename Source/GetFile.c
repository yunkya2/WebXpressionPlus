/* GetFile.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dos.h>
#include <sys/iocs.h>
#include <errno.h>

#include <network.h>
#include <socket.h>


#ifdef	WEBXPRESSION
#include "WebXpression.h"
#include "MicroConsole.h"
#endif

extern int AbortCheckGetFile (void);

#include "Httpfile.h"
#include "GetFile.h"
#include "WebCache.h"
#include "History.h"


extern void Date2Date (char *, struct tm *);

enum {
	REQ_HEAD,
	REQ_GET
};


int GetFileInit (void)
{
	inetd_version = _get_version ();

	return (0);
}


void HideMouse (void)
{
#ifdef	WEBXPRESSION
	_iocs_ms_curof ();
#endif
}

void ShowMouse (void)
{
#ifdef	WEBXPRESSION
	_iocs_ms_curon ();
#endif
}



/* URL で指定したファイルをネットワークから取ってくる */
static signed int GetFromNetwork (HTTPFILE * httpfile, char req_mode)
{
	int netd;
	struct sockaddr_in addr;
	struct hostent *h;
	char temp_str[1024];
	int ret;

	ret = AbortCheckGetFile ();
	if (ret != GF_SUCCESS)
		return (ret);

	ret = GF_ERROR;


    /* ソケットを作成する */
	netd = socket (AF_INET, SOCK_STREAM, 0);
	if (netd < 0) {
		McPuts ("※ ソケットが作成できませんでした\n");
		return (ret);
	}
	memset (&addr, 0, sizeof (addr));	/* 0 で埋める */

	addr.sin_family = AF_INET;	/* INETドメインを指定 */
	addr.sin_port = htons (httpfile->port);		/* http はポート 80番 */
    /* ホスト名(www.xxx.co.jp) を IP アドレス(int)に変換 */
	h = gethostbyname (httpfile->hostname);
	if (h == NULL) {
		McPuts ("※ドメイン名がみつかりません\n");
		return (ret);
	}
	addr.sin_addr.s_addr = *(long *) h->h_addr;

    /* 相手先に接続する */
	if (req_mode == REQ_HEAD)
		McDbPuts ("ヘッダを取得します");
	McPrintf ("%s に接続中...", httpfile->hostname);
	if (connect (netd, (char *) &addr, sizeof (addr)) < 0) {
		McPuts ("\n※ 接続に失敗しました\n");
		return (ret);
	}
	McCursorTop ();
	McPrintf ("%s に接続しました\n", httpfile->hostname);

	if (req_mode == REQ_HEAD)
		sprintf (temp_str, "HEAD %s%s%s HTTP/1.0\r\n", httpfile->path, httpfile->fname, httpfile->query);
	else
		sprintf (temp_str, "GET %s%s%s HTTP/1.0\r\n", httpfile->path, httpfile->fname, httpfile->query);
	write_s (netd, temp_str, strlen (temp_str));
	McDbPuts (temp_str);
    /* write_s (netd, "User-Agent: WebXpression / ver0.01 (X68000)\r\n", 48);    */
	strcpy (temp_str, "Accept: */*\r\n");
	write_s (netd, temp_str, strlen (temp_str));
	McDbPuts (temp_str);
	sprintf (temp_str, "Host: %s\r\n", httpfile->hostname);
	write_s (netd, temp_str, strlen (temp_str));
	McDbPuts (temp_str);
#if	1
	{
		if (strncmp ("file://", httpfile->referer, 7)) {
			sprintf (temp_str, "Referer: %s\r\n", httpfile->referer);
			write_s (netd, temp_str, strlen (temp_str));
			McDbPuts (temp_str);
		}
	}
#endif
	write_s (netd, "\r\n", 2);

	McPuts ("レスポンスを待ちます...");

    /* ヘッダを１行づつ読み込む */
	recvline (netd, temp_str, 1024);
	if ((strncmp (temp_str, "HTTP/1.0 200", 12))
	    && (strncmp (temp_str, "HTTP/1.1 200", 12))) {
		McDbPuts (temp_str);
		close_s (netd);	/* 接続の切断 */
		return (ret);
	}
	McCursorTop ();		/* "レスポンスを待ちます"を消去 */
	McDbPuts ("\n");

	*httpfile->content_type = '\0';
	httpfile->content_length = 0;
#define NO_TIMESTAMP	255
	(httpfile->time_stamp).tm_sec = NO_TIMESTAMP;

    /* 空行（ヘッダの終了）が来るまでループ */
	while (recvline (netd, temp_str, 1024), strlen (temp_str) > 3) {
		char temp_entity[256];

		McDbPrintf ("HEAD > %s", temp_str);
		sscanf (temp_str, "%s", temp_entity);
	    /* ヘッダ名は大文字／小文字を区別しない */
		if (!stricmp (temp_entity, "Content-Type:")) {
			sscanf (temp_str + 14, "%s", httpfile->content_type);
		}
		if (!stricmp (temp_entity, "Content-Length:")) {
			sscanf (temp_str + 16, "%d", &httpfile->content_length);
		}
		if (!stricmp (temp_entity, "Last-Modified:")) {
			Date2Date (temp_str + 15, &(httpfile->time_stamp));
		}
	}
	if (*httpfile->content_type == '\0')
		strcpy (httpfile->content_type, "application/octet-stream");


	if (req_mode == REQ_HEAD) {
		close_s (netd);	/* 接続の切断 */

		if ((httpfile->time_stamp).tm_sec == NO_TIMESTAMP) {
			time_t t;
			t = time (NULL);
			httpfile->time_stamp = *localtime (&t);
		}
		McCursorTop ();
		ret = GF_SUCCESS;
	} else {
		FILE *fp;
		int read_size = 0, s;
		void *r = NULL;
		int alloc_size, alloc_left;
		int lap_time;
		char cache_fname[256];

#define YOYUU	4096
		if (httpfile->content_length) {
			alloc_size = httpfile->content_length + YOYUU;
		} else {
		    /* 12MB 以上確保できないよーん */
			alloc_size = ((int) _dos_malloc (-1)) & 0x00fffffe - YOYUU;
		}
		if (((int) (httpfile->content = _dos_malloc (alloc_size))) < 0) {
			McPuts ("※ メモリが足りません\n");
			close_s (netd);		/* 接続の切断 */
			httpfile->content = NULL;
			return (ret);
		}
		r = httpfile->content;
		alloc_left = alloc_size;
		lap_time = _iocs_ontime ();

	    /* ファイルの転送が完了するまでループ */
		do {
			s = read_s (netd, r, alloc_left);
			read_size += s;
			r += s;
			if ((alloc_left -= s) <= 0) {
				McPuts ("※ メモリが足りません\n");
				break;
			}
			if (httpfile->content_length)
				sprintf (temp_str, "受信中 %d/%d バイト", read_size, httpfile->content_length);
			else
				sprintf (temp_str, "受信中 %d バイト", read_size);
			McCursorTop ();		/* カーソルを行の先頭に */
			McPuts (temp_str);
			ret = AbortCheckGetFile ();
		} while ((socklen (netd, 0) >= 0) && (ret == GF_SUCCESS));

		if (ret != GF_SUCCESS) {
			shutdown (netd, 0);	/* 受信したデータを受け取らず，すべて廃棄する */
			shutdown (netd, 2);	/* connection を abortする */
			WaitReleaseAll ();
		}
		close_s (netd);	/* 接続の切断 */
		lap_time -= _iocs_ontime ();
		McCursorTop ();	/* カーソルを行の先頭に */

#ifndef WEBXPRESSION
		McPrintf ("	受信終了（受信速度は %8.1f バイト／秒でした）", (double) read_size / (double) (-lap_time) * 100.0);
#endif

		if (httpfile->content_length == 0) {
			_dos_setblock (httpfile->content, read_size);
			httpfile->content_length = read_size;
			if (alloc_left <= 0) {
				_dos_mfree (httpfile->content);
				httpfile->content = NULL;
				return (ret);
			}
		}
		if (ret == GF_SUCCESS) {
			time_t t;
			if ((httpfile->time_stamp).tm_sec != NO_TIMESTAMP) {
				t = mktime (&httpfile->time_stamp);
			} else {
				t = time (NULL);
				httpfile->time_stamp = *localtime (&t);
			}
			WCInsertUrl (httpfile, cache_fname);
			if ((fp = fopen (cache_fname, "wb")) != NULL) {
				struct utimbuf u;
				fwrite (httpfile->content, sizeof (char), read_size, fp);
				fclose (fp);
				u.actime = t + 32400;
				u.modtime = t + 32400;
				utime (cache_fname, &u);
#ifndef WEBXPRESSION
				McPuts (" : ファイルを保存しました\n");
#endif
			} else {
				McPuts ("※ キャッシュを書き込めません\n");
			}
		} else {
			McPuts ("※ 中断しました\n");
			_dos_mfree (httpfile->content);
			httpfile->content = NULL;
		}
	}
	return (ret);
}



/* URL で指定したファイルをディスクから取ってくる */
static int GetFromLocal (HTTPFILE * httpfile)
{
	FILE *fp;
	char temp_fname[256], ext[256];
	int ret = GF_ERROR;
	/* 拡張子から content-type を得るのに必要 */
	char *content_type_str[] =
	{"text/html", "text/html", "text/html", "text/plain", "text/plain",
	 "image/gif", "image/jpeg", "image/jpeg", "image/png", "image/bmp",
	 "application/zip", "application/x-lzh", "application/x-gzip",
	 "application/pdf",
	 "application/octet-stream", "text/plain"};
	char *ext_type_str[] =
	{"HTM", "HTML", "CGI", "TXT", "DOC",
	 "GIF", "JPG", "JPEG", "PNG", "BMP",
	 "ZIP", "LZH", "TGZ",
	 "PDF",
	 "DAT", "*/*"};


//      sprintf (temp_fname, "%s%s%s", httpfile->hostname, httpfile->path, httpfile->fname);
	sprintf (temp_fname, "%s%s", httpfile->path, httpfile->fname);
	if ((fp = fopen (temp_fname, "rb")) != NULL) {
		int i;
		char c;
		char *p;

		fseek (fp, 0, SEEK_END);
		httpfile->content_length = ftell (fp);
		fseek (fp, 0, SEEK_SET);

		if ((int) (httpfile->content = _dos_malloc (httpfile->content_length)) > 0) {
			fread (httpfile->content, httpfile->content_length, 1, fp);
			fclose (fp);

		    /* マルチピリオド非対応・・・ */
			ext[0] = '\0';
			p = &httpfile->fname[0];
			while (c = *p++) {
				if (c == '.') {
					strcpy (ext, p);
					break;
				}
			}

			for (i = 0; i < sizeof (ext_type_str) / sizeof (char *) - 1; i++) {
				if (!stricmp (ext, ext_type_str[i]))
					break;
			}
			strcpy (httpfile->content_type, content_type_str[i]);
		    /* あ、タイムスタンプの処理まだ */

			ret = GF_SUCCESS;
		} else {
			McDbPuts ("GetFromLocal() : ");
			McPuts ("※ : メモリが足りません\n");
			httpfile->content = NULL;
		}
	} else {
		McDbPuts ("GetFromLocal() : ");
		McPrintf ("※ : ローカルファイル %s がオープンできません\n", httpfile->url);
	}

	return (ret);
}



/* URL で指定したファイルをキャッシュから取ってくる */
static int GetFromCache (HTTPFILE * httpfile, char *cache_fname)
{
	FILE *fp;
	int ret = GF_ERROR;

#ifndef WEBXPRESSION
	if (!strcmp (httpfile->content_type, "text/html"))
		return;
#endif
	if ((fp = fopen (cache_fname, "rb")) != NULL) {
		if ((int) (httpfile->content = _dos_malloc (httpfile->content_length)) > 0) {
			fread (httpfile->content, httpfile->content_length, sizeof (char), fp);
			ret = GF_SUCCESS;
		} else {
			McPuts ("※ メモリが足りません\n");
			httpfile->content = NULL;
		}
		fclose (fp);
	} else {
		McPrintf ("※ キャッシュファイル %s がオープンできません\n", cache_fname);
	}

	return (ret);
}



/* URL で指定したファイルを取ってくる */
/* httpfile 中の URLは分解ずみであること */
int GetFile (HTTPFILE * httpfile)
{
	char cache_fname[256];
	int ret = GF_ERROR;	/* 返り値 */

/*      McDbPrintf ("GetFile() : %s の受信を開始します\n", httpfile->url); */
	switch (WCExist (httpfile, cache_fname)) {
	case WC_NON:		/* キャッシュに存在しない場合 */
		if (inetd_version >= 0) {
		    /* TCP/IP ドライバが常駐している場合 */
			HideMouse ();
			ret = GetFromNetwork (httpfile, REQ_GET);
			ShowMouse ();
		} else {
			McPuts ("※ TCP/IP ドライバが常駐していません\n");
		}
		break;

	case WC_INCACHE:	/* キャッシュに存在し、起動後初めてのアクセスの場合 */
		WCSetAccess (httpfile);
		if (inetd_version >= 0) {
		    /* TCP/IP ドライバが常駐している場合 */
			HTTPFILE _t_httpfile, *t_httpfile = &_t_httpfile;	/* ヘッダが返る */

			InitHttpfile (t_httpfile);
			CatHttpfile (t_httpfile, httpfile, httpfile->url);
			strcpy (t_httpfile->referer, httpfile->referer);
			HideMouse ();
			McPuts ("更新チェックをします\n");
			ret = GetFromNetwork (t_httpfile, REQ_HEAD);	/* ヘッダを取得 */
			ShowMouse ();
			if (ret == GF_SUCCESS) {
			    /* ヘッダが取得出来た場合 */
				McDbPuts ("ヘッダ取得成功\n");
				if (difftime (mktime (&t_httpfile->time_stamp), mktime (&httpfile->time_stamp)) > (double) 0.0) {
				    /* ファイルが更新されていた場合 */
					McPuts ("ファイルが更新されています\n");
					HideMouse ();
					ret = GetFromNetwork (httpfile, REQ_GET);
					ShowMouse ();
				} else {
				    /* ファイルが更新されていない場合 */
					McPuts ("ファイルが更新されていません\n");
					ret = GetFromCache (httpfile, cache_fname);
				}
			} else {
			    /* ヘッダが取得できなかった場合 */
				McPuts ("更新チェックができませんでした\n");
				HideMouse ();
				ret = GetFromNetwork (httpfile, REQ_GET);
				ShowMouse ();
			}
		} else {
		    /* TCP/IP ドライバが常駐していない場合 */
			ret = GetFromCache (httpfile, cache_fname);
		}
		break;

	case WC_INCACHE2:	/* キャッシュに存在し、起動後１回以上アクセスしている場合 */
		ret = GetFromCache (httpfile, cache_fname);
		break;

	case WC_LOCAL:
		ret = GetFromLocal (httpfile);
		break;
	}

	return (ret);
}
