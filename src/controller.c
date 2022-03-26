#include "tcp-echo.h"
#include "controller.h"

extern char **environ;

void te_free_worker(te_worker_t *worker)
{
    int i;

    for (i = 0; worker->options.env[i] != NULL; i++)
        sdsfree(worker->options.env[i]);

    free(worker->options.env);
    free(worker->options.stdio);
    free(worker->options.cpumask);
    sdsfree(worker->title);
}

char **te_init_worker_env()
{
    /* NOTE(awiddersheim): Want to pass on the original environment used
     * when the controller process was spawned but also be able to include
     * additional variables that only get passed on to the worker. To do
     * that, copy the environment to a new location for use in the worker.
     */
    int i;
    char **envp;

    for (i = 0; environ[i] != NULL; i++)
        continue;

    envp = te_malloc(sizeof(char *) * (i + 1));

    for (i = 0; environ[i] != NULL; i++)
        envp[i] = sdsnew(environ[i]);

    envp[i] = NULL;

    return envp;
}

void te_init_worker(te_worker_t *worker, int id, int cpu, char *argv[])
{
    int stdio_count = 3;
    int cpumask_size;

    worker->id = id;

    worker->title = sdscatprintf(sdsempty(), "worker-%d", id);

    worker->options.exit_cb = te_on_worker_exit;
    worker->options.file = "tcp-echo";

    worker->options.args = argv;

    worker->options.env = te_init_worker_env();
    te_set_worker_env(&worker->options.env, "TE_WORKER_ID", "%d", worker->id);
    te_set_worker_env(&worker->options.env, "TE_CONTROLLER_PID", "%d", uv_os_getpid());

    worker->options.stdio = te_malloc(sizeof(uv_stdio_container_t) * stdio_count);
    worker->options.stdio_count = stdio_count;
    worker->options.stdio[0].flags = UV_IGNORE;
    worker->options.stdio[1].flags = UV_INHERIT_FD;
    worker->options.stdio[1].data.file = 1;
    worker->options.stdio[2].flags = UV_INHERIT_FD;
    worker->options.stdio[2].data.file = 2;

    if (cpu >= 0  && (cpumask_size = uv_cpumask_size()) >= 0) {
        worker->options.cpumask = te_calloc(cpumask_size, sizeof(char *));

        worker->options.cpumask[cpu] = 1;
        worker->options.cpumask_size = cpumask_size;
    }

}

void te_init_workers(te_controller_process_t *controller_process, char *argv[])
{
    int cpu;
    int cpu_count;
    uv_cpu_info_t *cpu_info;
    int result;
    int workers;

    if ((result = uv_cpu_info(&cpu_info, &cpu_count)) < 0)
        te_log_uv(FATAL, result, "Could not determine number of CPUs");

    uv_free_cpu_info(cpu_info, cpu_count);

    if (controller_process->config.workers > 0) {
        controller_process->worker_count = controller_process->config.workers;
    } else {
        controller_process->worker_count = cpu_count;
    }

    controller_process->workers = te_calloc(controller_process->worker_count, sizeof(te_worker_t));

    for (workers = 0, cpu = 0; workers < controller_process->worker_count;) {
        te_init_worker(&controller_process->workers[workers], workers + 1, cpu, argv);

        cpu++;

        if (cpu >= cpu_count)
            cpu = 0;

        workers++;
    }
}

void te_on_controller_loop_close(uv_handle_t *handle, __attribute__((unused)) void *arg)
{
    if (uv_is_closing(handle))
        return;

    uv_close(handle, NULL);
}

void te_on_worker_close(uv_handle_t *handle)
{
    te_worker_t *worker = (te_worker_t *) handle;
    te_controller_process_t *controller_process = (te_controller_process_t *) handle->loop->data;

    te_log(
        ((controller_process->state == PROCESS_RUNNING) ? WARN : INFO),
        "Worker (%s) with pid (%d) exited with (%ld)",
        worker->title,
        worker->pid,
        (long) worker->status
    );

    controller_process->alive_workers--;

    if (controller_process->state != PROCESS_RUNNING) {
        te_free_worker(worker);
    } else {
        uv_timer_again(controller_process->worker_timer);
    }
}

void te_on_worker_exit(uv_process_t *process, int64_t status, int signal)
{
    te_worker_t *worker = (te_worker_t *) process;

    worker->status = status;
    worker->signal = signal;
    worker->alive = 0;

    uv_close((uv_handle_t *) process, te_on_worker_close);
}

void te_set_worker_env(char ***env, const char *name, const char *fmt, ...)
{
    int i;
    char **newenv;
    va_list args;

    for (i = 0; (*env)[i] != NULL; i++)
        continue;

    newenv = te_realloc(*env, sizeof(char *) * (i + 2));

    newenv[i] = sdscatprintf(sdsempty(), "%s=", name);

    va_start(args, fmt);
    newenv[i] = sdscatvprintf(newenv[i], fmt, args);
    va_end(args);

    newenv[++i] = NULL;

    *env = newenv;
}

int te_update_path()
{
    int result;
    sds path;
    sds newpath;

    path = te_os_getenv("PATH");

    // *INDENT-OFF*
    newpath = sdscatprintf(
        sdsempty(),
        path ? "%s:." : "%s.",
        path ? path : ""
    );
    // *INDENT-ON*

    te_log(DEBUG, "Setting PATH to (%s)", newpath);

    if ((result = uv_os_setenv("PATH", newpath)) < 0)
        te_log_uv(WARN, result, "Could not setup path");

    sdsfree(newpath);
    sdsfree(path);

    return result;
}

int te_spawn_worker(uv_loop_t *loop, te_worker_t *worker)
{
    int result;

    te_log(INFO, "Creating worker (%s)", worker->title);

    if ((result = uv_spawn(loop, (uv_process_t *) worker, &worker->options)) < 0) {
        te_log_uv(ERROR, result, "Could not spawn worker (%d)", worker->id);
        return result;
    }

    worker->pid = uv_process_get_pid((uv_process_t *) worker);
    worker->alive = 1;

    te_log(INFO, "Created worker (%s) with pid (%d)", worker->title, worker->pid);

    return 0;
}

int te_spawn_workers(uv_loop_t *loop, te_controller_process_t *controller_process)
{
    int i;
    int workers = 0;

    if (controller_process->state != PROCESS_RUNNING || controller_process->worker_count == controller_process->alive_workers)
        return 0;

    for (i = 0; i < controller_process->worker_count; i++) {
        if (controller_process->workers[i].alive) {
            continue;
        }

        if (!te_spawn_worker(loop, &controller_process->workers[i])) {
            if (!controller_process->is_listening) {
                controller_process->is_listening = 1;

                te_log(INFO, "Listening on 0.0.0.0:%d", controller_process->config.port);
            }

            workers++;
        }
    }

    controller_process->alive_workers += workers;

    return workers;
}

void te_on_worker_timer(uv_timer_t *timer)
{
    te_controller_process_t *controller_process = (te_controller_process_t *) timer->loop->data;

    te_spawn_workers(timer->loop, controller_process);

    if (controller_process->worker_count == controller_process->alive_workers)
        uv_timer_stop(timer);
}

int te_controller_main(int argc, char *argv[])
{
    int result;
    te_controller_process_t controller_process = { 0 };
    uv_loop_t loop;
    uv_signal_t sigquit;
    uv_signal_t sigterm;
    uv_signal_t sigint;
    uv_signal_t sigusr1;
    uv_timer_t worker_timer;

    te_init_process((te_process_t *) &controller_process, CONTROLLER);
    te_init_config(argc, argv, &controller_process.config);

    te_set_log_title("controller");
    argv = uv_setup_args(argc, argv);
    te_set_process_title("tcp-echo[ctrlr]");

    if ((result = uv_loop_init(&loop)) < 0)
        te_log_uv(FATAL, result, "Could not create controller loop");

    loop.data = &controller_process;

    te_init_signal(&loop, &sigquit, te_signal_recv, SIGQUIT);
    te_init_signal(&loop, &sigterm, te_signal_recv, SIGTERM);
    te_init_signal(&loop, &sigint, te_signal_recv, SIGINT);
    te_init_signal(&loop, &sigusr1, te_signal_recv, SIGUSR1);

    te_update_path();

    te_init_workers(&controller_process, argv);

    uv_timer_init(&loop, &worker_timer);
    uv_timer_start(
        &worker_timer,
        te_on_worker_timer,
        0,
        1000
    );

    controller_process.worker_timer = &worker_timer;

    te_log(INFO, "Starting (%d) workers", controller_process.worker_count);

    uv_run(&loop, UV_RUN_DEFAULT);

    te_log(INFO, "controller shutting down");

    te_propagate_signal(&controller_process, SIGTERM);

    while (controller_process.state != PROCESS_KILLED && controller_process.alive_workers)
        uv_run(&loop, UV_RUN_ONCE);

    if (controller_process.state == PROCESS_KILLED) {
        te_log(
            WARN,
            "controller was killed before stopping all (%d) workers, only stopped (%d)",
            controller_process.worker_count,
            controller_process.worker_count - controller_process.alive_workers
        );
    }

    free(controller_process.workers);

    te_close_loop(&loop, te_on_controller_loop_close);

    te_log(INFO, "All done");

    te_free_log_title();

    return 0;
}
