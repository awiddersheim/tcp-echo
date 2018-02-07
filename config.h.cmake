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

/* Define if strerror_r returns char *. */
#cmakedefine STRERROR_R_CHAR_P

/* Define if pthread_(get|set)name_np() are available */
#cmakedefine PTHREAD_NAMES

/* Define if SET_NAME for prctl() is available */
#cmakedefine PRCTL_NAMES
