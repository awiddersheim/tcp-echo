#include <errno.h>
#include "error.h"

void log_write(const char *message, ...) __attribute__((format (printf, 1, 2)));

#define log_info(M, ...) log_write("[INFO]: " M "\n", ##__VA_ARGS__);
#define log_error(M, ...) log_write("[ERROR]: " M "\n", ##__VA_ARGS__);
#define log_fatal(M, ...) do {\
    log_write("[FATAL]: " M "\n", ##__VA_ARGS__); \
    exit(1); \
} while(0)
#define log_errno(M, ...) do {\
    char buffer[1024]; \
    int previous_errno = errno; \
    strerror_x(errno, buffer, sizeof(buffer)); \
    log_fatal(M " errno (%d) msg (%s)", ##__VA_ARGS__, previous_errno, buffer); \
    exit(1); \
} while(0)
