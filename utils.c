#include "main.h"

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);

	if (ptr == NULL)
        log_errno("Could not allocate memory");

	return ptr;
}

int create_sock()
{
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        log_fatal("Could not create socket");

    return sock;
}

char *key_init(int worker_id)
{
    char *key = NULL;
    int result;
    int size;
    const char *format = "/worker-%d";

    size = snprintf(NULL, 0, format, worker_id) + 1;

    key = xmalloc(size);

    result = snprintf(key, size, format, worker_id);

    if (result < 0)
        log_fatal("Could not create semaphore key (%d)", result);

    return key;
}

void sock_setreuse(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
        log_errno("Could not set address reuse");
}

void sock_setreuse_port(int sock, int reuse)
{
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) == -1)
        log_errno("Could not set port reuse");
}

void sock_bind(int sock, int port)
{
    struct sockaddr_in addr;

    memset(&addr, 0x0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        log_errno("Could not bind to port (%d)", port);
}

void sock_listen(int sock, int maxconn, int port)
{
    if (listen(sock, maxconn) < 0)
        log_errno("Could not listen on port (%d)", port);
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
            log_errno("Could not sem_unlink() key (%s)", key);
    }

    if ((mutex = sem_open(key, O_CREAT, 0644, HANDLERS - 1)) == SEM_FAILED)
        log_errno("Could not create semaphore key (%s)", key);

    return mutex;
}

pthread_attr_t *thread_init()
{
    pthread_attr_t *attr = NULL;

    attr = xmalloc(sizeof(pthread_attr_t));

    if (pthread_attr_init(attr) != 0)
        log_errno("Could not create thread attribute");

    if (pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED) != 0)
        log_errno("Could set detached state");

    return attr;
}
