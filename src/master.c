#include "tcp-echo.h"

volatile sig_atomic_t sig_recv;

uv_loop_t worker_loop;

typedef struct {
    uv_write_t request;
    uv_buf_t buffer;
    uv_stream_t *client;
} write_req_t;

void signal_recv(__attribute__((unused)) uv_signal_t *handle, int signal)
{
    sig_recv = signal;
}

void alloc_buffer(__attribute__((unused)) uv_handle_t *handle, size_t size, uv_buf_t *buffer)
{
    *buffer = uv_buf_init((char *) malloc(size), size);
}

void free_write_request(write_req_t *write_request)
{
    free(write_request->buffer.base);
    free(write_request);
}

void on_close(uv_handle_t *handle)
{
    if (handle->data)
        logg(INFO, "Closing connection from (%s)", handle->data);

    free(handle->data);
    free(handle);
}

void echo_write(uv_write_t *request, int status)
{
    write_req_t *write_request = (write_req_t *) request;

    if (status)
        logguv(ERROR, status, "Could not write to (%s)", write_request->client->data);

    free_write_request(write_request);
}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buffer)
{
    write_req_t *write_request;

    if (nread > 0) {
        write_request = xmalloc(sizeof(write_req_t));
        memset(write_request, 0x0, sizeof(write_req_t));

        write_request->buffer = uv_buf_init(buffer->base, nread);
        write_request->client = client;

        uv_write((uv_write_t *) write_request, client, &write_request->buffer, 1, echo_write);

        return;
    }

    if (nread < 0) {
        if (nread != UV_EOF)
            logguv(ERROR, (int) nread, "Could not read from (%s)", client->data);

        uv_close((uv_handle_t *) client, on_close);
    }

    free(buffer->base);
}

void on_connection(uv_stream_t *server, int status)
{
    char *peer;
    int result;
    uv_tcp_t *client;

    if (status < 0) {
        logguv(ERROR, status, "Could not handle new connection");
        return;
    }

    client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(&worker_loop, client);
    client->data = NULL;

    if ((result = uv_accept(server, (uv_stream_t *) client)) >= 0) {
        uv_read_start((uv_stream_t *) client, alloc_buffer, echo_read);

        peer = xgetpeername(client);

        logg(INFO, "Handling connection from (%s)", peer);

        client->data = peer;
    } else {
        logguv(ERROR, result, "Could not accept new connection");

        uv_close((uv_handle_t *) client, on_close);
    }
}

int worker__process(struct worker worker)
{
    int quit = 0;
    int fd;
    int result;
    struct sockaddr_in addr;
    uv_signal_t sigquit;
    uv_signal_t sigterm;
    uv_signal_t sigint;
    uv_tcp_t server;

    setproctitle("tcp-echo", "worker");
    title = worker.title;

    logg(INFO, "Worker (%d) created", worker.id);

    if ((result = uv_loop_init(&worker_loop)) < 0)
        logguv(FATAL, result, "Could not create worker loop");

    sig_recv = 0;
    uv_signal_init(&worker_loop, &sigquit);
    uv_signal_init(&worker_loop, &sigterm);
    uv_signal_init(&worker_loop, &sigint);
    uv_signal_start(&sigquit, signal_recv, SIGQUIT);
    uv_signal_start(&sigterm, signal_recv, SIGTERM);
    uv_signal_start(&sigint, signal_recv, SIGINT);

    uv_tcp_init_ex(&worker_loop, &server, AF_INET);

    uv_fileno((uv_handle_t *)&server, &fd);
    sock_setreuse_port(fd, 1);

    if ((result = uv_tcp_nodelay(&server, 1)) < 0)
        logguv(FATAL, result, "Could not set nodelay on socket");

    uv_ip4_addr("0.0.0.0", PORT_NUMBER, &addr);

    if ((result = uv_tcp_bind(&server, (const struct sockaddr *) &addr, 0)) < 0)
        logguv(FATAL, result, "Could not bind to port (%d)", PORT_NUMBER);

    if ((result = uv_listen((uv_stream_t *) &server, CONNECTION_BACKLOG, on_connection)) < 0)
        logguv(FATAL, result, "Could not listen for connections on (%d)", PORT_NUMBER);

    logg(INFO, "Listening on 0.0.0.0:%d", PORT_NUMBER);

    while (quit != 1)
    {
        if (sig_recv != 0) {
            logg(INFO, "Processing signal (%s)", strsignal(sig_recv));

            switch (sig_recv) {
                case SIGQUIT:
                case SIGTERM:
                    quit = 1;
                    continue;
                default:
                    break;
            }
            sig_recv = 0;
        }

        uv_run(&worker_loop, UV_RUN_ONCE);
    }

    logg(INFO, "Worker (%d) shutting down", worker.id);

    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    int result;
    unsigned int quit = 0;
    char *master_title = "master";
    int pid;
    struct worker *workers;
    uv_cpu_info_t *cpu_info;
    int cpu_count;
    uv_loop_t master_loop;
    uv_signal_t sigquit;
    uv_signal_t sigterm;
    uv_signal_t sigint;

    initproctitle(argc, argv);
    setproctitle("tcp-echo", "master");
    title = master_title;

    if ((result = uv_loop_init(&master_loop)) < 0)
        logguv(FATAL, result, "Could not create master loop");

    sig_recv = 0;
    uv_signal_init(&master_loop, &sigquit);
    uv_signal_init(&master_loop, &sigterm);
    uv_signal_init(&master_loop, &sigint);
    uv_signal_start(&sigquit, signal_recv, SIGQUIT);
    uv_signal_start(&sigterm, signal_recv, SIGTERM);
    uv_signal_start(&sigint, signal_recv, SIGINT);

    if ((result = uv_cpu_info(&cpu_info, &cpu_count)) < 0)
        logguv(FATAL, result, "Could not determine number of CPUs");

    uv_free_cpu_info(cpu_info, cpu_count);

    #if defined(WORKERS) && WORKERS > 0
    cpu_count = WORKERS;
    #endif

    logg(INFO, "Starting (%d) workers", cpu_count);

    workers = xmalloc(sizeof(struct worker) * cpu_count);
    memset(workers, 0x0, sizeof(struct worker) * cpu_count);

    /* TODO(awiddersheim): Pin each worker to it's own CPU. */
    for (i = 0; i < cpu_count;) {
        if (init_worker(&workers[i], i + 1) < 0)
            continue;

        logg(INFO, "Creating (%s)", workers[i].title);

        pid = fork();

        switch (pid) {
            case -1:
                /* failure */
                logge(ERROR, "Could not start worker (%d)... retrying", i);
                continue;
            case 0:
                /* child */

                /* TODO(awiddersheim): Build in some helper functions to
                 * close down loops easier.
                 */
                uv_loop_fork(&master_loop);
                uv_close((uv_handle_t *) &sigquit, NULL);
                uv_close((uv_handle_t *) &sigterm, NULL);
                uv_close((uv_handle_t *) &sigint, NULL);

                uv_run(&master_loop, UV_RUN_ONCE);
                uv_loop_close(&master_loop);

                update_worker_pid(&workers[i], uv_os_getpid());
                result = worker__process(workers[i]);
                free(workers);
                return result;
                break;
            default:
                /* parent */
                update_worker_pid(&workers[i], pid);
                logg(INFO, "Created worker (%d) with pid (%d)", workers[i].id, workers[i].pid);
                i++;
                break;
        }
    }

    logg(INFO, "All workers created");

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
            sig_recv = 0;
        }

        uv_run(&master_loop, UV_RUN_ONCE);
    }

    logg(INFO, "Master shutting down");

    for (i = 0; i < cpu_count; i++) {
        logg(INFO, "Terminating (worker-%d) with pid (%d)", workers[i].id, workers[i].pid);

        uv_kill(workers[i].pid, SIGTERM);

        waitpid(workers[i].pid, &workers[i].status, 0);

        logg(INFO, "Worker (%d) exited with (%d)", workers[i].id, WEXITSTATUS(workers[i].status));
    }

    free(workers);

    logg(INFO, "All done");

    return 0;
}
