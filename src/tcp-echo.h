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
} te_process_t;

extern int is_worker;
extern uv_loop_t loop;
extern te_process_state_t process_state;

#include "utils.h"

#endif
