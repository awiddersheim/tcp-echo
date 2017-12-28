#include "main.h"

#define WORKER_COUNT 4
#define HANDLER_COUNT 10
#define PORT_NUMBER 8090
#define MAX_CONN 5

void *handler__thread(void *_in) {
    struct handler_conn *conn = _in;
    char buffer[200];
    int data;
    int result;

    while(1) {
        data = recv(conn->fd, buffer, sizeof(buffer), 0);

        if (data == 0) {
            break;
        }
        else if (data < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;

            log_errno("Could not recv() from %s:%d", inet_ntoa(conn->addr.sin_addr), ntohs(conn->addr.sin_port));
        }

        while(data) {
            if ((result = send(conn->fd, buffer, data, 0)) < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;

                log_errno("Could not send() to %s:%d", inet_ntoa(conn->addr.sin_addr), ntohs(conn->addr.sin_port));
            }

            data -= result;
        }
    }

    close(conn->fd);
    free(conn);

    return NULL;
}

void *worker__thread() {
    int sock;
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen;
    struct handler_conn *conn;
    pthread_t thread;

    addrlen = sizeof(addr);

    sock = server_init(PORT_NUMBER, MAX_CONN);

    while(1) {
        if ((fd = accept(sock, (struct sockaddr *)&addr, &addrlen)) < 0)
            log_errno("Could not accept() connection from %s:%d", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        log_info("Handling connection from %s:%d in (%d)", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), (int)pthread_self());

        if ((conn = malloc(sizeof(struct handler_conn))) == NULL)
            log_errno("Could not allocate memory for connection storage");

        conn->fd = fd;
        conn->addr = addr;

        if (pthread_create(&thread, NULL, &handler__thread, conn) != 0)
            log_errno("Could not start handler");
    }

    close(sock);

    return NULL;
}

int main(void) {
    unsigned int i;
    pthread_t workers[WORKER_COUNT];

    log_info("Starting (%d) workers", WORKER_COUNT);

    for (i = 0; i < WORKER_COUNT; ++i) {
        if (pthread_create(&workers[i], NULL, &worker__thread, NULL) != 0)
            log_errno("Failed starting worker (%d)", i + 1);
    }

    log_info("Listening on 0.0.0.0:%d", PORT_NUMBER);

    for (i = 0; i < WORKER_COUNT; ++i) {
        pthread_join(workers[i], NULL);
    }
}
