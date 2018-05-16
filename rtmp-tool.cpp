#include "rtmp-tool.h"
#include <stdio.h>
#include <stdarg.h>
#include "librtmp/srs_librtmp.h"

int LOG_LEVEL = INFO;

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
        srs_human_error(buffer);
    } else if (loglevel == WARNING) {
        srs_human_warn(buffer);
    } else {
        srs_human_trace(buffer);
    }
}

void init_loglevel(int loglevel) {
    LOG_LEVEL = loglevel;
    return;
}
