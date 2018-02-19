#include "tcp-echo.h"

int gettimestamp(char *buffer, size_t size)
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

void sock_setreuse_port(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
        logge(FATAL, "Could not set port reuse");
}

void sock_set_linger(int sock, int enable, int timeout)
{
    struct linger linger;

    linger.l_onoff = enable;
    linger.l_linger = timeout;

    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1)
        logge(FATAL, "Could not disable linger");
}

int init_worker(struct worker *worker, int id)
{
    int result;

    worker->id = id;
    worker->status = -1;
    worker->pid = -1;

    result = snprintf(worker->title, sizeof(worker->title), "worker-%d", id);

    if (result >= 0 && (size_t) result >= sizeof(worker->title))
        logg(WARN, "Could not write entire worker title (worker-%d)", id);

    return 0;
}

int update_worker_pid(struct worker *worker, int pid)
{
    worker->pid = pid;

    return 0;
}

char *xgetpeername(uv_tcp_t *handle)
{
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    char *peer;
    char buffer[16];
    int result;
    int failure = 1;

    if ((result = uv_tcp_getpeername(handle, (struct sockaddr*) &addr, &addrlen)) < 0) {
        logguv(ERROR, result, "Could not get peer name");
        goto cleanup;
    }

    if ((result = uv_inet_ntop(AF_INET, &addr.sin_addr, buffer, sizeof(buffer))) < 0) {
        logguv(ERROR, result, "Could not get text address");
        goto cleanup;
    }

    failure = 0;

    cleanup:
        if (failure)
            xasprintf(&peer, "unknown");
        else
            xasprintf(&peer, "%s:%d", buffer, ntohs(addr.sin_port));

    return peer;
}
