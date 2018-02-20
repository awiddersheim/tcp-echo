#include "tcp-echo.h"

uv_loop_t loop;

process_state_t process_state = RUNNING;

int is_worker = 0;

extern char **environ;

struct worker {
    uv_process_t child;
    uv_process_options_t options;
    unsigned int id;
    uv_pid_t pid;
    int64_t status;
    int signal;
    int alive;
    char title[MAX_TITLE];
};

int spawn_worker(uv_loop_t *loop, struct worker *worker);

void free_worker_env(struct worker *worker)
{
    int i;

    for (i = 0; worker->options.env[i] != NULL; i++)
        free(worker->options.env[i]);

    free(worker->options.env);
}

void set_worker_env(char ***env, char *name, char *value)
{
    int i;
    char **newenv;

    for (i = 0; (*env)[i] != NULL; i++)
        continue;

    newenv = xrealloc(*env, sizeof(char *) * (i + 2));

    xasprintf(&newenv[i], "%s=%s", name, value);

    newenv[++i] = NULL;

    *env = newenv;
}

char **init_worker_env()
{
    /* NOTE(awiddersheim): Want to pass on the original environment used
     * when the master process was spawned but also be able to include
     * additional variables that only get passed on to the worker. To do
     * that, copy the environment to a new location for use in the worker.
     */
    int i;
    char **envp;

    for (i = 0; environ[i] != NULL; i++)
        continue;

    envp = xmalloc(sizeof(char *) * (i + 1));

    for (i = 0; environ[i] != NULL; i++)
        if ((envp[i] = strdup(environ[i])) == NULL)
            logge(FATAL, "Could not initialize worker environment");

    envp[i] = NULL;

    return envp;
}

void init_worker(struct worker *worker, int id)
{
    int result;

    memset(worker, 0x0, sizeof(struct worker));

    worker->id = id;

    result = snprintf(worker->title, sizeof(worker->title), "worker-%d", id);

    if (result >= 0 && (size_t) result >= sizeof(worker->title))
        logg(WARN, "Could not write entire worker title (worker-%d)", id);
}

void on_worker_exit(uv_process_t *process, int64_t status, int signal)
{
    struct worker *worker = (struct worker *) process;

    logg(
        INFO,
        "Worker (%s) with pid (%d) exited with (%ld)",
        worker->title,
        uv_process_get_pid(process),
        (long) status
    );

    worker->status = status;
    worker->signal = signal;
    worker->alive = 0;

    free_worker_env(worker);

    if (process_state == RUNNING) {
        while (spawn_worker(&loop, worker)) {
            sleep(0.5);
        }
    }
}


int spawn_worker(uv_loop_t *loop, struct worker *worker)
{
    int result;
    uv_stdio_container_t stdio[2];
    char *args[2];

    args[0] = "tcp-echo-worker";
    args[1] = NULL;

    worker->options.exit_cb = on_worker_exit;
    worker->options.file = "tcp-echo-worker";
    worker->options.args = args;
    worker->options.env = init_worker_env();

    set_worker_env(&worker->options.env, "WORKER_TITLE", worker->title);

    worker->options.stdio = stdio;
    worker->options.stdio[0].flags = UV_INHERIT_FD;
    worker->options.stdio[0].data.fd = 1;
    worker->options.stdio[1].flags = UV_INHERIT_FD;
    worker->options.stdio[1].data.fd = 2;
    worker->options.stdio_count = 2;

    logg(INFO, "Creating worker (%s)", worker->title);

    if ((result = uv_spawn(loop, (uv_process_t *) worker, &worker->options)) < 0) {
        logguv(ERROR, result, "Could not spawn worker (%d)", worker->id);
        return result;
    }

    worker->pid = uv_process_get_pid((uv_process_t *) worker);
    worker->alive = 1;

    logg(INFO, "Created worker (%s) with pid (%d)", worker->title, worker->pid);

    return 0;
}

int update_path()
{
    int result;
    char *path;
    char *newpath;

    result = os_getenv("PATH", &path);

    xasprintf(
        &newpath,
        result ? "%s." : "%s:.",
        result ? "" : path
    );

    logg(DEBUG, "Setting PATH to (%s)", newpath);

    if ((result = uv_os_setenv("PATH", newpath)) < 0)
        logguv(WARN, result, "Could not setup path");

    free(newpath);
    free(path);

    return result;
}

int main(int argc, char *argv[])
{
    int i;
    int result;
    struct worker *workers;
    uv_cpu_info_t *cpu_info;
    int cpu_count;
    uv_signal_t sigquit;
    uv_signal_t sigterm;
    uv_signal_t sigint;

    initproctitle(argc, argv);
    setproctitle("tcp-echo", "master");
    snprintf(title, sizeof(title), "master");

    if ((result = uv_loop_init(&loop)) < 0)
        logguv(FATAL, result, "Could not create master loop");

    uv_signal_init(&loop, &sigquit);
    uv_signal_init(&loop, &sigterm);
    uv_signal_init(&loop, &sigint);
    uv_signal_start(&sigquit, signal_recv, SIGQUIT);
    uv_signal_start(&sigterm, signal_recv, SIGTERM);
    uv_signal_start(&sigint, signal_recv, SIGINT);

    update_path();

    if ((result = uv_cpu_info(&cpu_info, &cpu_count)) < 0)
        logguv(FATAL, result, "Could not determine number of CPUs");

    uv_free_cpu_info(cpu_info, cpu_count);

    #if defined(WORKERS) && WORKERS > 0
    cpu_count = WORKERS;
    #endif

    logg(INFO, "Starting (%d) workers", cpu_count);

    workers = xmalloc(sizeof(struct worker) * cpu_count);

    /* TODO(awiddersheim): Pin each worker to it's own CPU. */
    for (i = 0; i < cpu_count;) {
        init_worker(&workers[i], i + 1);

        if (spawn_worker(&loop, &workers[i]))
            continue;

        i++;
    }

    logg(INFO, "Listening on 0.0.0.0:%d", PORT_NUMBER);

    uv_run(&loop, UV_RUN_DEFAULT);

    logg(INFO, "Master shutting down");

    /* TODO(awiddersheim) Schedule CPU affinity per worker */
    for (i = 0; i < cpu_count && process_state != KILLED;) {
        if (workers[i].alive) {
            logg(INFO, "Terminating (worker-%d) with pid (%d)", workers[i].id, workers[i].pid);

            uv_kill(workers[i].pid, SIGTERM);

            uv_run(&loop, UV_RUN_ONCE);

            if (workers[i].alive)
                continue;
        }

        i++;
    }

    if (process_state == KILLED) {
        logg(WARN, "Master was killed before stopping all workers");
    } else {
        free(workers);
    }

    logg(INFO, "All done");

    return 0;
}
