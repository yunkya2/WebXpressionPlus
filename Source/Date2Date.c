/* 日付文字列を変換して tm 構造体に代入して返す */
/* Sun, 05 Apr 1998 23:59:59 GMT (RFC1123)（推奨） */
/* Sunday, 05-Apr-98 23:59:59 GMT (RFC850) */
/* Sun Apr 5 23:59:59 1998 (ANSI C asctime) */
/* の３つの形式に対応 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "WebXpression.h"

void Date2Date (char *str, struct tm *time_stamp)
{
	char *month[]=
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	char month_str[4];
	char *s = str;
	int year;

	while (*s++ != ' ');

	switch (*(str + 3)) {
	case ',':		/* RFC1123 */
		sscanf (s, "%02d %3s %04d %02d:%02d:%02d",
			&time_stamp->tm_mday, month_str, &year,
			&time_stamp->tm_hour, &time_stamp->tm_min, &time_stamp->tm_sec);
		time_stamp->tm_year = year - 1900;
		break;

	case ' ':		/* ANSI C asctime */
		sscanf (s, "%3s %d %02d:%02d:%02d %d",
			month_str, &time_stamp->tm_mday,
			&time_stamp->tm_hour, &time_stamp->tm_min, &time_stamp->tm_sec,
			&year);
		time_stamp->tm_year = year - 1900;
		break;

	default:		/* RFC850 */
		sscanf (s, "%02d-%3s-%02d %02d:%02d:%02d",
			&time_stamp->tm_mday, month_str, &time_stamp->tm_year,
			&time_stamp->tm_hour, &time_stamp->tm_min, &time_stamp->tm_sec);
		break;
	}


	time_stamp->tm_mon = 0;
	while ((strnicmp (month_str, month[time_stamp->tm_mon], 3))
	       && (time_stamp->tm_mon < 11)) {
		time_stamp->tm_mon++;
	}
	time_stamp->tm_isdst = -1;
}
