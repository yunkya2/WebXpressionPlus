/* WebCache.h */

/* WCExist の返り値 */
enum {
	WC_NON = 0,		/* キャッシュに存在しない */
	WC_INCACHE,		/* 〃    に存在し、起動後初めてのアクセス */
	WC_INCACHE2,		/* 〃    に存在し、起動後１回以上アクセスしている */
	WC_LOCAL		/* ローカルファイルとして存在する（file:// 時） */
};


#ifdef GLOBAL_DEFINE		/* グローバル変数の定義と宣言を１つにまとめるテク */
#define Extern			/* Extern をヌル文字列に置換 */
#else
#define Extern extern		/* Extern を extern に置換 */
#endif


/* グローバル変数 */



/* 関数プロトタイプ宣言 */
int WCInit (void);
int WCExist (HTTPFILE *, char *);
int WCSetAccess (HTTPFILE *);
int WCInsertUrl (HTTPFILE *, char *);
int WCDeleteUrl (HTTPFILE *);
int WCSave (void);
int WCTini (void);
