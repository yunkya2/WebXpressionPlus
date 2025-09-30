/* GetFile.h */


#ifdef GLOBAL_DEFINE		/* グローバル変数の定義と宣言を１つにまとめるテク */
#define Extern			/* Extern をヌル文字列に置換 */
#else
#define Extern extern		/* Extern を extern に置換 */
#endif


/* グローバル変数 */

Extern int inetd_version;



/* AbortCheckGetFile() の返り値 */
enum {
	GF_SUCCESS = 0,
	GF_ABORT_ESC,
	GF_ABORT_BREAK,
	GF_ERROR,
};


/* 関数プロトタイプ宣言 */
int GetFileInit (void);
int GetFile (HTTPFILE *);
