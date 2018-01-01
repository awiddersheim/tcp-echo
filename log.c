#include "main.h"

void log_write(log_level log_level, const char *message, ...)
{
    va_list args;

    if (log_level < LOG_LEVEL)
        return;

    va_start(args, message);
    vfprintf(stdout, message, args);
    fflush(stdout);
    va_end(args);

    if (log_level == FATAL)
        exit(1);
}
