#ifndef AXTLS_OS_PORT_H
#define AXTLS_OS_PORT_H

#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SSL_CTX_MUTEX_INIT(mutex)
#define SSL_CTX_MUTEX_DESTROY(mutex)
#define SSL_CTX_LOCK(mutex)
#define SSL_CTX_UNLOCK(mutex)

#define SOCKET_READ(s, buf, size)       read(s, buf, size)
#define SOCKET_WRITE(s, buf, size)      write(s, buf, size)
#define SOCKET_CLOSE(A)                 UNUSED
#define SOCKET_ERRNO()                  errno

#endif // AXTLS_OS_PORT_H
