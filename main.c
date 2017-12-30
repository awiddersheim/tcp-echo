#include "main.h"

#define WORKERS 4
#define HANDLERS 10
#define PORT_NUMBER 8090
#define MAX_CONN 5
#define KEY_SIZE 256

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
    pthread_attr_t attr;
    sem_t *mutex;
    char key[KEY_SIZE];
    int result;
    #ifndef __APPLE__
    int handlers;
    #endif

    log_info("Started worker (%d)", worker_id);

    addrlen = sizeof(addr);

    sock = server_init(PORT_NUMBER, MAX_CONN);

    result = snprintf(key, KEY_SIZE, "/worker-%d", worker_id);

    if (result < 0)
        log_errno("Could not create semaphore key due to error in (%d)", worker_id);
    else if (result >= KEY_SIZE)
        log_fatal("Could not create semaphore key due to size in (%d)", worker_id);

    /* NOTE(awiddersheim): Cleanup any previous semaphores with the same
     * name.
     */
    if (sem_unlink(key) == -1) {
        /* NOTE(awiddersheim): Unlinking a semaphore which might not have been
         * opened on OSX seems to return EINVAL. Haven't been able to find any
         * documentation to support this though.
         */
        if (errno != ENOENT && errno != EINVAL)
            log_errno("Could not sem_unlink() key (%s) in (%d)", key, worker_id);
    }

    if ((mutex = sem_open(key, O_CREAT, 0644, HANDLERS - 1)) == SEM_FAILED)
        log_errno("Could not create semaphore key (%s) in (%d)", key, worker_id);

    if (pthread_attr_init(&attr) != 0)
        log_errno("Could not create thread attribute in (%d)", worker_id);

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
        log_errno("Could set detached state in (%d)", worker_id);

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

        if ((conn = malloc(sizeof(struct handler_conn))) == NULL)
            log_errno("Could not allocate memory for connection storage in (%d)", worker_id);

        conn->fd = fd;
        conn->addr = addr;
        conn->mutex = mutex;
        conn->worker_id = worker_id;

        sem_wait(mutex);

        if (pthread_create(&thread, &attr, &handler__thread, conn) != 0)
            log_errno("Could not start handler thread in (%d)", worker_id);
    }

    pthread_attr_destroy(&attr);
    close(sock);
    sem_unlink(key);

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
