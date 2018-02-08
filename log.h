#ifndef __LOG_H_
#define __LOG_H_

#include "main.h"

typedef enum {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
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

#define logg(L, M, ...) log_write(L, "%s (%s)(%d): " M "\n", level_names[L - 1], title, getpid(), ##__VA_ARGS__)

#define logge(L, M, ...) do {\
    char buffer[1024]; \
    int previous_errno = errno; \
    xstrerror(errno, buffer, sizeof(buffer)); \
    logg(L, M " errno (%d) msg (%s)", ##__VA_ARGS__, previous_errno, buffer); \
} while(0)

#define loggu(L, E, M, ...) logg(L, M " errno (%d) msg (%s: %s)", ##__VA_ARGS__, E, uv_err_name(E), uv_strerror(E))

#define loggl(L, M, ...) do {\
    int error = uv_last_error(loop); \
    logg(L, M " errno (%d) msg (%s: %s)", ##__VA_ARGS__, error, uv_err_name(error), uv_strerror(error)); \
} while(0)

#endif
