#include "tcp-echo.h"

void te_stop_process(uv_loop_t *loop)
{
    te_process_t *process = (te_process_t *) loop->data;

    uv_stop(loop);

    switch (process->state) {
        case PROCESS_PAUSED:
        case PROCESS_RUNNING:
            process->state = PROCESS_STOPPING;
            break;
        default:
            process->state = PROCESS_KILLED;
            break;
    }
}

void te_close_loop(uv_loop_t *loop, uv_walk_cb walk_cb)
{
    int i;

    uv_walk(loop, walk_cb, NULL);

    /* Run loop a few times to clear out any handlers */
    for (i = 0; i <= 10 && uv_run(loop, UV_RUN_ONCE); i++);

    if (uv_loop_close(loop))
        te_log(WARN, "Could not cleanly close loop");
}

sds te_os_getenv(const char *name)
{
    sds ptr;
    int result;
    size_t size = 2;

    ptr = sdsnewlen("", size);

    result = uv_os_getenv(name, ptr, &size);

    if (result == UV_ENOBUFS) {
        ptr = sdsgrowzero(ptr, size);

        result = uv_os_getenv(name, ptr, &size);
    }

    if (result < 0) {
        ptr[0] = '\0';

        sdsupdatelen(ptr);
    }

    return ptr;
}

void te_signal_recv(uv_signal_t *handle, int signal)
{
    te_process_t *process = (te_process_t *) handle->loop->data;

    te_log(INFO, "Processing signal (%s)", strsignal(signal));

    switch (signal) {
        case SIGINT:
            if (process->is_worker)
                break;
        case SIGQUIT:
        case SIGTERM:
            te_stop_process(handle->loop);
            break;
        default:
            break;
    }
}

int te_gettimestamp(char *buffer, size_t size)
{
    time_t now;
    struct tm tm;
    struct timeval tv;

    now = time(NULL);
    gmtime_r(&now, &tm);
    gettimeofday(&tv, NULL);

    return snprintf(
        buffer,
        size,
        "%d-%02d-%02d %02d:%02d:%02d.%03ld",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        (long) tv.tv_usec / 1000
    );
}

void te_sock_setreuse_port(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
        te_log_errno(FATAL, "Could not set port reuse");
}

void te_sock_set_linger(int sock, int enable, int timeout)
{
    struct linger linger;

    linger.l_onoff = enable;
    linger.l_linger = timeout;

    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1)
        te_log_errno(FATAL, "Could not set SO_LINGER");
}

void te_sock_set_tcp_linger(__attribute__((unused)) int sock, __attribute__((unused)) int timeout)
{
    #ifdef TCP_LINGER2
    if (setsockopt(sock, IPPROTO_TCP, TCP_LINGER2, &timeout, sizeof(timeout)) == -1)
        te_log_errno(FATAL, "Could not set TCP_LINGER2");
    #endif
}

sds te_getpeername(uv_tcp_t *handle)
{
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    sds peer;
    char buffer[16];
    int result;
    int failure = 1;

    if ((result = uv_tcp_getpeername(handle, (struct sockaddr*) &addr, &addrlen)) < 0) {
        te_log_uv(ERROR, result, "Could not get peer name");
        goto cleanup;
    }

    if ((result = uv_inet_ntop(AF_INET, &addr.sin_addr, buffer, sizeof(buffer))) < 0) {
        te_log_uv(ERROR, result, "Could not get text address");
        goto cleanup;
    }

    failure = 0;

    cleanup:
        if (failure)
            peer = sdsnew("unknown");
        else
            peer = sdscatprintf(sdsempty(), "%s:%d", buffer, ntohs(addr.sin_port));

    return peer;
}

int te_set_process_title(const char *fmt, ...)
{
    va_list args;
    char buffer[16];

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    return uv_set_process_title(buffer);
}

void te_init_process(te_process_t *process, int is_worker)
{
    memset(process, 0x0, sizeof(te_process_t));

    process->state = PROCESS_RUNNING;
    process->is_worker = is_worker;
    process->ppid = uv_os_getppid();
}

void te_set_libuv_allocator()
{
    int result;

    if ((result = uv_replace_allocator(te__malloc_uv, te__realloc_uv, te__calloc_uv, free)) != 0)
        te_log_uv(FATAL, result, "Could not replace libuv allocators");
}
