#include "main.h"

void handler__cleanup(void *_in) {
    struct handler_conn *conn = _in;

    log_info(
        "Closing connection to %s:%d in (%d)",
        inet_ntoa(conn->addr.sin_addr),
        ntohs(conn->addr.sin_port),
        conn->worker_id
    );

    close(conn->fd);
    sem_post(conn->mutex);
    free(conn);
}

void *handler__thread(void *_in)
{
    struct handler_conn *conn = _in;
    char buffer[200];
    int data;
    int result;

    pthread_cleanup_push(handler__cleanup, conn);

    while(1) {
        data = recv(conn->fd, buffer, sizeof(buffer), 0);

        if (data == 0) {
            break;
        }
        else if (data < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;

            log_errno(
                "Could not recv() from %s:%d in (%d)",
                inet_ntoa(conn->addr.sin_addr),
                ntohs(conn->addr.sin_port),
                conn->worker_id
            );
        }

        while(data) {
            if ((result = send(conn->fd, buffer, data, 0)) < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;

                log_errno(
                    "Could not send() to %s:%d in (%d)",
                    inet_ntoa(conn->addr.sin_addr),
                    ntohs(conn->addr.sin_port),
                    conn->worker_id
                );
            }

            data -= result;
        }
    }

    pthread_exit(NULL);
    pthread_cleanup_pop(0);

    return NULL;
}

void *worker__thread(void *_in)
{
    unsigned int worker_id = *(int *)_in;
    int sock;
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen;
    struct handler_conn *conn;
    pthread_t thread;
    pthread_attr_t *attr;
    sem_t *mutex;
    char *key;
    #ifndef __APPLE__
    int handlers;
    #endif

    log_info("Started worker (%d)", worker_id);

    addrlen = sizeof(addr);

    sock = server_init(PORT_NUMBER, MAX_CONN);

    key = key_init(worker_id);

    mutex = semaphore_init(key);

    attr = thread_init();

    while(1) {
        if ((fd = accept(sock, (struct sockaddr *)&addr, &addrlen)) < 0)
            log_errno(
                "Could not accept() connection from %s:%d in (%d)",
                inet_ntoa(addr.sin_addr),
                ntohs(addr.sin_port),
                worker_id
            );

        #ifndef __APPLE__
        sem_getvalue(mutex, &handlers);
        #endif

        log_info(
            #ifdef __APPLE__
            "Handling connection from %s:%d in (%d)",
            #else
            "Handling connection (%d) from %s:%d in (%d)",
            HANDLERS - handlers,
            #endif
            inet_ntoa(addr.sin_addr),
            ntohs(addr.sin_port),
            worker_id
        );

        conn = xmalloc(sizeof(struct handler_conn));

        conn->fd = fd;
        conn->addr = addr;
        conn->mutex = mutex;
        conn->worker_id = worker_id;

        sem_wait(mutex);

        if (pthread_create(&thread, attr, &handler__thread, conn) != 0)
            log_errno("Could not start handler thread in (%d)", worker_id);
    }

    close(sock);

    pthread_attr_destroy(attr);
    free(attr);

    sem_unlink(key);
    free(key);

    return NULL;
}

int main(void)
{
    unsigned int i;
    unsigned int worker_id[WORKERS];
    pthread_t workers[WORKERS];

    log_info("Starting (%d) workers", WORKERS);

    for (i = 0; i < WORKERS; ++i) {
        worker_id[i] = i + 1;
        if (pthread_create(&workers[i], NULL, &worker__thread, &worker_id[i]) != 0)
            log_errno("Failed starting worker (%d)", i + 1);
    }

    log_info("Listening on 0.0.0.0:%d", PORT_NUMBER);

    for (i = 0; i < WORKERS; ++i) {
        pthread_join(workers[i], NULL);
    }

    return 0;
}
