#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

void log_write(const char *message, ...) __attribute__((format (printf, 1, 2)));

#define log_info(M, ...) log_write("[INFO]: " M "\n", ##__VA_ARGS__);
#define log_error(M, ...) log_write("[ERROR]: " M "\n", ##__VA_ARGS__);
#define log_fatal(M, ...) do {\
    fprintf(stdout, "[FATAL]: " M "\n", ##__VA_ARGS__); \
    exit(1); \
} while(0)
#define log_errno(M, ...) log_fatal(M " errno (%d) msg (%s)\n", ##__VA_ARGS__, errno, strerror(errno));
