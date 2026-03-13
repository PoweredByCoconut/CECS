#include "util.h"
#include <sys/time.h>
#include <string.h>

double current_time_secs(void) {
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_sec + now.tv_usec / 1000000.0;
}
