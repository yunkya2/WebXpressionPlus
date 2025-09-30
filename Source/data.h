/* Data.h */


enum {
	PAT_NO_SCROLL = 0,	/* 上下の矢印 */
	PAT_POINTER,		/* 矢印 */
	PAT_SCROLL_F,		/* 正方向スクロール */
	PAT_SCROLL_B,		/* 逆方向スクロール */
	PAT_SCROLL_FF,		/* 正方向高速スクロール */
	PAT_SCROLL_FB,		/* 逆方向高速スクロール */
	PAT_RET,		/* 両クリックで戻る */
	PAT_STOP,		/* 移動不可能 */
};


extern char WebPage2;
extern char WebPage3;
extern struct _patst mouse_pat0, mouse_pat1, mouse_pat2, mouse_pat3
 ,mouse_pat4, mouse_pat5, mouse_pat6, mouse_pat7;
