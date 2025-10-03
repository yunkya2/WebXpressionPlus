/* Config.h */

#ifdef GLOBAL_DEFINE		/* グローバル変数の定義と宣言を１つにまとめるテク */
#define Extern			/* Extern をヌル文字列に置換 */
#else
#define Extern extern		/* Extern を extern に置換 */
#endif


/* グローバル変数 */

/* WebCache.cnf で設定する値 */
Extern unsigned char check_local_link;
Extern unsigned char hold_online;
Extern unsigned char image_compress;
Extern unsigned char image_quality;
Extern unsigned char key_repeat_1st;
Extern unsigned char key_repeat_2nd;
Extern unsigned char color_mode;
Extern unsigned char refresh_rate;
Extern unsigned short cache_image;
Extern unsigned short webcache_save;
Extern unsigned short history_max;
Extern unsigned short config_color[7];
Extern int line_table_size;
Extern int link_table_size;
Extern int link_table_buffer_size;
Extern int image_table_size;
Extern int anchor_table_size;
Extern char text_editor[64];


/* 関数プロトタイプ宣言 */
int InitConfig (char *);
