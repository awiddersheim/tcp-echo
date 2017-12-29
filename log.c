#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "log.h"

void log_write(const char *message, ...)
{
    va_list vl;
    va_start(vl, message);

    vfprintf(stdout, message, vl);
    fflush(stdout);

    va_end(vl);
}

void log_errno(const char *message, ...)
{
    char buffer[256];
    char *error_message;
    int result = 0;
    int previous_errno = errno;

    #ifdef STRERROR_R_CHAR_P
    error_message = strerror_r(previous_errno, buffer, sizeof(buffer));
    #else
    result = strerror_r(previous_errno, buffer, sizeof(buffer));

    if (result != 0) {
        result = snprintf(buffer, sizeof(buffer), "Unknown (issue with strerror() (%d))", errno);

        if (result > 0)
            result = 0;
    }

    error_message = buffer;
    #endif

    log_fatal(
        "%s errno (%d) msg (%s)",
        message,
        previous_errno,
        result == 0 ? error_message : "Unknown (issue with strerror())"
    );
}
