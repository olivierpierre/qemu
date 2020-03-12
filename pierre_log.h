#include <stdio.h>
#include <sys/time.h>

#define PIERRE_LOG_ENABLED  1
#define PIERRE_LOG_FILENAME "qemu-events.log"

#if PIERRE_LOG_ENABLED != 0

extern FILE *pierre_log_fd;
extern struct timeval pierre_log_init_time;

void pierre_log_init(void);

#define pierre_log(...) { \
    struct timeval log_time, log_time_offset; \
    gettimeofday(&log_time, NULL); \
    timersub(&log_time, &pierre_log_init_time, &log_time_offset); \
    fprintf(pierre_log_fd, "%ld.%06ld:", log_time_offset.tv_sec, log_time_offset.tv_usec); \
    fprintf(pierre_log_fd, __VA_ARGS__); \
    fflush(pierre_log_fd); \
}

#define pl_timing_start() struct timeval start, stop, res; \
    gettimeofday(&start, NULL)
#define pl_timing_stop() gettimeofday(&stop, NULL); \
    timersub(&stop, &start, &res); \
    pierre_log("%s:%ld.%06ld\n", __FUNCTION__, res.tv_sec, res.tv_usec)

#else /* PIERRE_LOG_ENABLED */

#define pierre_log(...)          (void)0
#define pierre_log_init()     (void)0

#endif /* PIERRE_LOG_ENABLED */
