#include <stdlib.h>
#include <stdio.h>

void log_write(const char *message, ...) __attribute__((format (printf, 1, 2)));
void log_errno(const char *message, ...) __attribute__((format (printf, 1, 2)));

#define log_info(M, ...) log_write("[INFO]: " M "\n", ##__VA_ARGS__);
#define log_error(M, ...) log_write("[ERROR]: " M "\n", ##__VA_ARGS__);
#define log_fatal(M, ...) do {\
    log_write("[FATAL]: " M "\n", ##__VA_ARGS__); \
    exit(1); \
} while(0)
