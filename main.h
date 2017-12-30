#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include "config.h"
#include "log.h"
#include "utils.h"

struct handler_conn {
    int fd;
    struct sockaddr_in addr;
};
