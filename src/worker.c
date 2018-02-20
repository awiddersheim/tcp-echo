#include "tcp-echo.h"

uv_loop_t loop;

process_state_t process_state = RUNNING;

int is_worker = 1;

typedef struct {
    uv_tcp_t client;
    char *peer;
    time_t timeout;
} conn_t;

typedef struct {
    uv_write_t request;
    uv_buf_t buffer;
    conn_t *conn;
} write_req_t;

typedef enum {
    SERVER = 1,
    CLIENT = 2
} tcp_type_t;

void update_worker_title()
{
    int result;
    char *titleptr;

    if ((result = os_getenv("WORKER_TITLE", &titleptr)) < 0) {
        logguv(WARN, result, "Could not set worker title");

        snprintf(title, sizeof(title), "worker-%d", uv_os_getpid());

        goto cleanup;
    }

    memcpy(&title, titleptr, sizeof(title));

    cleanup:
        free(titleptr);
}

void alloc_buffer(__attribute__((unused)) uv_handle_t *handle, size_t size, uv_buf_t *buffer)
{
    *buffer = uv_buf_init((char *) malloc(size), size);
}

tcp_type_t *init_tcp_type(tcp_type_t type)
{
    tcp_type_t *type_ptr;

    type_ptr = xmalloc(sizeof(tcp_type_t));
    *type_ptr = type;

    return type_ptr;
}

conn_t *init_conn(uv_loop_t *loop)
{
    conn_t *conn;

    conn = xmalloc(sizeof(conn_t));
    memset(conn, 0x0, sizeof(conn_t));

    uv_tcp_init(loop, &conn->client);

    conn->timeout = time(NULL);

    conn->client.data = init_tcp_type(CLIENT);

    return conn;
}

write_req_t *init_write_request(conn_t *conn, char *buffer, int size)
{
    write_req_t *write_request;

    write_request = xmalloc(sizeof(write_req_t));
    memset(write_request, 0x0, sizeof(write_req_t));

    write_request->buffer = uv_buf_init(buffer, size);
    write_request->conn = conn;

    return write_request;
}

void on_conn_close(uv_handle_t *handle)
{
    conn_t *conn = (conn_t *) handle;

    free(conn->client.data);
    free(conn->peer);
    free(conn);
}

void echo_write(uv_write_t *request, int status)
{
    write_req_t *write_request = (write_req_t *) request;

    if (status)
        logguv(ERROR, status, "Could not write to (%s)", write_request->conn->peer);

    free(write_request->buffer.base);
    free(write_request);
}

void conn_write_timeout(uv_write_t *request, int status)
{
    int fd;
    write_req_t *write_request = (write_req_t *) request;

    if (status)
        logguv(ERROR, status, "Could not write to (%s)", write_request->conn->peer);

    logg(INFO, "Closing connection from (%s)", write_request->conn->peer);

    uv_fileno((uv_handle_t *) write_request->conn, &fd);
    #ifdef SEND_RESET
    sock_set_linger(fd, 1, 0);
    #endif

    uv_close((uv_handle_t *) write_request->conn, on_conn_close);

    free(write_request);
}

void echo_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buffer)
{
    conn_t *conn = (conn_t *) stream;
    write_req_t *write_request;

    if (nread > 0) {
        write_request = init_write_request(conn, buffer->base, nread);

        uv_write((uv_write_t *) write_request, (uv_stream_t *) conn, &write_request->buffer, 1, echo_write);

        conn->timeout = time(NULL);

        return;
    }

    if (nread < 0) {
        if (nread != UV_EOF) {
            logguv(ERROR, (int) nread, "Could not read from (%s), closing connection", conn->peer);
        } else {
            logg(INFO, "Connection closed by (%s)", conn->peer);
        }

        uv_close((uv_handle_t *) conn, on_conn_close);
    }

    free(buffer->base);
}

void on_stale_walk(uv_handle_t *handle, __attribute__((unused)) void *arg)
{
    conn_t *conn;
    double difference;
    const char *timeout = "Closing connection due to inactivity\n";
    write_req_t *write_request;

    if (uv_is_closing(handle))
        return;

    if (uv_handle_get_type(handle) == UV_TCP && handle->data != NULL
            && *(tcp_type_t *) handle->data == CLIENT) {
        conn = (conn_t *) handle;

        difference = difftime(time(NULL), conn->timeout);

        logg(DEBUG, "Connection from (%s) has been idle for (%.0f) seconds", conn->peer, difference);

        if (difference > CONNECTION_TIMEOUT) {
            logg(INFO, "Connection from (%s) has timed out", conn->peer);

            write_request = init_write_request(conn, (char *) timeout, strlen(timeout));

            uv_write((uv_write_t *) write_request, (uv_stream_t *) conn, &write_request->buffer, 1, conn_write_timeout);
        }
    }
}

void on_stale_timer(__attribute__((unused)) uv_timer_t *timer)
{
    uv_walk(&loop, on_stale_walk, NULL);
}

void on_connection(uv_stream_t *server, int status)
{
    int fd;
    int result;
    conn_t *conn;

    if (status < 0) {
        logguv(ERROR, status, "Could not handle new connection");
        return;
    }

    conn = init_conn(&loop);

    if ((result = uv_accept(server, (uv_stream_t *) conn)) >= 0) {
        conn->peer = xgetpeername((uv_tcp_t *) conn);

        logg(INFO, "Handling connection from (%s)", conn->peer);

        uv_fileno((uv_handle_t *) conn, &fd);
        sock_set_linger(fd, 1, LINGER_TIMEOUT);
        sock_set_tcp_linger(fd, LINGER_TIMEOUT);

        uv_read_start((uv_stream_t *) conn, alloc_buffer, echo_read);
    } else {
        logguv(ERROR, result, "Could not accept new connection");

        uv_close((uv_handle_t *) conn, on_conn_close);
    }
}

void init_server(uv_loop_t *loop, uv_tcp_t *server)
{
    int fd;
    int result;
    struct sockaddr_in addr;

    uv_tcp_init_ex(loop, server, AF_INET);
    server->data = init_tcp_type(SERVER);

    uv_fileno((uv_handle_t *) server, &fd);
    sock_setreuse_port(fd, 1);

    #ifdef ENABLE_NODELAY
    if ((result = uv_tcp_nodelay(server, 1)) < 0)
        logguv(FATAL, result, "Could not set nodelay on socket");
    #endif

    #ifdef ENABLE_KEEPALIVE
    if ((result = uv_tcp_keepalive(server, 1, KEEPALIVE_INTERVAL)) < 0)
        logguv(FATAL, result, "Could not set keepalive on socket");
    #endif

    uv_ip4_addr("0.0.0.0", PORT_NUMBER, &addr);

    if ((result = uv_tcp_bind(server, (const struct sockaddr *) &addr, 0)) < 0)
        logguv(FATAL, result, "Could not bind to port (%d)", PORT_NUMBER);

    if ((result = uv_listen((uv_stream_t *) server, CONNECTION_BACKLOG, on_connection)) < 0)
        logguv(FATAL, result, "Could not listen for connections on (%d)", PORT_NUMBER);
}

int main(int argc, char *argv[])
{
    int result;
    uv_signal_t sigquit;
    uv_signal_t sigterm;
    uv_signal_t sigint;
    uv_tcp_t server;
    uv_timer_t stale_timer;

    initproctitle(argc, argv);
    setproctitle("tcp-echo", "worker");
    update_worker_title();

    logg(INFO, "Worker created");

    if ((result = uv_loop_init(&loop)) < 0)
        logguv(FATAL, result, "Could not create worker loop");

    uv_signal_init(&loop, &sigquit);
    uv_signal_init(&loop, &sigterm);
    uv_signal_init(&loop, &sigint);
    uv_signal_start(&sigquit, signal_recv, SIGQUIT);
    uv_signal_start(&sigterm, signal_recv, SIGTERM);
    uv_signal_start(&sigint, signal_recv, SIGINT);

    uv_timer_init(&loop, &stale_timer);
    uv_timer_start(
        &stale_timer,
        on_stale_timer,
        CONNECTION_TIMEOUT * 1000,
        CONNECTION_TIMEOUT * 1000
    );

    init_server(&loop, &server);

    uv_run(&loop, UV_RUN_DEFAULT);

    logg(INFO, "Worker shutting down");

    return 0;
}

