#include <stdio.h>

#define PIERRE_LOG_ENABLED  1
#define PIERRE_LOG_FILENAME "pierre.log"

#if PIERRE_LOG_ENABLED != 0

extern FILE *pierre_log_fd;

void pierre_log_init(void);

#define pierre_log(...) { \
    fprintf(pierre_log_fd, __VA_ARGS__); \
    fflush(pierre_log_fd); \
}

#else /* PIERRE_LOG_ENABLED */

#define pierre_log(...)          (void)0
#define pierre_log_init()     (void)0

#endif /* PIERRE_LOG_ENABLED */
