/* Httpfile.h */

#include <time.h>
#include <utime.h>

#define HTTPFILE_URL_MAX	255

/* HTTPFILE 管理構造体 */
typedef struct {
	char url[HTTPFILE_URL_MAX];
	char scheme[256];
	char hostname[256];
	char path[256];
	char fname[256];
	char query[256];
	char anchor[256];
	int port;
	struct tm time_stamp;	/* 最終更新日時(last_modified) */
	int content_length;
	char content_type[32];
	void *content;
	XPTEXT *xptext;
	char referer[256];
} HTTPFILE;



/* 関数プロトタイプ宣言 */
void InitHttpfile (HTTPFILE *);
void CatHttpfile (HTTPFILE *, HTTPFILE *, char *);
