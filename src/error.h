#ifndef __ERROR_H_
#define __ERROR_H_

#include "tcp-echo.h"

void te_strerror(int err, char *buffer, size_t size);

#endif
