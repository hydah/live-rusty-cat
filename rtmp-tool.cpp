#include "rtmp-tool.h"
#include <stdio.h>
#include <stdarg.h>
#include "librtmp/srs_librtmp.h"

int LOG_LEVEL = INFO;
#define RE_PULSE_MS 300
#define RE_PULSE_JITTER_MS 3000

void log(int loglevel, const char *format, ...)
{
    if (loglevel > LOG_LEVEL) {
        return;
    }

    char buffer[10000];
    va_list args;
    va_start(args, format);
    vsprintf(buffer,format, args);
    va_end(args);

    if (loglevel == ERROR) {
        srs_human_error("%s", buffer);
    } else if (loglevel == WARNING) {
        srs_human_warn("%s", buffer);
    } else {
        srs_human_trace("%s", buffer);
    }
}

void init_loglevel(int loglevel) {
    LOG_LEVEL = loglevel;
    return;
}

int64_t re_create(int startup_time)
{
    // if not very precise, we can directly use this as re.
    int64_t re = srs_utils_time_ms();

    // use the starttime to get the deviation
    int64_t deviation = re - startup_time;
    log(INFO, "deviation is %d ms, pulse is %d ms", (int)(deviation), (int)(RE_PULSE_MS));

    // so, we adjust time to max(0, deviation)
    // because the last pulse, we already sleeped
    int adjust = (int)(deviation);
    if (adjust > 0) {
        log(INFO, "adjust re time for %d ms", adjust);
        re -= adjust;
    } else {
        log(INFO, "no need to adjust re time");
    }

    return re;
}

void re_update(int64_t re, int32_t starttime, uint32_t time)
{
    // send by pulse algorithm.
    int64_t now = srs_utils_time_ms();
    int64_t diff = time - starttime - (now -re);
    if (diff > RE_PULSE_MS && diff < RE_PULSE_JITTER_MS) {
        usleep((useconds_t)(diff * 1000));
    }
}

void re_cleanup(int64_t re, int32_t starttime, uint32_t time)
{
    // for the last pulse, always sleep.
    // for the virtual live encoder long time publishing.
    int64_t now = srs_utils_time_ms();
    int64_t diff = time - starttime - (now -re);
    if (diff > 0) {
        log(INFO, "re_cleanup, diff=%d, start=%d, last=%d ms",
            (int)diff, starttime, time);
        usleep((useconds_t)(diff * 1000));
    }
}
