/* Number of workers to use. If left defined as <= 0, the amount of
 * CPUs on the system are used by default.
 */
#define WORKERS 0

/* Port number to listen on */
#define PORT_NUMBER 8090

/* Number of connections allowed in the backlog */
#define CONNECTION_BACKLOG 128

/* Logging level. Valid options are:
 * DEBUG,
 * INFO,
 * WARN,
 * ERROR,
 * FATAL
 */
#define LOG_LEVEL INFO

/* Enable timestamps in log lines. May be helpful but also
 * unnecessary in some cases as process supervisors like
 * systemd will do this.
 */
#define TIMESTAMPS

/* Time in seconds a connection is allowed to sit idle for
 * before being considered dead and timing out.
 */
#define CONNECTION_TIMEOUT 10

/* The number of simultaneous connections a worker can handle. A setting
 * of <= 0 means an unlimited number of simultaneous connections will
 * be handled by each worker.
 */
#define WORKER_CONNECTIONS 0

/* The maximum number of connections a worker can accept before it
 * shuts down. This isn't all that useful unless a memory leak
 * develops somewhere. A setting of <= 0 means an unlimited number of
 * connections will be handled by each worker.
 */
#define WORKER_MAX_CONNECTIONS 0

/* The linger timeout after closing client connections */
#define LINGER_TIMEOUT 10

/* Send reset packets to idle clients */
#undef SEND_RESET

/* Enable SO_KEEPALIVE on client connections */
#undef ENABLE_KEEPALIVE

/* The delay before sending keepalives */
#define KEEPALIVE_DELAY 5

/* Enable TCP_NODELAY on client connections */
#define ENABLE_NODELAY

/* Maximum length of a worker title */
#define MAX_WORKER_TITLE 16

/* Define if strerror_r returns char* */
#cmakedefine STRERROR_R_CHAR_P
