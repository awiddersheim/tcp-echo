#include "tcp-echo.h"

void te_strerror(int err, char *buffer, size_t size) {
    int result = 0;

    #ifdef STRERROR_R_CHAR_P
    error_message = strerror_r(err, buffer, size);
    #else
    result = strerror_r(err, buffer, size);

    if (result != 0) {
        result = snprintf(buffer, size, "Unknown (issue with strerror() (%d:%d))", errno, result);

        if (result > 0)
            result = 0;
    }
    #endif

    if (result != 0)
        snprintf(buffer, size, "Unknown (issue with strerror())");
}
