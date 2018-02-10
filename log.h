#ifndef __LOG_H_
#define __LOG_H_

#include "main.h"

extern char *title;

typedef enum {
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    FATAL = 5
} log_level_t;

static const char *level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

void vlogg(log_level_t log_level, const char *message, va_list args);
void logg(log_level_t log_level, const char *message, ...);
void logge(log_level_t log_level, const char *message, ...);
void logguv(log_level_t log_level, int error , const char *message, ...);

#endif
