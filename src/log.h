#ifndef __LOG_H_
#define __LOG_H_

#include "tcp-echo.h"

typedef enum {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
} te_log_level_t;

static const char *te_log_levels[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
    NULL
};

sds te_set_log_title(const char *fmt, ...);
sds te_get_log_title();
void te_free_log_title();
void te_set_log_level(te_log_level_t log_level);
te_log_level_t te_get_log_level();
void te_enable_log_timestamps();
int te_log_timestamps_enabled();

#define te__log(lvl, fmt, ...) do {\
    if (lvl < te_get_log_level()) \
        break; \
    fprintf(stdout, fmt, __VA_ARGS__); \
    fflush(stdout); \
    if (lvl == FATAL) \
        exit(1); \
} while(0)

#define te__log_helper2(lvl, fmt, ...) do {\
    if (te_log_timestamps_enabled()) { \
        char timestamp[24]; \
        te_gettimestamp(timestamp, sizeof(timestamp)); \
        te__log(lvl, "%s - " fmt, timestamp, __VA_ARGS__); \
    } else { \
        te__log(lvl, fmt, __VA_ARGS__); \
    } \
} while(0)

#define te__log_helper(lvl, fmt, ...) te__log_helper2(lvl, "[%s] (%s)(%d): " fmt "%s\n", te_log_levels[lvl - 1], te_get_log_title(), uv_os_getpid(), __VA_ARGS__);
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
