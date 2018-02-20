#ifndef __LOG_H_
#define __LOG_H_

#include "tcp-echo.h"

#define MAX_TITLE 256

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

#define __logg(lvl, fmt, ...) do {\
    if (lvl < LOG_LEVEL) \
        break; \
    fprintf(stdout, fmt, __VA_ARGS__); \
    fflush(stdout); \
    if (lvl == FATAL) \
        exit(1); \
} while(0)

#ifdef TIMESTAMPS
#define __logg_helper2(lvl, fmt, ...) do {\
    char timestamp[24]; \
    gettimestamp(timestamp, sizeof(timestamp)); \
    __logg(lvl, "%s - " fmt, __VA_ARGS__); \
} while(0)
#else
#define __logg_helper2(__VA_ARGS__) __logg(__VA_ARGS_)
#endif

#define __logg_helper(lvl, fmt, ...) __logg_helper2(lvl, "[%s] (%s)(%d): " fmt "\n%s", timestamp, log_levels[lvl - 1], title, uv_os_getpid(), __VA_ARGS__);
#define logg(...) __logg_helper(__VA_ARGS__, "")

#define __logge_helper(lvl, fmt, ...) do {\
    char buffer[1024]; \
    int previous_errno = errno; \
    xstrerror(errno, buffer, sizeof(buffer)); \
    logg(lvl, fmt "%s errno (%d) msg (%s)", __VA_ARGS__, previous_errno, buffer); \
} while(0)
#define logge(...) __logge_helper(__VA_ARGS__, "")

#define __logguv_helper(lvl, err, fmt, ...) logg(lvl, fmt "%s errno (%d) msg (%s: %s)", __VA_ARGS__, err, uv_err_name(err), uv_strerror(err));
#define logguv(...) __logguv_helper( __VA_ARGS__, "")

#endif
