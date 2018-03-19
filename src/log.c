#include "tcp-echo.h"

static sds title;

sds te_set_title(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    title = sdscatvprintf(sdsempty(), fmt, args);
    va_end(args);

    if (sdslen(title) > MAX_WORKER_TITLE) {
        te_log(WARN, "The worker title (%s) was too long and will be truncated", title);

        title[MAX_WORKER_TITLE] = '\0';

        sdsupdatelen(title);
    }

    return title;
}

sds te_get_title()
{
    return title;
}

void te_free_title()
{
    sdsfree(title);
}
