#ifndef __UTILS_H_
#define __UTILS_H_

#include "tcp-echo.h"

void te_sock_setreuse_port(int sock, int reuse);
void te_sock_set_linger(int sock, int enable, int timeout);
void te_sock_set_tcp_linger(int sock, int timeout);
char *te_getpeername(uv_tcp_t *handle);
int te_gettimestamp(char *buffer, size_t size);
int te_os_getenv(const char *name, char **var);
void te_signal_recv(uv_signal_t *handle, int signal);

void te_on_server_close(uv_handle_t *handle);
void te_on_conn_close(uv_handle_t *handle);
void te_on_loop_close(uv_handle_t *handle, void *arg);
void te_close_loop(uv_loop_t *loop);

void te_initproctitle (int argc, char **argv);
void te_setproctitle (const char *prog, const char *txt);

#define te_malloc(x) te__malloc(x, __FILE__, __LINE__)
#define te_realloc(x, y) te__realloc(x, y, __FILE__, __LINE__)

static __inline void *te__malloc(size_t size, const char *file, int line)
{
    void *ptr = malloc(size);

    if (ptr == NULL)
        te_log_errno(FATAL, "Could not allocate memory in (%s) on line (%d)", file, line);

    return ptr;
}

static __inline void *te__realloc(void *oldptr, size_t size, const char *file, int line)
{
    void *ptr = realloc(oldptr, size);

    if (ptr == NULL)
        te_log_errno(FATAL, "Could not re-allocate memory in (%s) on line (%d)", file, line);

    return ptr;
}

static __inline void te_vasprintf(char **strp, const char *format, va_list args)
{
    int result;
    va_list args_tmp;

    /* Calculate length needed for string */
    __va_copy(args_tmp, args);
    result = (vsnprintf(NULL, 0, format, args_tmp) + 1);
    va_end(args_tmp);

    /* Allocate and fill string */
    *strp = te_malloc(result);

    result = vsnprintf(*strp, result, format, args);

    if (result < 0)
        te_log(FATAL, "Could not create string (%d)", result);
}

static __inline void te_asprintf(char **strp, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    te_vasprintf(strp, format, args);
    va_end(args);
}

#endif
