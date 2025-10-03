/* Image.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>		/* spawnlp() のために必要 */
#include <x68k/dos.h>
#include <x68k/iocs.h>
#include "WebXpression.h"
#include "Httpfile.h"
#include "MicroConsole.h"
#include "Config.h"
#include "Image.h"
#include "Compress.h"
#include "GetFile.h"
#include "WebCache.h"
#include "gifl.h"
#include "MpuCache.h"

extern GIFLOAD *gifdecodemain (GIFLOAD * g, char *p);
extern void new_B_KEYINP (void), new_B_KEYSNS (void), new_BITSNS (void);
extern void new_CONCTRL (void), new_MS_INIT (void), new_MS_CUROF (void), new_MS_CURST (void),
  new_SKEY_MOD (void);

static short node = 0;		/* ノード数 */
static IMAGE_LIST *image_list_top, *image_list_end;
static IMAGE_LIST *image_list_ptr;	/* 次に表示するイメージ */



void InitLoadImage (void)
{
	image_list_top = NULL;
	image_list_end = NULL;
	image_list_ptr = NULL;

	GetMpuType();
}



void DispImageList (void)
{
	IMAGE_LIST *t_ptr = image_list_top;

	printf ("イメージリストを表示します\n");
	while (t_ptr != NULL) {
		printf ("イメージ : %s\n", t_ptr->url);
		t_ptr = t_ptr->next_ptr;
	}
}



/* 指定されたノードを削除する */
static void DeleteImageNode (IMAGE_LIST * t_ptr)
{
	if (image_list_top != NULL) {
		if (t_ptr->data != NULL)
			free (t_ptr->data);

		if (t_ptr->before_ptr == NULL) {
			if (t_ptr->next_ptr == NULL) {
			    /* １つしかないノードを削除 */
				image_list_top = NULL;
				image_list_end = NULL;
				image_list_ptr = NULL;
				free (t_ptr);
			} else {
			    /* 先頭のノードを削除 */
				image_list_top = t_ptr->next_ptr;
				(t_ptr->next_ptr)->before_ptr = NULL;
				free (t_ptr);
			}
		} else {
			if (t_ptr->next_ptr == NULL) {
			    /* 末尾のノードを削除 */
				image_list_end = t_ptr->before_ptr;
				(t_ptr->before_ptr)->next_ptr = NULL;
				free (t_ptr);
			} else {
			    /* 中間のノードを削除 */
				(t_ptr->before_ptr)->next_ptr = t_ptr->next_ptr;
				(t_ptr->next_ptr)->before_ptr = t_ptr->before_ptr;
				free (t_ptr);
			}
		}
	}
}



/* 先頭にノードを１つ追加する */
/* Html2Xpression() で <IMG SRC> を見つけ次第呼ばれるルーチン */
IMAGE_LIST *InsertImageNode (HTTPFILE * httpfile)
{
	IMAGE_LIST *t_ptr;

    /* query, anchor を削除する処理が必要か？ */

	if ((t_ptr = malloc (sizeof (IMAGE_LIST))) == NULL) {
		McDbPuts ("InsertImageNode() : ");
		McPuts ("※ メモリが足りません\n");
		return (NULL);
	} else {
		if (image_list_top == NULL) {
		    /* ノード０個の所に追加 */
			image_list_top = t_ptr;
			image_list_end = t_ptr;
			image_list_ptr = t_ptr;
			t_ptr->before_ptr = NULL;
			t_ptr->next_ptr = NULL;
		} else {
		    /* 先頭にノードを追加 */
			image_list_top->before_ptr = t_ptr;
			t_ptr->before_ptr = NULL;
			t_ptr->next_ptr = image_list_top;
			image_list_top = t_ptr;
			if (image_list_ptr == NULL)	/* 読み込みポインタが先頭ならば */
				image_list_ptr = t_ptr;
		}
		t_ptr->x = 0;
		t_ptr->y = 0;
		t_ptr->data = NULL;
		t_ptr->count = 1;
		strcpy (t_ptr->url, httpfile->url);
	}

	node++;

    /* キャッシュしているノードが cache_image を越えたら未参照のノードを削除 */
	if (node > cache_image) {
		IMAGE_LIST *t2_ptr;
		t2_ptr = image_list_end;
		while (t2_ptr != NULL) {
			if ((!t2_ptr->count) && (t2_ptr != image_list_ptr) && (t2_ptr != t_ptr)) {
				DeleteImageNode (t2_ptr);
				node--;
				break;
			}
			t2_ptr = t2_ptr->before_ptr;
		}
	}
	return (t_ptr);
}



/* url で指定したノードを検索する */
IMAGE_LIST *SearchImageNode (HTTPFILE * httpfile)
{
	IMAGE_LIST *t_ptr = image_list_top;

    /* query, anchor を削除する処理が必要か？ */
	while (t_ptr != NULL) {
		if (!strcmp (t_ptr->url, httpfile->url)) {
			t_ptr->count++;
			return (t_ptr);		/* 見つかったのでポインタを返す */
		}
		t_ptr = t_ptr->next_ptr;
	}

	return (NULL);
}



static int LoadGif (HTTPFILE * httpfile)
{
	unsigned char *h = httpfile->content;
	unsigned short xs, ys;	/* 元画像の大きさ(Source) */
	unsigned short xd, yd;	/* 展開後画像の大きさ(Dest) */
	GIFLOAD *g,*g_ret;
	char temp_fname[256];
	int mpu_cache;

    /* .GIF のヘッダから画像の大きさを得る */
	xs = ((unsigned short) (*(h + 7))) * 256 + (unsigned short) (*(h + 6));
	ys = ((unsigned short) (*(h + 9))) * 256 + (unsigned short) (*(h + 8));
	if (image_compress) {
		xd = (xs + 1) / 2;
		yd = (ys + 1) / 2;
	} else {
		xd = xs;
		yd = ys;
	}

    /* temp_fname を得るだけ */
	WCExist (httpfile, temp_fname);

	if ((int) (image_list_ptr->data = malloc (xd * yd * 2)) == 0) {
		McDbPuts ("LoadGif() : ");
		McPuts ("※ .GIF data 用メモリが足りません\n");
		image_list_ptr->data = NULL;
		return (-1);
	}
	if ((int) (g = malloc (sizeof (GIFLOAD))) == 0) {
		McDbPuts ("LoadGif() : ");
		McPuts ("※ .GIF ワーク用メモリが足りません\n");
		free (image_list_ptr->data);
		image_list_ptr->data = NULL;
		return (-1);
	}
    /* ワークをクリア */
	memset ((char *) g, 0, sizeof (GIFLOAD));	/* 0 で埋める */
	g->tpcolor = -1;
	g->addr = (void *) -1;

	McPuts (".GIF を展開しています...\n");
    /* g->addr に展開後のイメージが（malloc2() で確保されて）返ってくる */
    mpu_cache = SetMpuCacheMode (0);	/* キャッシュオフ */
	g_ret = gifdecodemain (g, temp_fname);	/* .gif デコード */
	SetMpuCacheMode (mpu_cache);		/* キャッシュを元の状態に */

	if ((int) g_ret < 0) {
		McDbPuts ("LoadGif() : ");
		McPuts ("※ .GIF のデコードができませんでした\n");
		if ((int) g->addr > 0)
			_dos_mfree (g->addr);
		free (g);
		free (image_list_ptr->data);
		image_list_ptr->data = NULL;
		return (-1);
	}
	if ((int) g->addr < 0) {
		McDbPuts ("LoadGif() : ");
		McPuts ("※ .GIF 展開用メモリが足りません\n");
		free (g);
		free (image_list_ptr->data);
		image_list_ptr->data = NULL;
		return (-1);
	}
	if (image_compress) {
		if (!image_quality)
			CompressImage256HS (image_list_ptr->data, g->addr, &g->pal_buf[0], xs, ys);
		else
			CompressImage256HQ (image_list_ptr->data, g->addr, &g->pal_buf[0], xs, ys);
		_dos_mfree (g->addr);
		free (g);
	} else {
		NonCompressImage256 (image_list_ptr->data, g->addr, &g->pal_buf[0], xs, ys);
		_dos_mfree (g->addr);
		free (g);
	}
	image_list_ptr->x = xd;
	image_list_ptr->y = yd;

	return (0);
}



static int LoadJpeg (HTTPFILE * httpfile)
{
	char cmdline[256];
	FILE *fp;
	unsigned short xs = 0, ys = 0;	/* 元画像の大きさ(Source) */
	unsigned short xd, yd;	/* 展開後画像の大きさ(Dest) */
	char temp_fname[256];
	unsigned short *temp_image;
	void *old_CONCTRL, *old_MS_INIT, *old_MS_CUROF, *old_MS_CURST, *old_SKEY_MOD;
	char progressive = 0;
	int jpeg_error_code = 0;

    /* temp_fname を得るだけ */
	WCExist (httpfile, temp_fname);

    /* JPEG ファイルの高さ／横幅を得る */
    /* vwx のソースの file.c を参考にしました（多謝） */
	if ((fp = fopen (temp_fname, "rb")) != NULL) {
		fseek (fp, 2, SEEK_SET);
		while (!feof (fp)) {
			unsigned char buf[8];
			int c;

			if ((c = fgetc (fp)) != 0xff)
				continue;
			c = fgetc (fp);
			if ((c != 0xc0) && (c != 0xc2)) {
				c = fgetc (fp) * 256 + fgetc (fp);
				fseek (fp, c - 2, 1);
				continue;
			}
			if (c == 0xc2) {	/* プログレッシブ JPEG ？ */
				progressive = !0;
			}
			fread (buf + 1, 7, sizeof (char), fp);
			ys = *(unsigned short *) (buf + 4);
			xs = *(unsigned short *) (buf + 6);
			break;
		}
		fclose (fp);
	}
	if (image_compress) {
		xd = (xs + 1) / 2;
		yd = (ys + 1) / 2;
	} else {
		xd = xs;
		yd = ys;
	}

	if ((int) (temp_image = malloc (xs * ys * 2)) == 0) {
		McDbPuts ("LoadJpeg() : ");
		McPuts ("※ .JPG 展開用メモリが足りません\n");
		image_list_ptr->data = NULL;
		return (-1);
	}
	if (!progressive) {
	    /* Baseline JPEG（普通の JPEG）の場合 */
	    /* JPEGED.R が画面やマウスを初期化しないように */
		old_CONCTRL = _dos_intvcs (0xff23, new_CONCTRL);
		old_MS_INIT = _dos_intvcs (0x170, new_MS_INIT);
		old_MS_CUROF = _dos_intvcs (0x172, new_MS_CUROF);
		old_MS_CURST = _dos_intvcs (0x176, new_MS_CURST);
		old_SKEY_MOD = _dos_intvcs (0x17d, new_SKEY_MOD);

		sprintf (cmdline, "-VS%d,%d,$%08x", xs, ys, (int)temp_image);
		McPuts (".JPG を展開しています...\n");
		jpeg_error_code = spawnlp (P_WAIT, "JPEGED.R", "JPEGED.R", cmdline, temp_fname, NULL);

		_dos_intvcs (0x17d, old_SKEY_MOD);
		_dos_intvcs (0x176, old_MS_CURST);
		_dos_intvcs (0x172, old_MS_CUROF);
		_dos_intvcs (0x170, old_MS_INIT);
		_dos_intvcs (0xff23, old_CONCTRL);
	} else {
	    /* Progressive JPEG の場合 */
		sprintf (cmdline, "-VS$%%08x", (int)temp_image);
		McPuts ("Progressive .JPG を展開しています...\n");
		jpeg_error_code = spawnlp (P_WAIT, "VSJPEG.X", "VSJPEG.X", cmdline, temp_fname, NULL);
	}
	if (jpeg_error_code) {
		McDbPuts ("LoadJpeg() : ");
		McPuts ("※ JPEG 展開時にエラーが発生しました\n");
		free (temp_image);
		image_list_ptr->data = NULL;
		return (-1);
	}
	if (image_compress) {
		if ((int) (image_list_ptr->data = malloc (xd * yd * 2)) > 0) {
			if (!image_quality)
				CompressImageHS (image_list_ptr->data, temp_image, xs, ys);
			else
				CompressImageHQ (image_list_ptr->data, temp_image, xs, ys);
		} else {
			McDbPuts ("LoadJpeg() : ");
			McPuts ("※ .JPG data 用メモリが足りません\n");
			image_list_ptr->data = NULL;
		}
		free (temp_image);
		temp_image = NULL;
	} else {
		image_list_ptr->data = temp_image;
	}
	image_list_ptr->x = xd;
	image_list_ptr->y = yd;

	return (0);
}



static int LoadPNG (HTTPFILE * httpfile)
{
	char cmdline[256];
	FILE *fp;
	unsigned short xs = 0, ys = 0;	/* 元画像の大きさ(Source) */
	unsigned short xd, yd;	/* 展開後画像の大きさ(Dest) */
	char temp_fname[256];
	unsigned short *temp_image;
	int png_error_code = 0;

    /* temp_fname を得るだけ */
	WCExist (httpfile, temp_fname);

    /* PNG ファイルの高さ／横幅を得る */
	if ((fp = fopen (temp_fname, "rb")) != NULL) {
		char header_work[9];
		char *header = "\x89PNG\x0d\x0a\x1a\x0a";	/* .PNG ファイルのヘッダ */
		fread (header_work, sizeof (char), 8, fp);
		if (strncmp (header_work, header, 8)) {
			McPuts ("※ .PNG ファイルではありません\n");
			fclose (fp);
			return (-1);
		}
		while (!feof (fp)) {
			unsigned char type[5];	/* チャンクタイプ */
			unsigned char *t = type;
			int length;	/* チャンク長 */
			unsigned char *l = (unsigned char *) (&length);		/* 変なキャスト */

			*l++ = fgetc (fp);	/* これで length を得る */
			*l++ = fgetc (fp);
			*l++ = fgetc (fp);
			*l++ = fgetc (fp);

			*t++ = fgetc (fp);
			*t++ = fgetc (fp);
			*t++ = fgetc (fp);
			*t++ = fgetc (fp);
			if (!strnicmp (type, "IHDR", 4)) {
				unsigned char *xs_p = (unsigned char *) (&xs), *ys_p = (unsigned char *) (&ys);
				fseek (fp, 2, SEEK_CUR);
				*xs_p++ = fgetc (fp);
				*xs_p++ = fgetc (fp);
				fseek (fp, 2, SEEK_CUR);
				*ys_p++ = fgetc (fp);
				*ys_p++ = fgetc (fp);
				fseek (fp, -8, SEEK_CUR);
			}
			if (!strnicmp (type, "IEND", 4))
				break;
			fseek (fp, length, SEEK_CUR);
			fseek (fp, 4, SEEK_CUR);	/* CRC のぶん */
		}
		fclose (fp);
	}
	if (image_compress) {
		xd = (xs + 1) / 2;
		yd = (ys + 1) / 2;
	} else {
		xd = xs;
		yd = ys;
	}

	if ((int) (temp_image = malloc (xs * ys * 2)) == 0) {
		McDbPuts ("LoadPNG() : ");
		McPuts ("※ .PNG 展開用メモリが足りません\n");
		image_list_ptr->data = NULL;
		return (-1);
	}
	sprintf (cmdline, "-VS$%08x", (int)temp_image);
	McPuts (".PNG を展開しています...\n");
	png_error_code = spawnlp (P_WAIT, "PNGL.X", "PNGL.X", cmdline, temp_fname, NULL);

	if (png_error_code) {
		McDbPuts ("LoadPNG() : ");
		McPuts ("※ PNG 展開時にエラーが発生しました\n");
		free (temp_image);
		image_list_ptr->data = NULL;
		return (-1);
	}
	if (image_compress) {
		if ((int) (image_list_ptr->data = malloc (xd * yd * 2)) > 0) {
			if (!image_quality)
				CompressImageHS (image_list_ptr->data, temp_image, xs, ys);
			else
				CompressImageHQ (image_list_ptr->data, temp_image, xs, ys);
			image_list_ptr->x = xd;
			image_list_ptr->y = yd;
		} else {
			McDbPuts ("LoadPNG() : ");
			McPuts ("※ .PNG data 用メモリが足りません\n");
			image_list_ptr->data = NULL;
		}
		free (temp_image);
		temp_image = NULL;
	} else {
		image_list_ptr->data = temp_image;
		image_list_ptr->x = xd;
		image_list_ptr->y = yd;
	}

	return (0);
}



/* イメージリスト読み込み */
/* EVENT_IDLE 時呼び出されるルーチン */
int LoadImage (HTTPFILE * httpfile)
{
	HTTPFILE _t_httpfile, *t_httpfile = &_t_httpfile;
	int gf_ret;		/* GetFile() の返り値 */
	int ret = LI_COMPLETE_NOT_LOAD;


    /* 読み込むべきイメージがあるか？ */
	if (image_list_ptr == NULL)
		return (LI_COMPLETE_NOT_LOAD);

#if	0
	if ((image_list_ptr == image_list_top) && (image_list_ptr->data == NULL))
		return (!0);
#endif

	InitHttpfile (t_httpfile);
	CatHttpfile (t_httpfile, httpfile, image_list_ptr->url);
	strcpy(t_httpfile->referer, httpfile->url);

	gf_ret = GetFile (t_httpfile);
	switch (gf_ret) {
	case GF_SUCCESS:
	    /* 読み込めた場合 */
		McDbPrintf ("LoadImage() : %s を処理します\n", t_httpfile->url);

		if (!strcmp (t_httpfile->content_type, "image/gif"))
			LoadGif (t_httpfile);
		else if (!strcmp (t_httpfile->content_type, "image/jpeg"))
			LoadJpeg (t_httpfile);
		else if (!strcmp (t_httpfile->content_type, "image/png"))
			LoadPNG (t_httpfile);
		free (t_httpfile->content);

		image_list_ptr = image_list_ptr->before_ptr;
		if (image_list_ptr == NULL)
			ret = LI_COMPLETE_LOAD;	/* もう読み込む必要がない */
		else
			ret = LI_CONTINUE_LOAD;	/* まだ読み込む必要がある */
		break;

	case GF_ABORT_BREAK:
		image_list_ptr = NULL;
		ret = LI_COMPLETE_NOT_LOAD;	/* もう読み込む必要がない */
		break;

	case GF_ABORT_ESC:
	case GF_ERROR:
	    /* 読み込めなかった場合 */
		McDbPuts ("LoadImage() : ");
		McPrintf ("%s が読み込めませんでした\n", image_list_ptr->url);

		image_list_ptr = image_list_ptr->before_ptr;
		if (image_list_ptr == NULL)
			ret = LI_COMPLETE_NOT_LOAD;	/* もう読み込む必要がない */
		else
			ret = LI_CONTINUE_NOT_LOAD;	/* まだ読み込む必要がある */
		break;
	}

	return (ret);
}
