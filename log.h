#ifndef __LOG_H_
#define __LOG_H_

#include "main.h"

typedef enum {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5,
} log_level;

static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

extern char *title;

void log_write(log_level log_level, const char *message, ...);

#define logg(L, M, ...) do { \
    char thread_name[16]; \
    get_thread_name(thread_name, sizeof(thread_name)); \
    log_write(L, "%s (%s)(%s)(%d): " M "\n", level_names[L - 1], title, thread_name, getpid(), ##__VA_ARGS__); \
} while(0)

#define log_errno(L, M, ...) do {\
    char buffer[1024]; \
    int previous_errno = errno; \
    xstrerror(errno, buffer, sizeof(buffer)); \
    logg(L, M " errno (%d) msg (%s)", ##__VA_ARGS__, previous_errno, buffer); \
} while(0)

#endif
