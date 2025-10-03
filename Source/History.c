/* History.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <x68k/dos.h>

#include "History.h"
#include "Config.h"


/* 履歴テーブル構造体 */
typedef struct _history_table {
	int current_line;
	char url[256];
	char old_url[256];
} HISTORY_TABLE;


static HISTORY_TABLE *history_table;
static short history_ptr;



int InitHistory (void)
{
	short h;

	history_table = malloc (sizeof (HISTORY_TABLE) * history_max);
	if ((int) history_table == 0) {
		printf ("※ メモリが足りません（ヒストリー用のメモリが確保できません）\n");
		return (-1);
	}
	for (h = 0; h < history_max; h++) {
		history_table[h].current_line = 0;	/* 未使用に */
		*history_table[h].url = '\0';	/* 未使用に */
		*history_table[h].old_url = '\0';	/* 未使用に */
	}
	history_ptr = 0;

	return (0);
}



/* 先頭にノードを１つ追加する */
void AddHistory (char *url, char *old_url, int current_line)
{
	short h;
	if ((old_url != NULL) && (!strcmp (url, old_url)))
		return;

	for (h = 0; h < history_max; h++) {
		if (!strcmp (history_table[h].url, url)) {	/* 既にあったら追加しない */
			history_table[h].current_line = current_line;
			return;
		}
	}

#if	0
	printf ("AddHistory() :\n	new = %s\n	old = %s\n", url, old_url);

	for (h = 0; h < history_max; h++)
		printf ("history_table[%hd] = %s,%d\n", h, history_table[h].url, history_table[h].current_line);
#endif

	strcpy (history_table[history_ptr].url, url);
	if (old_url == NULL) {
		*history_table[history_ptr].old_url = '\0';
	} else {
		strcpy (history_table[history_ptr].old_url, old_url);
	}
	history_table[history_ptr].current_line = current_line;
	if (++history_ptr >= history_max)
		history_ptr = 0;
}



/* 前ののノードを返す（「戻る」ボタン処理） */
char *BeforeHistory (char *url, int *current_line)
{
	short h;

	*current_line = 0;
	for (h = 0; h < history_max; h++) {
#if	0
		printf ("history_table[%hd] = %s,%d\n", h, history_table[h].url, history_table[h].current_line);
#endif
		if (!strcmp (history_table[h].url, url)) {
			if (*history_table[h].old_url) {
				*current_line = history_table[h].current_line;
				return (history_table[h].old_url);
			} else {
				break;
			}
		}
	}
	return (NULL);
}



/* 次のノードを返す（「進む」ボタン処理） */
char *NextHistory (void)
{
	return (NULL);		/* まだないにょーん */
}
