#ifndef __LOG_H_
#define __LOG_H_

#include "tcp-echo.h"

#define MAX_TITLE 16

char title[MAX_TITLE];

typedef enum {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
} log_level_t;

static const char *log_levels[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

#define te__log(lvl, fmt, ...) do {\
    if (lvl < LOG_LEVEL) \
        break; \
    fprintf(stdout, fmt, __VA_ARGS__); \
    fflush(stdout); \
    if (lvl == FATAL) \
        exit(1); \
} while(0)

#ifdef TIMESTAMPS
#define te__log_helper2(lvl, fmt, ...) do {\
    char timestamp[24]; \
    te_gettimestamp(timestamp, sizeof(timestamp)); \
    te__log(lvl, "%s - " fmt, __VA_ARGS__); \
} while(0)
#else
#define te__log_helper2(__VA_ARGS__) te__log(__VA_ARGS_)
#endif

#define te__log_helper(lvl, fmt, ...) te__log_helper2(lvl, "[%s] (%s)(%d): " fmt "\n%s", timestamp, log_levels[lvl - 1], title, uv_os_getpid(), __VA_ARGS__);
#define te_log(...) te__log_helper(__VA_ARGS__, "")

#define te__log_errno_helper(lvl, fmt, ...) do {\
    char buffer[1024]; \
    int previous_errno = errno; \
    te_strerror(errno, buffer, sizeof(buffer)); \
    te_log(lvl, fmt "%s errno (%d) msg (%s)", __VA_ARGS__, previous_errno, buffer); \
} while(0)
#define te_log_errno(...) te__log_errno_helper(__VA_ARGS__, "")

#define te__log_uv_helper(lvl, err, fmt, ...) te_log(lvl, fmt "%s errno (%d) msg (%s: %s)", __VA_ARGS__, err, uv_err_name(err), uv_strerror(err));
#define te_log_uv(...) te__log_uv_helper( __VA_ARGS__, "")

#endif
