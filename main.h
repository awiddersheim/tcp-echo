#ifndef __MAIN_H_
#define __MAIN_H_

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include "config.h"
#include "error.h"
#include "log.h"

struct handler_conn {
    int fd;
    struct sockaddr_in addr;
    sem_t *mutex;
};

struct worker {
    unsigned int id;
    unsigned int pid;
    int status;
    char title[256];
};

#include "utils.h"

#endif
