/* Config.c	WebXpression.cnf 読み込み */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/xglob.h>

#include "WebXpression.h"
#include "Config.h"


typedef struct {
	char *var_name;
	unsigned char *var_ptr;
} VAR_CHAR;

static VAR_CHAR var_char[]=
{
	"check-local-link", &check_local_link,
	"hold-online", &hold_online,
	"image-compress", &image_compress,
	"image-quality", &image_quality,
	"key-repeat-1st", &key_repeat_1st,
	"key-repeat-2nd", &key_repeat_2nd,
	"color-mode", &color_mode,
	"refresh-rate", &refresh_rate,
	NULL, NULL
};

typedef struct {
	char *var_name;
	unsigned short *var_ptr;
} VAR_SHORT;

static VAR_SHORT var_short[]=
{
	"cache-image", &cache_image,
	"webcache-save", &webcache_save,
	"history-max", &history_max,
	NULL, NULL
};

typedef struct {
	char *var_name;
	unsigned short *var_ptr;
} VAR_RGB;

static VAR_RGB var_rgb[]=
{
	"color-back", &config_color[0],
	"color-str", &config_color[1],
	"color-clear", &config_color[2],	/* ダミー */
	"color-link", &config_color[3],
	"color-fixed-back", &config_color[4],
	"color-fixed-highlight", &config_color[5],
	"color-fixed-str", &config_color[6],
	NULL, NULL
};

typedef struct {
	char *var_name;
	unsigned int *var_ptr;
} VAR_INT;

static VAR_INT var_int[]=
{
	"line-table-size", &line_table_size,
	"link-table-size", &link_table_size,
	"link-table-buffer-size", &link_table_buffer_size,
	"image-table-size", &image_table_size,
	"anchor-table-size", &anchor_table_size,
	NULL, NULL
};


typedef struct {
	char *var_name;
	unsigned char *var_ptr;
} VAR_STR;

static VAR_STR var_str[]=
{
	"text-editor", text_editor,
	NULL, NULL
};



/* 起動時に１回だけ呼ばれる */
int InitConfig (char *fname)
{
	FILE *fp;
	short h;
	char temp_str[256];

    /* WebCache.cnf で設定する値の初期値 */
	check_local_link = 1;
	hold_online = 1;
	image_compress = 1;
	image_quality = 0;
	key_repeat_1st = 30;
	key_repeat_2nd = 5;
	color_mode = 0;
	refresh_rate = 4;
	cache_image = 4;
	webcache_save = 4;
	history_max = 32;
	line_table_size = 4096;
	link_table_size = 1024;
	link_table_buffer_size = 32768;
	image_table_size = 1024;
	anchor_table_size = 1024;


	strcpy (temp_str, mypsp->exe_path);
	_addlastsep (temp_str);
	strcat (temp_str, fname);
    /* カレントディレクトリを検索 */
	if ((fp = fopen (fname, "r")) == NULL) {
	    /* WebXpression.x のあるディレクトリを検索 */
		if ((fp = fopen (temp_str, "r")) == NULL) {
			printf ("コンフィグファイルが読めません\n");
			return (-1);
		}
	}
	while (fscanf (fp, "%s", temp_str) != EOF) {
		char nl = 0;	/* 次行へ行くフラグ */
		if (ferror (fp) || feof (fp))
			break;

		h = 0;
		do {		/* char 型変数読み込み */
			if (!strcmp (temp_str, var_char[h].var_name)) {
				unsigned int t;
				fscanf (fp, "%d", &t);
				*var_char[h].var_ptr = (unsigned char) t;
				fgets (temp_str, 256, fp);	/* 以下改行まで読み捨てる */
				nl = !0;
				break;
			}
		} while (var_char[++h].var_name != NULL);
		if (nl)
			continue;

		h = 0;
		do {		/* short 型変数読み込み */
			if (!strcmp (temp_str, var_short[h].var_name)) {
				fscanf (fp, "%hd", var_short[h].var_ptr);
				fgets (temp_str, 256, fp);	/* 以下改行まで読み捨てる */
				nl = !0;
				break;
			}
		} while (var_short[++h].var_name != NULL);
		if (nl)
			continue;

		h = 0;
		do {		/* short 型変数（r,g,b で指定）読み込み */
			if (!strcmp (temp_str, var_rgb[h].var_name)) {
				unsigned int r, g, b;
				fscanf (fp, "%d,%d,%d", &r, &g, &b);
				*(var_rgb[h].var_ptr) = (unsigned short) ((g << 11) | (r << 6) | (b << 1));
				fgets (temp_str, 256, fp);	/* 以下改行まで読み捨てる */
				nl = !0;
				break;
			}
		} while (var_rgb[++h].var_name != NULL);
		if (nl)
			continue;

		h = 0;
		do {		/* int 型変数読み込み */
			if (!strcmp (temp_str, var_int[h].var_name)) {
				fscanf (fp, "%d", var_int[h].var_ptr);
				fgets (temp_str, 256, fp);	/* 以下改行まで読み捨てる */
				nl = !0;
				break;
			}
		} while (var_int[++h].var_name != NULL);
		if (nl)
			continue;

		h = 0;
		do {		/* char[] 型変数読み込み */
			if (!strcmp (temp_str, var_str[h].var_name)) {
				char *p = var_str[h].var_ptr;
				while (fgetc (fp) != (int) '"');
				while ((*p++ = (char) fgetc (fp)) != '"');
				*(p - 1) = '\0';
				fgets (temp_str, 256, fp);	/* 以下改行まで読み捨てる */
				nl = !0;
				break;
			}
		} while (var_str[++h].var_name != NULL);
		if (nl)
			continue;

		fgets (temp_str, 256, fp);	/* 以下改行まで読み捨てる */
	}

	return (0);
}
