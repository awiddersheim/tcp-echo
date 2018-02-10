/* Number of workers to use. If left defined as <= 0, the amount of
 * CPUs on the system are used by default.
 */
#define WORKERS 0

/* Number of connection handlers per worker */
#define HANDLERS 10

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

/* Define if strerror_r returns char * */
#cmakedefine STRERROR_R_CHAR_P
