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
    RUNNING,
    STOPPING,
    KILLED
} te_process_state_t;

typedef struct {
    te_process_state_t state;
    int is_worker;
    uv_pid_t ppid;
    int workers_reaped;
} te_process_t;

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
