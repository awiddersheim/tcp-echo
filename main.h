#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "log.h"
#include "utils.h"

struct handler_conn {
    int fd;
    struct sockaddr_in addr;
};
