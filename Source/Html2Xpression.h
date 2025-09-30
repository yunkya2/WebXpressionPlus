/* Html2Xpression.h */

#ifdef GLOBAL_DEFINE		/* グローバル変数の定義と宣言を１つにまとめるテク */
#define Extern			/* Extern をヌル文字列に置換 */
#else
#define Extern extern		/* Extern を extern に置換 */
#endif


/* グローバル変数 */
Extern unsigned short html_color[8];	/* HTML 中で指定された文字色 */



/* 関数プロトタイプ宣言 */
void InitHtml2Xpression (void);
int Html2Sjis (HTTPFILE *);
XPTEXT *Html2Xpression (HTTPFILE *);
XPTEXT *Plain2Xpression (HTTPFILE *, XPTEXT *);
void ReCheckLinkTable (HTTPFILE *);
int SearchAnchor (XPTEXT *, char *);
void FreeXptext (XPTEXT *);
void FreeXptext2 (XPTEXT *);
