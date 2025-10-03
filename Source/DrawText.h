/* DrawText.h */


/* 関数プロトタイプ宣言 */
int DrawTextAll (XPTEXT *);
void ScrollForward (XPTEXT *);
void ScrollBackward (XPTEXT *);
void ScrollFastForward (XPTEXT *);
void ScrollFastBackward (XPTEXT *);
void PageForward (XPTEXT *);
void PageBackward (XPTEXT *);
void PageTop (XPTEXT *);
void PageEnd (XPTEXT *);
void PageJump (XPTEXT *);

void DrawBack (void);
void SetConfigColor (void);
void SetHtmlColor (void);
void ClearText (void);
void WaitVdisp (void);
signed short CheckLink (int, XPTEXT *);
int CalcScbarY (int, int);
int CalcScbarLine (int, int);
