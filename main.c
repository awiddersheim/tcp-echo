#include "main.h"

volatile sig_atomic_t sig_recv;

void signal_recv(int signal)
{
    sig_recv = signal;
}

void handler__cleanup(void *_in) {
    struct conn *conn = _in;

    logg(
        INFO,
        "Closing connection to %s:%d",
        inet_ntoa(conn->addr.sin_addr),
        ntohs(conn->addr.sin_port)
    );

    close(conn->fd);
    sem_post(conn->mutex);
    free(conn);
}

void *handler__thread(void *_in)
{
    struct conn *conn = _in;
    char buffer[200];
    int data;
    int result;

    if (conn->id > 0)
        set_thread_name("Handler-%d", conn->id);
    else
        set_thread_name("Handler", conn->id);

    pthread_cleanup_push(handler__cleanup, conn);

    for (;;) {
        data = recv(conn->fd, buffer, sizeof(buffer), 0);

        if (data == 0) {
            break;
        }
        else if (data < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;

            log_errno(
                FATAL,
                "Could not recv() from %s:%d in (%d)",
                inet_ntoa(conn->addr.sin_addr),
                ntohs(conn->addr.sin_port)
            );
        }

        while (data) {
            if ((result = send(conn->fd, buffer, data, 0)) < 0) {
                if (errno == EAGAIN || errno == EINTR)
                    continue;

                log_errno(
                    FATAL,
                    "Could not send() to %s:%d",
                    inet_ntoa(conn->addr.sin_addr),
                    ntohs(conn->addr.sin_port)
                );
            }

            data -= result;
        }
    }

    pthread_exit(NULL);
    pthread_cleanup_pop(0);

    return NULL;
}

/* TODO(awiddersheim): Use thread pools for handling connections
 * instead of semaphores limiting the number of concurrent threads.
 * Alternatively, just go straight to async I/O with libuv.
 */
void worker__process(struct worker worker)
{
    unsigned int quit = 0;
    int sock;
    int fd;
    struct sockaddr_in addr;
    socklen_t addrlen;
    struct conn *conn;
    pthread_t thread;
    pthread_attr_t *attr;
    sem_t *mutex;
    char *key;
    #ifndef __APPLE__
    int handlers;
    #endif
    struct timeval tv;
    fd_set fds;

    sig_recv = 0;
    signal(SIGQUIT, signal_recv);
    signal(SIGTERM, signal_recv);
    signal(SIGINT, SIG_IGN);

    setproctitle("tcp-echo", "worker");
    title = worker.title;
    set_thread_name("MainThread");

    logg(INFO, "Worker (%d) created", worker.id);

    addrlen = sizeof(addr);

    sock = server_init(PORT_NUMBER, MAX_CONN);

    key = key_init(worker.id);

    mutex = semaphore_init(key);

    attr = thread_init();

    /* Timeout for select() */
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (quit != 1) {
        if (sig_recv != 0) {
            logg(INFO, "Processing signal (%s)", strsignal(sig_recv));

            switch (sig_recv) {
                case SIGINT:
                case SIGQUIT:
                case SIGTERM:
                    quit = 1;
                    continue;
                default:
                    break;
            }
        }

        sig_recv = 0;

        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        if ((select(sock + 1, &fds, NULL, NULL, &tv)) < 0) {
            if (errno == EINTR)
                continue;
            else
                log_errno(FATAL, "Could not select() on socket");
        }

        if (FD_ISSET(sock, &fds)) {
            if ((fd = accept(sock, (struct sockaddr *)&addr, &addrlen)) < 0) {
                if (errno == EINTR || errno == EAGAIN ||  errno == EWOULDBLOCK)
                    continue;
                else
                    log_errno(
                        FATAL,
                        "Could not accept() connection from %s:%d",
                        inet_ntoa(addr.sin_addr),
                        ntohs(addr.sin_port)
                    );
            }

            #ifndef __APPLE__
            sem_getvalue(mutex, &handlers);
            #endif

            logg(
                INFO,
                #ifdef __APPLE__
                "Handling connection from %s:%d",
                #else
                "Handling connection (%d) from %s:%d",
                HANDLERS - handlers,
                #endif
                inet_ntoa(addr.sin_addr),
                ntohs(addr.sin_port)
            );

            conn = xmalloc(sizeof(struct conn));

            conn->fd = fd;
            conn->addr = addr;
            conn->mutex = mutex;
            #ifdef __APPLE__
            conn->id = 0;
            #else
            conn->id = HANDLERS - handlers;
            #endif

            sem_wait(mutex);

            if (pthread_create(&thread, attr, &handler__thread, conn) != 0)
                log_errno(FATAL, "Could not start handler thread");
        }
    }

    close(sock);

    pthread_attr_destroy(attr);
    free(attr);

    sem_unlink(key);
    free(key);

    exit(0);
}

int main(int argc, char *argv[])
{
    unsigned int i = 0;
    unsigned int quit = 0;
    struct worker workers[WORKERS];
    char *master_title = "master";
    int pid;

    initproctitle(argc, argv);
    setproctitle("tcp-echo", "master");
    title = master_title;
    set_thread_name("MainThread");

    sig_recv = 0;
    signal(SIGQUIT, signal_recv);
    signal(SIGTERM, signal_recv);
    signal(SIGINT, signal_recv);

    logg(INFO, "Starting (%d) workers", WORKERS);

    /* TODO(awiddersheim): When starting workers, use CPU count as the number
     * of workers to start. Also, pin each worker to it's own CPU.
     */
    while (i < WORKERS) {
        if (init_worker(&workers[i], i + 1) < 0)
            continue;

        logg(INFO, "Creating (%s)", workers[i].title);

        pid = fork();

        switch (pid) {
            case -1:
                /* failure */
                log_errno(ERROR, "Could not start worker (%d)... retrying", i);
                continue;
            case 0:
                /* child */
                update_worker_pid(&workers[i], getpid());
                worker__process(workers[i]);
                break;
            default:
                /* parent */
                update_worker_pid(&workers[i], pid);
                logg(INFO, "Created worker (%d) with pid (%d)", workers[i].id, workers[i].pid);
                i++;
                break;
        }
    }

    logg(INFO, "Listening on 0.0.0.0:%d", PORT_NUMBER);

    /* TODO(awiddersheim): Be a lot more intelligent of a master by reaping
     * and respawning any children that may have died before they should
     * have.
     */
    while (quit != 1)
    {
        if (sig_recv != 0) {
            logg(INFO, "Processing signal (%s)", strsignal(sig_recv));

            switch (sig_recv) {
                case SIGINT:
                case SIGQUIT:
                case SIGTERM:
                    quit = 1;
                    continue;
                default:
                    break;
            }
        }

        sig_recv = 0;

        sleep(1);
    }

    for (i = 0; i < WORKERS; i++) {
        logg(INFO, "Terminating (worker-%d) with pid (%d)", workers[i].id, workers[i].pid);

        kill(workers[i].pid, SIGTERM);

        waitpid(workers[i].pid, &workers[i].status, 0);

        logg(INFO, "Worker (%d) exited with (%d)", workers[i].id, WEXITSTATUS(workers[i].status));
    }

    return 0;
}
