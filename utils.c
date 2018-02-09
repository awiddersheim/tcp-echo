#include "main.h"

void sock_setreuse_port(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
        logge(FATAL, "Could not set port reuse");
}

int init_worker(struct worker *worker, int id)
{
    int result;

    worker->id = id;
    worker->status = -1;
    worker->pid = -1;

    result = snprintf(worker->title, sizeof(worker->title), "worker-%d", id);

    if (result < 0) {
        logge(ERROR, "Could not write worker title for (worker-%d)", id);
        return -1;
    } else if ((unsigned long)result >= sizeof(worker->title)) {
        logg(WARN, "Could not write entire worker title for (worker-%d)", id);
    }

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
