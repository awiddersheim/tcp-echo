#ifndef __UTILS_H_
#define __UTILS_H_

#include "main.h"

int create_sock();
char *key_init(int worker_id);
void set_setreuse(int sock, int reuse);
void set_setreuse_port(int sock, int reuse);
void sock_bind(int sock, int port);
void sock_listen(int sock, int port, int maxconn);
int server_init(int port, int maxconn);
sem_t *semaphore_init(char *key);
pthread_attr_t *thread_init();
int init_worker(struct worker *worker, int id);
int update_worker_pid(struct worker *worker, int pid);

void initproctitle (int argc, char **argv);
void setproctitle (const char *prog, const char *txt);

void set_thread_name(const char *name, ...);
void get_thread_name(char *name, size_t size);

static inline void *xmalloc(size_t size)
{
    void *ptr = malloc(size);

    if (ptr == NULL)
        log_errno(FATAL, "Could not allocate memory");

    return ptr;
}

static inline int xvasprintf(char **strp, const char *format, va_list args)
{
    int result;
    va_list args_tmp;

    /* Calculate length needed for string */
    va_copy(args_tmp, args);
    result = (vsnprintf(NULL, 0, format, args_tmp) + 1);
    va_end(args_tmp);

    /* Allocate and fill string */
    *strp = xmalloc(result);

    result = vsnprintf(*strp, result, format, args);

    if (result < 0)
        logg(FATAL, "Could not create string (%d)", result);

    return result;
}

static inline int xasprintf(char **strp, const char *format, ...)
{
    int result;
    va_list args;

    va_start(args, format);
    result = xvasprintf(strp, format, args);
    va_end(args);

    return result;
}

#endif
