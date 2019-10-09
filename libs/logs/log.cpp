#include "log.hpp"
#include "error_code.hpp"
#include <time.h>
#include <sys/time.h>

ISrsLog::ISrsLog()
{
}

ISrsLog::~ISrsLog()
{
}

int ISrsLog::initialize()
{
    return ERROR_SUCCESS;
}

void ISrsLog::reopen()
{
}

void ISrsLog::verbose(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::info(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::trace(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::warn(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

void ISrsLog::error(const char* /*tag*/, int /*context_id*/, const char* /*fmt*/, ...)
{
}

ISrsThreadContext::ISrsThreadContext()
{
}

ISrsThreadContext::~ISrsThreadContext()
{
}

int ISrsThreadContext::generate_id()
{
    return 0;
}

int ISrsThreadContext::get_id()
{
    return 0;
}

int ISrsThreadContext::set_id(int /*v*/)
{
    return 0;
}

const char* srs_human_format_time()
{
    struct timeval tv;
    static char buf[24];

    memset(buf, 0, sizeof(buf));

    // clock time
    if (gettimeofday(&tv, NULL) == -1) {
        return buf;
    }

    // to calendar time
    struct tm* tm;
    if ((tm = localtime((const time_t*)&tv.tv_sec)) == NULL) {
        return buf;
    }

    snprintf(buf, sizeof(buf),
        "%d-%02d-%02d %02d:%02d:%02d.%03d",
        1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        (int)(tv.tv_usec / 1000));

    // for srs-librtmp, @see https://github.com/ossrs/srs/issues/213
    buf[sizeof(buf) - 1] = 0;

    return buf;
}
