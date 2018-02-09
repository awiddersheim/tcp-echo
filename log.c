#include "main.h"

/* Used for identifying processes in logs */
char *title;

static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

void vlogg(log_level_t log_level, const char *message, va_list args)
{
    char buffer[4096];

    if (log_level < LOG_LEVEL)
        return;

    snprintf(
        buffer,
        sizeof(buffer),
        "%s (%s)(%d): %s\n",
        level_names[log_level - 1],
        title,
        getpid(),
        message
    );

    vfprintf(stdout, buffer, args);
    fflush(stdout);

    if (log_level == FATAL)
        exit(1);
}

void logg(log_level_t log_level, const char *message, ...)
{
    va_list args;

    va_start(args, message);
    vlogg(log_level, message, args);
    va_end(args);
}

void logge(log_level_t log_level, const char *message, ...)
{
    char buffer[2048];
    char error_string[1024];
    int previous_errno = errno;
    va_list args;

    xstrerror(errno, error_string, sizeof(error_string));

    snprintf(
        buffer,
        sizeof(buffer),
        "%s errno (%d) msg (%s)",
        message,
        previous_errno,
        error_string
    );

    va_start(args, message);
    vlogg(log_level, buffer, args);
    va_end(args);
}

void logguv(log_level_t log_level, int error, const char *message, ...)
{
    char buffer[2048];
    va_list args;

    snprintf(
        buffer,
        sizeof(buffer),
        "%s errno (%d) msg (%s: %s)",
        message,
        error,
        uv_err_name(error),
        uv_strerror(error)
    );

    va_start(args, message);
    vlogg(log_level, buffer, args);
    va_end(args);
}
