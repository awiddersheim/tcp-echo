#include "tcp-echo.h"
#include "worker.h"

void te_alloc_buffer(__attribute__((unused)) uv_handle_t *handle, size_t size, uv_buf_t *buffer)
{
    *buffer = uv_buf_init((char *) te_malloc(size), size);
}

void free_write_request(te_write_req_t *write_request)
{
    sdsfree(write_request->buffer.base);
    free(write_request);
}

te_tcp_type_t *te_init_tcp_type(te_tcp_type_t type)
{
    te_tcp_type_t *type_ptr;

    type_ptr = te_malloc(sizeof(te_tcp_type_t));
    *type_ptr = type;

    return type_ptr;
}

te_conn_t *te_init_connection(uv_loop_t *loop)
{
    te_conn_t *conn;

    conn = te_calloc(1, sizeof(te_conn_t));

    uv_tcp_init(loop, &conn->client);

    conn->timeout = time(NULL);

    conn->client.data = te_init_tcp_type(CLIENT);

    return conn;
}

uv_tcp_t *te_init_server(uv_loop_t *loop)
{
    int fd;
    int result;
    struct sockaddr_in addr;
    uv_tcp_t *server;
    te_process_t *worker_process = (te_process_t *) loop->data;

    server = te_malloc(sizeof(uv_tcp_t));

    uv_tcp_init_ex(loop, server, AF_INET);
    server->data = te_init_tcp_type(SERVER);

    uv_fileno((uv_handle_t *) server, &fd);
    te_sock_setreuse_port(fd, 1);

    if (worker_process->config.enable_nodelay && (result = uv_tcp_nodelay(server, 1)) < 0)
        te_log_uv(FATAL, result, "Could not set nodelay on socket");

    if (worker_process->config.keepalive_delay && (result = uv_tcp_keepalive(server, 1, worker_process->config.keepalive_delay)) < 0)
        te_log_uv(FATAL, result, "Could not set keepalive on socket");

    uv_ip4_addr("0.0.0.0", worker_process->config.port, &addr);

    if ((result = uv_tcp_bind(server, (const struct sockaddr *) &addr, 0)) < 0)
        te_log_uv(FATAL, result, "Could not bind to port (%d)", worker_process->config.port);

    if ((result = uv_listen((uv_stream_t *) server, worker_process->config.connection_backlog, te_on_connection)) < 0)
        te_log_uv(FATAL, result, "Could not listen for connections on (%d)", worker_process->config.port);

    return server;
}

te_write_req_t *te_init_write_request(te_conn_t *conn, sds buffer)
{
    te_write_req_t *write_request;

    write_request = te_calloc(1, sizeof(te_write_req_t));

    write_request->buffer = uv_buf_init(buffer, sdslen(buffer));
    write_request->conn = conn;

    return write_request;
}


void te_on_server_close(uv_handle_t *handle)
{
    free(handle->data);
    free(handle);
}

void te_on_connection_close(uv_handle_t *handle)
{
    te_conn_t *conn = (te_conn_t *) handle;
    te_worker_process_t *worker_process = (te_worker_process_t *) handle->loop->data;

    worker_process->current_connections--;

    if (worker_process->config.max_connections > 0 && worker_process->total_connections >= worker_process->config.max_connections)
        te_stop_process(handle->loop);

    if (
        worker_process->config.max_concurrent_connections > 0
        && worker_process->current_connections < worker_process->config.max_concurrent_connections
        && worker_process->state == PROCESS_PAUSED
    ) {
        te_log(INFO, "Worker resuming connection handling");

        te_init_server(handle->loop);

        worker_process->state = PROCESS_RUNNING;
    }

    free(conn->client.data);
    sdsfree(conn->peer);
    free(conn);
}

void te_on_connection_shutdown(uv_shutdown_t *shutdown_request, int status)
{
    te_conn_t *conn = (te_conn_t *) shutdown_request->handle;

    if (status)
        te_log_uv(WARN, status, "Could not shutdown connection from (%s)", conn->peer);

    if (!uv_is_closing((uv_handle_t *) shutdown_request->handle)) {
        te_log(INFO, "Closing connection from (%s)", conn->peer);

        uv_close((uv_handle_t *) shutdown_request->handle, te_on_connection_close);
    }
}

void te_on_worker_loop_close(uv_handle_t *handle, __attribute__((unused)) void *arg)
{
    if (uv_is_closing(handle))
        return;

    switch (handle->type) {
        case UV_TCP:
            if (*(te_tcp_type_t *) handle->data == CLIENT) {
                te_log(
                    INFO,
                    "Shutting down connection from (%s)",
                    ((te_conn_t *) handle)->peer
                );

                uv_shutdown(
                    &((te_conn_t *) handle)->shutdown,
                    (uv_stream_t *) handle,
                    te_on_connection_shutdown
                );
            } else {
                uv_close(handle, te_on_server_close);
            }

            break;
        default:
            uv_close(handle, NULL);
            break;
    }
}

void te_on_echo_write(uv_write_t *request, int status)
{
    te_write_req_t *write_request = (te_write_req_t *) request;

    if (status)
        te_log_uv(ERROR, status, "Could not write to (%s)", write_request->conn->peer);

    free_write_request(write_request);
}

void te_on_connection_timeout(uv_write_t *request, int status)
{
    int fd;
    te_write_req_t *write_request = (te_write_req_t *) request;
    te_worker_process_t *worker_process = (te_worker_process_t *) request->handle->loop->data;

    if (status)
        te_log_uv(ERROR, status, "Could not write to (%s)", write_request->conn->peer);

    uv_fileno((uv_handle_t *) write_request->conn, &fd);

    if (worker_process->config.enable_send_reset)
        te_sock_set_linger(fd, 1, 0);

    if (!uv_is_closing((uv_handle_t *) write_request->conn)) {
        te_log(INFO, "Closing connection from (%s)", write_request->conn->peer);

        uv_close((uv_handle_t *) write_request->conn, te_on_connection_close);
    }

    free_write_request(write_request);
}

void te_on_echo_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buffer)
{
    te_conn_t *conn = (te_conn_t *) stream;
    te_write_req_t *write_request;
    sds new_buffer;

    if (nread > 0) {
        new_buffer = sdsnewlen(buffer->base, nread);

        write_request = te_init_write_request(conn, new_buffer);

        uv_write(
            (uv_write_t *) write_request,
            (uv_stream_t *) conn,
            &write_request->buffer,
            1,
            te_on_echo_write
        );

        conn->timeout = time(NULL);
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            te_log_uv(ERROR, (int) nread, "Could not read from (%s)", conn->peer);
            te_log(INFO, "Closing connection from (%s)", conn->peer);
        } else {
            te_log(INFO, "Connection closed by (%s)", conn->peer);
        }

        uv_close((uv_handle_t *) conn, te_on_connection_close);
    }

    free(buffer->base);
}

void te_on_stale_walk(uv_handle_t *handle, __attribute__((unused)) void *arg)
{
    te_conn_t *conn;
    double difference;
    sds buffer;
    te_write_req_t *write_request;
    te_worker_process_t *worker_process = (te_worker_process_t *) handle->loop->data;

    if (uv_is_closing(handle)
        || handle->type != UV_TCP
        || handle->data == NULL
        || *(te_tcp_type_t *) handle->data != CLIENT)
        return;

    conn = (te_conn_t *) handle;

    difference = difftime(time(NULL), conn->timeout);

    te_log(DEBUG, "Connection from (%s) has been idle for (%.0f) second(s)", conn->peer, difference);

    if (difference > worker_process->config.idle_timeout) {
        te_log(
            INFO,
            "Connection has timed out from (%s) after (%d) second(s)",
            conn->peer,
            worker_process->config.idle_timeout
        );

        buffer = sdsnew("Closing connection due to inactivity\n");

        write_request = te_init_write_request(conn, buffer);

        uv_write(
            (uv_write_t *) write_request,
            (uv_stream_t *) conn,
            &write_request->buffer,
            1,
            te_on_connection_timeout
        );
    }
}

void te_on_stale_timer(uv_timer_t *timer)
{
    uv_walk(timer->loop, te_on_stale_walk, NULL);
}

void te_on_parent_timer(uv_timer_t *timer)
{
    uv_pid_t ppid;
    te_worker_process_t *worker_process = (te_worker_process_t *) timer->loop->data;

    ppid = uv_os_getppid();

    if (ppid != worker_process->controller_pid) {
        te_log(
            ERROR,
            "Detected parent change from (%d) to (%d)",
            worker_process->controller_pid,
            ppid
        );

        te_stop_process(timer->loop);

        uv_close((uv_handle_t *) timer, NULL);
    }
}

void te_on_connection(uv_stream_t *server, int status)
{
    int fd;
    int result;
    te_conn_t *conn;
    te_worker_process_t *worker_process = (te_worker_process_t *) server->loop->data;

    if (status < 0) {
        te_log_uv(ERROR, status, "Could not handle new connection");
        return;
    }

    conn = te_init_connection(server->loop);

    if ((result = uv_accept(server, (uv_stream_t *) conn)) >= 0) {
        conn->peer = te_getpeername((uv_tcp_t *) conn);

        te_log(INFO, "Handling connection from (%s)", conn->peer);

        uv_fileno((uv_handle_t *) conn, &fd);

        te_sock_set_linger(
            fd,
            1,
            worker_process->config.linger_timeout
        );

        te_sock_set_tcp_linger(
            fd,
            worker_process->config.linger_timeout
        );

        worker_process->current_connections++;
        worker_process->total_connections++;

        if (
            worker_process->config.max_connections > 0
            && worker_process->total_connections >= worker_process->config.max_connections
        ) {
            te_log(
                INFO,
                "Worker has handled max total connections (%d)",
                worker_process->total_connections
            );

            uv_close((uv_handle_t *) server, te_on_server_close);
        }

        if (
            worker_process->config.max_concurrent_connections > 0
            && worker_process->current_connections >= worker_process->config.max_concurrent_connections
            && !uv_is_closing((uv_handle_t *) server)
        ) {
            te_log(
                WARN,
                "Worker pausing while handling max simultaneous connections (%d)",
                worker_process->current_connections
            );

            worker_process->state = PROCESS_PAUSED;

            uv_close((uv_handle_t *) server, te_on_server_close);
        }

        uv_read_start((uv_stream_t *) conn, te_alloc_buffer, te_on_echo_read);
    } else {
        te_log_uv(ERROR, result, "Could not accept new connection");

        uv_close((uv_handle_t *) conn, te_on_connection_close);
    }
}

int te_set_worker_process_title(sds worker_id)
{
    int result;

    if (!sdslen(worker_id)) {
        result = te_set_process_title("tcp-echo[wrk]");
    } else {
        result = te_set_process_title("tcp-echo[wrk%s]", worker_id);
    }

    return result;
}

char *te_set_worker_log_title(char *worker_id)
{
    sds title;

    if (!sdslen(worker_id)) {
        title = te_set_log_title("worker-%d", uv_os_getpid());
    } else {
        title = te_set_log_title("worker-%s", worker_id);
    }

    return title;
}

uv_pid_t te_get_controller_pid()
{
    sds cpid;
    uv_pid_t controller_pid;

    cpid = te_os_getenv("TE_CONTROLLER_PID");

    if (sdslen(cpid)) {
        controller_pid = strtol(cpid, NULL, 10);

        te_log(DEBUG, "Found passed controller PID (%d)", controller_pid);
    } else {
        controller_pid = uv_os_getppid();

        te_log(DEBUG, "No controller PID was passed, using (%d)", controller_pid);
    }

    sdsfree(cpid);

    return controller_pid;
}

int te_worker_main(int argc, char *argv[])
{
    int result;
    sds worker_id;
    te_worker_process_t worker_process = { 0 };
    uv_loop_t loop;
    uv_signal_t sigquit;
    uv_signal_t sigterm;
    uv_signal_t sigint;
    uv_signal_t sigusr1;
    uv_timer_t stale_timer;
    uv_timer_t parent_timer;

    te_init_process((te_process_t *) &worker_process, WORKER);
    te_init_config(argc, argv, &worker_process.config);

    worker_id = te_os_getenv("TE_WORKER_ID");

    te_set_worker_log_title(worker_id);
    argv = uv_setup_args(argc, argv);
    te_set_worker_process_title(worker_id);

    worker_process.controller_pid = te_get_controller_pid();

    te_log(INFO, "Worker created");

    if ((result = uv_loop_init(&loop)) < 0)
        te_log_uv(FATAL, result, "Could not create worker loop");

    loop.data = &worker_process;

    te_init_signal(&loop, &sigquit, te_signal_recv, SIGQUIT);
    te_init_signal(&loop, &sigterm, te_signal_recv, SIGTERM);
    te_init_signal(&loop, &sigint, te_signal_recv, SIGINT);
    te_init_signal(&loop, &sigusr1, te_signal_recv, SIGUSR1);

    uv_timer_init(&loop, &stale_timer);
    uv_timer_start(
        &stale_timer,
        te_on_stale_timer,
        worker_process.config.idle_timeout * 1000,
        3000
    );

    uv_timer_init(&loop, &parent_timer);
    uv_timer_start(
        &parent_timer,
        te_on_parent_timer,
        0,
        1000
    );

    te_init_server(&loop);

    uv_run(&loop, UV_RUN_DEFAULT);

    te_log(INFO, "Worker shutting down");

    te_close_loop(&loop, te_on_worker_loop_close);

    te_log(INFO, "All done");

    sdsfree(worker_id);
    te_free_log_title();

    return 0;
}
