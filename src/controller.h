#ifndef __CONTROLLER_H_
#define __CONTROLLER_H_

int te_controller_main(int argc, char *argv[]);

void te_free_worker(te_worker_t *worker);

char **te_init_worker_env();
void te_init_worker(te_worker_t *worker, int id, int cpu, char *argv[]);

void te_on_controller_loop_close(uv_handle_t *handle, __attribute__((unused)) void *arg);
void te_on_worker_close(uv_handle_t *handle);
void te_on_worker_exit(uv_process_t *process, int64_t status, int signal);

void te_set_worker_env(char ***env, const char *name, const char *fmt, ...);
int te_update_path();

int te_spawn_worker(uv_loop_t *loop, te_worker_t *worker);

#endif
