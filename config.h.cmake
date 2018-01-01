/* Number of workers */
#define WORKERS 4

/* Number of connection handlers per worker */
#define HANDLERS 10

/* Port number to listen on */
#define PORT_NUMBER 8090

/* Maximum connections to hold in backlog */
#define MAX_CONN 5

/* Logging level. Valid options are:
 * DEBUG = 1,
 * INFO  = 2,
 * WARN  = 3,
 * ERROR = 4,
 * FATAL = 5,
 */
#define LOG_LEVEL INFO

/* Define to 1 if strerror_r returns char *. */
#cmakedefine STRERROR_R_CHAR_P 1
