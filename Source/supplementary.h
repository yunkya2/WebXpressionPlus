/* supplementary.h */

#ifndef _SUPPLEMENTARY_H_
#define _SUPPLEMENTARY_H_

#include <stdarg.h>
#include <time.h>
#include <utime.h>

#define P_WAIT 0

int stricmp(const char *s1, const char *s2);
int strnicmp(const char *s1, const char *s2, size_t n);
char *_addlastsep(char *path);
char *_toslash(char *path);
char *_fullpath(char *dst, const char *src, size_t maxlen);
int spawnlp(int mode, const char *path, const char *arg0, ...);
int utime(const char *filename, const struct utimbuf *times);
void recvinit(void);
int recvline(int socket, char *buff, size_t maxlen);
int recvremain(char *buff, size_t maxlen);

#endif /* _SUPPLEMENTARY_H_ */