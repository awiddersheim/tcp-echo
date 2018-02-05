#include "main.h"

int create_sock()
{
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        logg(FATAL, "Could not create socket");

    return sock;
}

char *key_init(int worker_id)
{
    char *key = NULL;

    xasprintf(&key, "/worker-%d", worker_id);

    return key;
}

void sock_setreuse(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
        logge(FATAL, "Could not set address reuse");
}

void sock_setreuse_port(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
        logge(FATAL, "Could not set port reuse");
}

void sock_bind(int sock, int port)
{
    struct sockaddr_in addr;

    memset(&addr, 0x0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        logge(FATAL, "Could not bind to port (%d)", port);
}

void sock_listen(int sock, int maxconn, int port)
{
    if (listen(sock, maxconn) < 0)
        logge(FATAL, "Could not listen on port (%d)", port);
}

int server_init(int port, int maxconn)
{
    int sock;

    sock = create_sock();

    sock_setreuse(sock, 1);
    sock_setreuse_port(sock, 1);

    sock_bind(sock, port);
    sock_listen(sock, maxconn, port);

    return sock;
}

sem_t *semaphore_init(char *key)
{
    sem_t *mutex;

    /* NOTE(awiddersheim): Cleanup any previous semaphores with the same
     * name.
     */
    if (sem_unlink(key) == -1) {
        /* NOTE(awiddersheim): Unlinking a semaphore which might not have been
         * opened on OSX seems to return EINVAL. Haven't been able to find any
         * documentation to support this though.
         */
        if (errno != ENOENT && errno != EINVAL)
            logge(FATAL, "Could not sem_unlink() key (%s)", key);
    }

    if ((mutex = sem_open(key, O_CREAT, 0644, HANDLERS - 1)) == SEM_FAILED)
        logge(FATAL, "Could not create semaphore key (%s)", key);

    return mutex;
}

pthread_attr_t *thread_init()
{
    pthread_attr_t *attr = NULL;

    attr = xmalloc(sizeof(pthread_attr_t));

    if (pthread_attr_init(attr) != 0)
        logge(FATAL, "Could not create thread attribute");

    if (pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED) != 0)
        logge(FATAL, "Could set detached state");

    return attr;
}

int init_worker(struct worker *worker, int id)
{
    int result;

    worker->id = id;
    worker->status = -1;
    worker->pid = -1;

    result = snprintf(worker->title, sizeof(worker->title), "worker-%d", id);

    if (result < 0) {
        logge(ERROR, "Could not write worker title for (worker-%d)", id);
        return -1;
    } else if ((unsigned long)result >= sizeof(worker->title)) {
        logg(WARN, "Could not write entire worker title for (worker-%d)", id);
    }

    return 0;
}

int update_worker_pid(struct worker *worker, int pid)
{
    worker->pid = pid;

    return 0;
}

void set_thread_name(const char *name, ...)
{
    va_list args;
    char buffer[16];

    va_start(args, name);
    vsnprintf(buffer, sizeof(buffer), name, args);
    va_end(args);

    #ifdef PTHREAD_NAMES
    pthread_setname_np(buffer);
    #elif defined(PRCTL_NAMES)
    prctl(PR_SET_NAME, buffer);
    #else
    logg(WARN, "No method to set thread name (%s)", buffer);
    #endif
}

void get_thread_name(char *name, __attribute__((unused)) size_t size)
{
    #ifdef PTHREAD_NAMES
    pthread_getname_np(pthread_self(), name, size);
    #elif defined(PRCTL_NAMES)
    prctl(PR_GET_NAME, name);
    #else
    logg(WARN, "No method to get thread name (%s)", name);
    #endif
}
