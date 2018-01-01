#include <errno.h>
#include "error.h"

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

void log_write(log_level log_level, const char *message, ...);

#define logg(L, M, ...) do {\
    log_write(L, "%s: " M "\n", level_names[L - 1], ##__VA_ARGS__); \
} while(0)

#define log_errno(L, M, ...) do {\
    char buffer[1024]; \
    int previous_errno = errno; \
    strerror_x(errno, buffer, sizeof(buffer)); \
    logg(L, M " errno (%d) msg (%s)", ##__VA_ARGS__, previous_errno, buffer); \
} while(0)
