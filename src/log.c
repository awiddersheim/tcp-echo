#include "tcp-echo.h"

static sds te__log_title;
static te_log_level_t te__log_level;
static int te__log_timestamps_enabled = 0;

sds te_set_log_title(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    te__log_title = sdscatvprintf(sdsempty(), fmt, args);
    va_end(args);

    if (sdslen(te__log_title) > TE_MAX_WORKER_TITLE) {
        te_log(WARN, "The worker title (%s) was too long and has been truncated", te__log_title);

        te__log_title[TE_MAX_WORKER_TITLE] = '\0';

        sdsupdatelen(te__log_title);
    }

    return te__log_title;
}

sds te_get_log_title()
{
    return te__log_title;
}

void te_free_log_title()
{
    sdsfree(te__log_title);
}

void te_set_log_level(te_log_level_t log_level)
{
    te__log_level = log_level;
}

te_log_level_t te_get_log_level()
{
    return te__log_level;
}

void te_enable_log_timestamps()
{
    te__log_timestamps_enabled = 1;
}

int te_log_timestamps_enabled()
{
    return te__log_timestamps_enabled;
}
