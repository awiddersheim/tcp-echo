#ifndef __UTILS_H_
#define __UTILS_H_

#include "tcp-echo.h"

void sock_setreuse_port(int sock, int reuse);
void sock_set_linger(int sock, int enable, int timeout);
void sock_set_tcp_linger(int sock, int timeout);
int init_worker(struct worker *worker, int id);
int update_worker_pid(struct worker *worker, int pid);
char *xgetpeername(uv_tcp_t *handle);
int gettimestamp(char *buffer, size_t size);

void initproctitle (int argc, char **argv);
void setproctitle (const char *prog, const char *txt);

#define xmalloc(x) __xmalloc(x, __FILE__, __LINE__)

static __inline void *__xmalloc(size_t size, const char *file, int line)
{
    void *ptr = malloc(size);

    if (ptr == NULL)
        logge(FATAL, "Could not allocate memory in (%s) on line (%d)", file, line);

    return ptr;
}

static __inline void xvasprintf(char **strp, const char *format, va_list args)
{
    int result;
    va_list args_tmp;

    /* Calculate length needed for string */
    __va_copy(args_tmp, args);
    result = (vsnprintf(NULL, 0, format, args_tmp) + 1);
    va_end(args_tmp);

    /* Allocate and fill string */
    *strp = xmalloc(result);

    result = vsnprintf(*strp, result, format, args);

    if (result < 0)
        logg(FATAL, "Could not create string (%d)", result);
}

static __inline void xasprintf(char **strp, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    xvasprintf(strp, format, args);
    va_end(args);
}

#endif
