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

#ifdef PRCTL_NAMES
#include <sys/prctl.h>
#endif

struct conn {
    int fd;
    struct sockaddr_in addr;
    sem_t *mutex;
    unsigned int id;
};

struct worker {
    unsigned int id;
    unsigned int pid;
    int status;
    char title[256];
};

#include "utils.h"

#endif
