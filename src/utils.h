#ifndef __UTILS_H_
#define __UTILS_H_

#include "tcp-echo.h"

void te_sock_setreuse_port(int sock, int reuse);
void te_sock_set_linger(int sock, int enable, int timeout);
void te_sock_set_tcp_linger(int sock, int timeout);
sds te_getpeername(uv_tcp_t *handle);
int te_gettimestamp(char *buffer, size_t size);
sds te_os_getenv(const char *name);
void te_signal_recv(uv_signal_t *handle, int signal);
int te_set_process_title(const char *fmt, ...);
void te_stop_process(uv_loop_t *loop);
void te_init_process(te_process_t *process, int is_worker);
void te_init_signal(uv_loop_t *loop, uv_signal_t *handle, uv_signal_cb signal_cb, int signal);
void te_set_libuv_allocator(void);

void te_on_server_close(uv_handle_t *handle);
void te_on_connection_close(uv_handle_t *handle);
void te_on_loop_close(uv_handle_t *handle, void *arg);
void te_close_loop(uv_loop_t *loop, uv_walk_cb callback);

#define te_malloc(x) te__malloc(x, __FILE__, __LINE__)
#define te_realloc(x, y) te__realloc(x, y, __FILE__, __LINE__)
#define te_calloc(x, y) te__calloc(x, y, __FILE__, __LINE__)

static __inline void *te__malloc(size_t size, const char *file, int line)
{
    void *ptr = malloc(size);

    if (ptr == NULL)
        te_log_errno(FATAL, "Could not allocate memory in (%s) on line (%d)", file, line);

    return ptr;
}

static __inline void *te__malloc_uv(size_t size)
{
    return te__malloc(size, "libuv", 0);
}

static __inline void *te__realloc(void *oldptr, size_t size, const char *file, int line)
{
    void *ptr = realloc(oldptr, size);

    if (ptr == NULL)
        te_log_errno(FATAL, "Could not re-allocate memory in (%s) on line (%d)", file, line);

    return ptr;
}

static __inline void *te__realloc_uv(void *oldptr, size_t size)
{
    return te__realloc(oldptr, size, "libuv", 0);
}

static __inline void *te__calloc(size_t nelem, size_t size, const char *file, int line)
{
    void *ptr = calloc(nelem, size);

    if (ptr == NULL)
        te_log_errno(FATAL, "Could not zero allocate memory in (%s) on line (%d)", file, line);

    return ptr;
}

static __inline void *te__calloc_uv(size_t nelem, size_t size)
{
    return te__calloc(nelem, size, "libuv", 0);
}

#endif
