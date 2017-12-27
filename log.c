#include <stdarg.h>
#include <stdio.h>

void log_write(const char *message, ...)
{
    va_list vl;
    va_start(vl, message);

    vfprintf(stdout, message, vl);
    fflush(stdout);

    va_end(vl);
}
