#ifndef __WORKER_H_
#define __WORKER_H_

typedef struct {
    uv_write_t request;
    uv_buf_t buffer;
    te_conn_t *conn;
} te_write_req_t;

void te_alloc_buffer(__attribute__((unused)) uv_handle_t *handle, size_t size, uv_buf_t *buffer);
void free_write_request(te_write_req_t *write_request);

te_tcp_type_t *te_init_tcp_type(te_tcp_type_t type);
te_conn_t *te_init_connection(uv_loop_t *loop);
uv_tcp_t *te_init_server(uv_loop_t *loop);
te_write_req_t *te_init_write_request(te_conn_t *conn, sds buffer);

void te_on_server_close(uv_handle_t *handle);
void te_on_connection_close(uv_handle_t *handle);
void te_on_connection_shutdown(uv_shutdown_t *shutdown_request, int status);
void te_on_worker_loop_close(uv_handle_t *handle, __attribute__((unused)) void *arg);
void te_on_echo_write(uv_write_t *request, int status);
void te_on_connection_timeout(uv_write_t *request, int status);
void te_on_echo_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buffer);
void te_on_stale_walk(uv_handle_t *handle, __attribute__((unused)) void *arg);
void te_on_stale_timer(uv_timer_t *timer);
void te_on_parent_timer(uv_timer_t *timer);
void te_on_connection(uv_stream_t *server, int status);

int te_set_worker_process_title(sds worker_id);
char *te_set_worker_title(char *worker_id);
void te_update_parent_pid(te_process_t *process);

#endif
