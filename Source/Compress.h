/* Compress.h */


/* 関数プロトタイプ宣言 */
void CompressImageHS (void *, void *, unsigned short, unsigned short);
void CompressImageHQ (void *, void *, unsigned short, unsigned short);
void CompressImage256HS (unsigned short *, unsigned char *,
			 unsigned short *, unsigned short, unsigned short);
void CompressImage256HQ (unsigned short *, unsigned char *,
			 unsigned short *, unsigned short, unsigned short);
void NonCompressImage256 (unsigned short *, unsigned char *,
			  unsigned short *, unsigned short, unsigned short);
