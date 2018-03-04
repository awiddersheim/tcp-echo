#include "tcp-echo.h"

int te_os_getenv(const char *name, char **var)
{
    int result;
    char *envptr;
    size_t size = 1;

    envptr = te_malloc(size);

    result = uv_os_getenv(name, envptr, &size);

    if (result == UV_ENOBUFS) {
        envptr = te_realloc(envptr, size);

        result = uv_os_getenv(name, envptr, &size);
    }

    *var = envptr;

    return result;
}

void te_signal_recv(__attribute__((unused)) uv_signal_t *handle, int signal)
{
    te_log(INFO, "Processing signal (%s)", strsignal(signal));

    switch (signal) {
        case SIGINT:
            if (is_worker)
                break;
        case SIGQUIT:
        case SIGTERM:
            uv_stop(&loop);

            switch (process_state) {
                case RUNNING:
                    process_state = STOPPING;
                    break;
                default:
                    process_state = KILLED;
                    break;
            }

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

char *te_getpeername(uv_tcp_t *handle)
{
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    char *peer;
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
            te_asprintf(&peer, "unknown");
        else
            te_asprintf(&peer, "%s:%d", buffer, ntohs(addr.sin_port));

    return peer;
}
