#ifndef __CONFIG_H_
#define __CONFIG_H_

typedef struct {
    int workers;
    int connection_backlog;
    int port;
    unsigned int max_connections;
    unsigned int max_concurrent_connections;
    int idle_timeout;
    int linger_timeout;
    int keepalive_delay;
    int enable_nodelay;
    int enable_send_reset;
} te_config_t;

void te_init_config(int argc, char **argv, te_config_t *config);

#endif
