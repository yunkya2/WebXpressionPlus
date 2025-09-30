/* Image.h */

/* LoadImage の返り値 */
/* この順番に依存しているコードがあるので注意 */
enum {
    /* もう読み込む必要がない */
	LI_COMPLETE_NOT_LOAD = 0,	/* 読み込めなかった */
	LI_COMPLETE_LOAD,	/* １枚読み込んだ */

    /* まだ読み込む必要がある */
	LI_CONTINUE_NOT_LOAD,	/* 読み込めなかった */
	LI_CONTINUE_LOAD,	/* １枚読み込んだ */
};


/* 関数プロトタイプ宣言 */
void InitLoadImage (void);
void DispImageList (void);
IMAGE_LIST *InsertImageNode (HTTPFILE *);
IMAGE_LIST *SearchImageNode (HTTPFILE *);
int LoadImage (HTTPFILE *);
