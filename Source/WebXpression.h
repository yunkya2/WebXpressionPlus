/* WebXpression.h */
/* 注：WebXpression.inc と同内容にすること！ */


/* 行管理テーブル */
typedef struct {
	char *ptr;		/* テキストへのポインタ */
	unsigned short start_dot;	/* 左端から何ドット目から表示開始するか */
	unsigned short org_line;	/* 元の HTML の何行目だったか（エディタ起動用） */
	char font_size;		/* サイズ */
	char font_type;		/* =0 なら標準フォント */
	char font_decoration;	/* 文字装飾 */
	char dammy;	/* ダミー */
} LINE_PTR;


/* リンクテーブル構造体 */
typedef struct {
	char *url;
	char in_cache;		/* = !0 : キャッシュに存在する */
	char dammy;
} LINK_TABLE;


#define SIZE_OF_ANCHOR	32
/* アンカーテーブル構造体 */
typedef struct {
	int line;		/* アンカーの存在する行 */
	char anchor[SIZE_OF_ANCHOR + 1];
	char dammy;
} ANCHOR_TABLE;


/* イメージリスト構造体 */
typedef struct _image_list {
	struct _image_list *next_ptr;
	struct _image_list *before_ptr;
	short count;		/* リンクカウント */
    /* （いくつのイメージテーブル構造体からリンクされているか） */
	unsigned short x;
	unsigned short y;
	void *data;		/* 展開したイメージ本体へのポインタ */
    /* = 0 : まだ読み込んでいない */
    /* = !0 : 読み込めなかった */
    /* = それ以外 : イメージへのポインタ */
	char url[256];
} IMAGE_LIST;


/* イメージテーブル構造体 */
typedef struct {
	IMAGE_LIST *image_list;
	unsigned short disp_x;	/* 表示する x サイズ (x<=512) */
	char in_cache;		/* = !0 : キャッシュに存在する */
	char dammy;
} IMAGE_TABLE;


/* Xpression 形式テキスト管理構造体 */
typedef struct {
	int filesize;
	LINE_PTR *line_ptr;
	char *text;		/* 整形後のテキスト */
	int line;		/* 整形後の行数 */
	int current_line;	/* 現在表示している行数 */
	LINK_TABLE *link_table;	/* リンクを構造体で管理する */
	short link_table_max;
	char *link_table_buffer;
	IMAGE_TABLE *image_table;
	short image_table_max;
	ANCHOR_TABLE *anchor_table;	/* アンカーを構造体で管理する */
	short anchor_table_max;
	char title[64+1];
} XPTEXT;


/* イベントフラグ */
enum {
	EVENT_IDLE = 0,
	EVENT_SCROLL_FORWARD,
	EVENT_SCROLL_BACKWARD,
	EVENT_SCROLL_FASTFORWARD,
	EVENT_SCROLL_FASTBACKWARD,
	EVENT_PAGE_FORWARD,
	EVENT_PAGE_BACKWARD,
	EVENT_PAGE_TOP,
	EVENT_PAGE_END,
	EVENT_PAGE_JUMP,
	EVENT_LINK,
	EVENT_RETURN,
	EVENT_TOUROKU,
	EVENT_ADDRESSBOOK,
	EVENT_DUMP,
	EVENT_SHELL,
	EVENT_SET_CONFIG_COLOR,
	EVENT_SET_HTML_COLOR,
	EVENT_EDIT,
	EVENT_QUIT,
};


/* イベント構造体 */
typedef struct {
	char type;		/* イベントの種類 */
	char type2;		/* イベントの種類その２ */
	char shift;		/* = 非0: [SHIFT]キーが押されている */
	char ctrl;		/* = 非0: [CTRL]キーが押されている */
	unsigned short mouse_x;	/* マウスカーソルの座標 */
	unsigned short mouse_y;
	unsigned short mouse_button;
	unsigned short mouse_pat;
	unsigned short old_mouse_x;	/* 前のマウスカーソルの座標 */
	unsigned short old_mouse_y;
	unsigned short old_mouse_pat;
	signed short link_num;	/* リンク番号 */
	XPTEXT *check_xptext;	/* 表示中の xptext */
	int keycode;		/* キーコード */
} EVENTREC;



#ifdef GLOBAL_DEFINE		/* グローバル変数の定義と宣言を１つにまとめるテク */
#define Extern			/* Extern をヌル文字列に置換 */
#else
#define Extern extern		/* Extern を extern に置換 */
#endif


/* グローバル変数 */

Extern struct dos_psp *mypsp;
Extern unsigned char d_option;


/* 関数プロトタイプ宣言 */
void WaitReleaseAll (void);

#include "supplementary.h"
