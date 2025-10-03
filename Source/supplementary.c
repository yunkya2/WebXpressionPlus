/*
 * supplementary.c
 * X68k LIBCにあってelf2x68k(newlib)にない関数の代替実装
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <x68k/dos.h>
#include "WebXpression.h"

/****************************************************************************/

int stricmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && tolower(*s1) == tolower(*s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)tolower(*s1) - (unsigned char)tolower(*s2);
}

int strnicmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;

    while (n-- != 0 && tolower(*s1) == tolower(*s2)) {
        if (n == 0 || *s1 == '\0') {
            break;
        }
        s1++;
        s2++;
    }
    return (unsigned char)tolower(*s1) - (unsigned char)tolower(*s2);
}

/****************************************************************************/

/* _addlastsep(), _toslash() の実装はX68k LIBCのソースコードを引用しました */

#define ISMBBLEAD(c) \
        ((unsigned char)(((c) ^ 0x20) + 0x23) >= (unsigned char)((0x81 ^ 0x20) + 0x23))

char *_addlastsep(char *path)
{
    char *p;

    /* path を一文字づつ調べる */
    for (p = path; *p; p++) {

        /* バックスラッシュを変換 */
        if (*p == '\\')
            *p = '/';

        /* 漢字はスキップ */
        else if (ISMBBLEAD (p[0]) && p[1])
            p++;
    }

    /* 最後が / か : でなければ追加 */
    if (path < p && p[-1] != '/' && p[-1] != ':') {
        *p++ = '/';
        *p = '\0';
    }

    /* path を返す */
    return path;
}

char *_toslash(char *path)
{
    char *p;

    /* path を一文字づつ調べる */
    for (p = path; *p; p++) {

        /* バックスラッシュを変換 */
        if (*p == '\\')
            *p = '/';

        /* 漢字はスキップ */
        else if (ISMBBLEAD (p[0]) && p[1])
            p++;
    }

    /* path を返す */
    return path;
}

char *_fullpath(char *dst, const char *src, size_t maxlen)
{
    strncpy(dst, src, maxlen - 1);
    dst[maxlen - 1] = '\0';
    if (strlen(dst) > 1 && dst[strlen(dst) - 1] == '/')
        dst[strlen(dst) - 1] = '\0';
    return dst;
}

/****************************************************************************/

int spawnlp(int mode, const char *path, const char *arg0, ...)
{
    char file[256];
    char cmdline[256];
    int res;

    strcpy(file, path);
    strcat(file, " ");

    va_list ap;
    va_start(ap, arg0); // 最初の引数は飛ばす
    while (1) {
        const char *arg = va_arg(ap, const char *);
        if (arg == NULL)
            break;
        strcat(file, arg);
        strcat(file, " ");
    }
    va_end(ap);
    file[strlen(file) - 1] = '\0';

    // パス検索
    res = _dos_exec2(2, file, cmdline, NULL);
    if (res != 0) {
        return res;
    }

    // ロード、実行
    return _dos_exec2(0, file, cmdline, NULL);
}

int utime(const char *filename, const struct utimbuf *times)
{
    struct tm *tm;
    uint32_t dos_time;

    if (times == NULL) {
        return -1;
    }

    tm = localtime(&times->modtime);
    dos_time = ((tm->tm_year - 80) << 25) |
               ((tm->tm_mon + 1) << 21) |
               (tm->tm_mday << 16) |
               (tm->tm_hour << 11) |
               (tm->tm_min << 5) |
               (tm->tm_sec >> 1);

    int fd = _dos_open(filename, 1);
    if (fd < 0) {
        return -1;
    }
    int res = _dos_filedate(fd, dos_time);
    _dos_close(fd);
    return res;
}

/****************************************************************************/

#define LINEBUF_SIZE 1024
static char linebuf[LINEBUF_SIZE + 1];
static char *lineptr = NULL;
static char *lineend = NULL;

void  recvinit(void)
{
    lineptr = NULL;
    lineend = NULL;
}

int recvline(int socket, char *buff, size_t maxlen)
{
    char *p;
    if (lineptr && (p = strchr(lineptr, '\n'))) {
        size_t len = p - lineptr + 1;
        if (len > maxlen - 1)
            len = maxlen - 1;
        memcpy(buff, lineptr, len);
        buff[len] = '\0';
        lineptr = p + 1;
        return len;
    }

    if (lineptr) {
        size_t len = strlen(lineptr);
        if (len > maxlen - 1) {
            len = maxlen - 1;
            memcpy(buff, lineptr, len);
            buff[len] = '\0';
            lineptr += len;
            return len;
        }
        memcpy(buff, lineptr, len);
        buff[len] = '\0';
        buff += len;
        maxlen -= len;
        lineptr = NULL;
    }

    ssize_t n = read(socket, linebuf, LINEBUF_SIZE);
    if (n > 0) {
        linebuf[n] = '\0';
        lineptr = linebuf;
        lineend = linebuf + n;
        return recvline(socket, buff, maxlen);
    }
    return -1;
}

int recvremain(char *buff, size_t maxlen)
{
    if (lineptr) {
        size_t len = lineend - lineptr;
        if (len > maxlen)
            len = maxlen;
        memcpy(buff, lineptr, len);
        lineptr += len;
        if (lineptr >= lineend)
            lineptr = NULL;
        return len;
    } else {
        return 0;
    }
}
