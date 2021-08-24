#ifndef __MAIN_H_
#define __MAIN_H_

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <uv.h>
#include <sds.h>
#include "error.h"
#include "log.h"

typedef enum {
    PROCESS_RUNNING,
    PROCESS_PAUSED,
    PROCESS_STOPPING,
    PROCESS_KILLED
} te_process_state_t;

typedef struct worker {
    uv_process_t child;
    uv_process_options_t options;
    unsigned int id;
    uv_pid_t pid;
    int64_t status;
    int signal;
    int alive;
    sds title;
} te_worker_t;

typedef enum {
    CONTROLLER,
    WORKER
} te_process_type_t;

#define TE_PROCESS_FIELDS           \
    te_process_state_t state;       \
    te_process_type_t process_type; \

typedef struct {
    TE_PROCESS_FIELDS
} te_process_t;

typedef struct {
    TE_PROCESS_FIELDS
    int is_listening;
    int alive_workers;
    int worker_count;
    te_worker_t *workers;
    uv_timer_t *worker_timer;
} te_controller_process_t;

typedef struct {
    TE_PROCESS_FIELDS
    unsigned int current_connections;
    unsigned int total_connections;
    uv_pid_t controller_pid;
} te_worker_process_t;

typedef struct {
    uv_tcp_t client;
    sds peer;
    time_t timeout;
    uv_shutdown_t shutdown;
} te_conn_t;

typedef enum {
    SERVER = 1,
    CLIENT = 2
} te_tcp_type_t;

#include "utils.h"

#endif
