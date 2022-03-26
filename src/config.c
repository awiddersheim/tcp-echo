#include "tcp-echo.h"

void te_usage(int exit_code, const char *fmt, ...)
{
    FILE *target = exit_code ? stderr : stdout;
    va_list args;

    if (fmt != NULL) {
        fprintf(target, "ERROR: ");
        va_start(args, fmt);
        vfprintf(target, fmt, args);
        va_end(args);
        fprintf(target, "\n\n");
    }

    fprintf(target, "tcp-echo %s\n"
        "\n"
        "Usage: tcp-echo [OPTIONS]\n"
        "  --help                       Shows this usage information.\n"
        "  --port <port>                Server port (default: 8090).\n"
        "  --workers <number>           Number of workers to use (default: 0).\n"
        "                               If set to zero, the number of CPU cores is used.\n"
        "  --connection-backlog <number> Number of connections allowed in the backlog (default: 128).\n"
        "  --idle-timeout <timeout>     The amount of time in seconds (default: 10)\n"
        "                               a connection is allowed to sit idle before it is considered dead.\n"
        "  --linger-timeout <timeout>   The linger timeout in seconds after closing a connection (default: 10).\n"
        "  --max-connections <number>   The maximum number of connections (default: 0) a worker can accept\n"
        "                               before it shuts down. THis isn't all that useful unless a memory leak develops\n"
        "                               somewhere. A setting of zero means an unlimited number of connections will be\n"
        "                               handled by each worker.\n"
        "  --max-concurrent-connections <number> The maximum number of concurrent connections (default: 0) a worker can handle.\n"
        "                               A setting of zero means an unlimited number of simultaneous connections can be handled\n"
        "                               by each worker.\n"
        "  --keepalive-delay <number>   Enable TCP keep-alive with the initial delay in seconds, ignored when zero (default: 0).\n"
        "  --enable-log-timestamps      Enable timestamps in logs. May be helpful but also uneceessary if running with a process\n"
        "                               supervisor like systemd.\n"
        "  --enable-nodelay             Enable TCP_NODELAY\n"
        "  --enable-send-reset          Enable sending RST packets to idle clients.\n"
        "  --log-level <level>          Logging level (default: INFO) can be one of DEBUG, INFO, WARN, ERROR or FATAL\n",
        TE_VERSION
    );

    exit(exit_code);
}

void te__init_config(te_config_t *config)
{
    te_set_log_level(INFO);

    config->workers = 0;
    config->connection_backlog = 128;
    config->port = 8090;
    config->max_connections = 0;
    config->max_concurrent_connections = 0;
    config->idle_timeout = 10;
    config->linger_timeout = 10;
    config->keepalive_delay = 0;
    config->enable_nodelay = 0;
    config->enable_send_reset = 0;
}

int te__set_log_level(char *log_level)
{
    for (int i = 0; te_log_levels[i] != NULL; i++) {
        if (!strcmp(te_log_levels[i], log_level)) {
            te_set_log_level((te_log_level_t) i);

            return 0;
        }
    }

    return 1;
}

void te_init_config(int argc, char **argv, te_config_t *config)
{
    int i;

    te__init_config(config);

    for (i = 1; i < argc; i++) {
        int lastarg = i == argc - 1;

        if (!strcmp(argv[i], "--help")) {
            te_usage(0, NULL);
        } else if (!strcmp(argv[i], "--workers") && !lastarg) {
            config->workers = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--port") && !lastarg) {
            config->port = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--connection-backlog") && !lastarg) {
            config->connection_backlog = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--idle-timeout") && !lastarg) {
            config->idle_timeout = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--linger-timeout") && !lastarg) {
            config->linger_timeout = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--max-connections") && !lastarg) {
            config->max_connections = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--max-concurrent-connections") && !lastarg) {
            config->max_concurrent_connections = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--keepalive-delay") && !lastarg) {
            config->keepalive_delay = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--log-level") && !lastarg) {
            if (te__set_log_level(argv[++i]) > 0) {
                te_usage(1, "Unknown log level provided");
            }
        } else if (!strcmp(argv[i], "--enable-log-timestamps")) {
            te_enable_log_timestamps();
        } else if (!strcmp(argv[i], "--enable-nodelay")) {
            config->enable_nodelay = 1;
        } else if (!strcmp(argv[i], "--enable-send-reset")) {
            config->enable_send_reset = 1;
        } else {
            if (argv[i][0] == '-') {
                te_usage(1, "Unrecognized option or bad number of args for: '%s'", argv[i]);
            } else {
                /* Likely the command name, stop here. */
                break;
            }
        }
    }

    return;
}
