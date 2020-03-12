#include "pierre_log.h"

#if PIERRE_LOG_ENABLED != 0

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

FILE *pierre_log_fd;
struct timeval pierre_log_init_time;

void pierre_log_init(void) {
    gettimeofday(&pierre_log_init_time, NULL);

    pierre_log_fd = fopen(PIERRE_LOG_FILENAME, "w+");
    if(!pierre_log_fd) {
        fprintf(stderr, "pierre_log: cannot open output file\n");
        exit(-1);
    }
}

#endif /* PIERRE_LOG_ENABLED */
