#include "tcp-echo.h"

#define LOG_MAX 4096

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
    char buffer[LOG_MAX];
    char timebuffer[32];
    char timestamp[32];
    int result;

    if (log_level < LOG_LEVEL)
        return;

    timestamp[0] = '\0';

    #ifdef TIMESTAMPS
    gettimestamp(timebuffer, sizeof(timebuffer));
    snprintf(timestamp, sizeof(timestamp), "%s - ", timebuffer);
    #endif

    result = snprintf(
        buffer,
        LOG_MAX,
        "%s[%s] (%s)(%d): %s\n",
        timestamp,
        level_names[log_level - 1],
        title,
        uv_os_getpid(),
        message
    );

    /* NOTE(awiddersheim): Make sure string is newline delimited */
    if (result >= LOG_MAX)
        buffer[strlen(buffer) - 1] = '\n';

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
    char buffer[LOG_MAX];
    char error_string[1024];
    int previous_errno = errno;
    va_list args;

    xstrerror(errno, error_string, sizeof(error_string));

    snprintf(
        buffer,
        LOG_MAX,
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
    char buffer[LOG_MAX];
    va_list args;

    snprintf(
        buffer,
        LOG_MAX,
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
