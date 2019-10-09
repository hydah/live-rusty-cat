#include "rtmp_tool.h"
#include <stdio.h>
#include <stdarg.h>
#include "error_code.hpp"
#include "logs/log.hpp"
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

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

int64_t srs_utils_time_ms()
{
    return srs_update_system_time_ms();
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


bool srs_is_little_endian()
{
    // convert to network(big-endian) order, if not equals,
    // the system is little-endian, so need to convert the int64
    static int little_endian_check = -1;

    if(little_endian_check == -1) {
        union {
            int32_t i;
            int8_t c;
        } little_check_union;

        little_check_union.i = 0x01;
        little_endian_check = little_check_union.c;
    }

    return (little_endian_check == 1);
}

bool srs_is_system_control_error(int error_code)
{
    return error_code == ERROR_CONTROL_RTMP_CLOSE
        || error_code == ERROR_CONTROL_REPUBLISH
        || error_code == ERROR_CONTROL_REDIRECT;
}

bool srs_is_client_gracefully_close(int error_code)
{
    return error_code == ERROR_SOCKET_READ
        || error_code == ERROR_SOCKET_READ_FULLY
        || error_code == ERROR_SOCKET_WRITE
        || error_code == ERROR_SOCKET_TIMEOUT;
}

static int64_t _srs_system_time_us_cache = 0;
static int64_t _srs_system_time_startup_time = 0;

int64_t srs_get_system_time_ms()
{
    if (_srs_system_time_us_cache <= 0) {
        srs_update_system_time_ms();
    }

    return _srs_system_time_us_cache / 1000;
}

int64_t srs_get_system_startup_time_ms()
{
    if (_srs_system_time_startup_time <= 0) {
        srs_update_system_time_ms();
    }

    return _srs_system_time_startup_time / 1000;
}

int64_t srs_update_system_time_ms()
{
    timeval now;

    if (gettimeofday(&now, NULL) < 0) {
        srs_warn("gettimeofday failed, ignore");
        return -1;
    }

    // @see: https://github.com/ossrs/srs/issues/35
    // we must convert the tv_sec/tv_usec to int64_t.
    int64_t now_us = ((int64_t)now.tv_sec) * 1000 * 1000 + (int64_t)now.tv_usec;

    // for some ARM os, the starttime maybe invalid,
    // for example, on the cubieboard2, the srs_startup_time is 1262304014640,
    // while now is 1403842979210 in ms, diff is 141538964570 ms, 1638 days
    // it's impossible, and maybe the problem of startup time is invalid.
    // use date +%s to get system time is 1403844851.
    // so we use relative time.
    if (_srs_system_time_us_cache <= 0) {
        _srs_system_time_startup_time = _srs_system_time_us_cache = now_us;
        return _srs_system_time_us_cache / 1000;
    }

    // use relative time.
    int64_t diff = now_us - _srs_system_time_us_cache;
    diff = srs_max(0, diff);
    if (diff < 0 || diff > 1000 * SYS_TIME_RESOLUTION_US) {
        srs_warn("clock jump, history=%"PRId64"us, now=%"PRId64"us, diff=%"PRId64"us", _srs_system_time_us_cache, now_us, diff);
        // @see: https://github.com/ossrs/srs/issues/109
        _srs_system_time_startup_time += diff;
    }

    _srs_system_time_us_cache = now_us;
    srs_info("clock updated, startup=%"PRId64"us, now=%"PRId64"us", _srs_system_time_startup_time, _srs_system_time_us_cache);

    return _srs_system_time_us_cache / 1000;
}

std::string srs_dns_resolve(string host)
{
    if (inet_addr(host.c_str()) != INADDR_NONE) {
        return host;
    }

    hostent* answer = gethostbyname(host.c_str());
    if (answer == NULL) {
        return "";
    }

    char ipv4[16];
    memset(ipv4, 0, sizeof(ipv4));

    // covert the first entry to ip.
    if (answer->h_length > 0) {
        inet_ntop(AF_INET, answer->h_addr_list[0], ipv4, sizeof(ipv4));
    }

    return ipv4;
}

void srs_parse_hostport(const std::string& hostport, std::string& host, int& port)
{
    size_t pos = hostport.find(":");
    if (pos != std::string::npos) {
        string p = hostport.substr(pos + 1);
        host = hostport.substr(0, pos);
        port = ::atoi(p.c_str());
    } else {
        host = hostport;
    }
}

void srs_parse_endpoint(std::string hostport, std::string& ip, int& port)
{
    ip = "0.0.0.0";

    size_t pos = std::string::npos;
    if ((pos = hostport.find(":")) != std::string::npos) {
        ip = hostport.substr(0, pos);
        string sport = hostport.substr(pos + 1);
        port = ::atoi(sport.c_str());
    } else {
        port = ::atoi(hostport.c_str());
    }
}

std::string srs_int2str(int64_t value)
{
    // len(max int64_t) is 20, plus one "+-."
    char tmp[22];
    snprintf(tmp, 22, "%"PRId64, value);
    return tmp;
}

std::string srs_float2str(double value)
{
    // len(max int64_t) is 20, plus one "+-."
    char tmp[22];
    snprintf(tmp, 22, "%.2f", value);
    return tmp;
}

std::string srs_bool2switch(bool v) {
    return v? "on" : "off";
}


std::string srs_string_replace(std::string str, std::string old_str, std::string new_str)
{
    std::string ret = str;

    if (old_str == new_str) {
        return ret;
    }

    size_t pos = 0;
    while ((pos = ret.find(old_str, pos)) != std::string::npos) {
        ret = ret.replace(pos, old_str.length(), new_str);
    }

    return ret;
}

std::string srs_string_trim_end(std::string str, std::string trim_chars)
{
    std::string ret = str;

    for (int i = 0; i < (int)trim_chars.length(); i++) {
        char ch = trim_chars.at(i);

        while (!ret.empty() && ret.at(ret.length() - 1) == ch) {
            ret.erase(ret.end() - 1);

            // ok, matched, should reset the search
            i = 0;
        }
    }

    return ret;
}

std::string srs_string_trim_start(std::string str, std::string trim_chars)
{
    std::string ret = str;

    for (int i = 0; i < (int)trim_chars.length(); i++) {
        char ch = trim_chars.at(i);

        while (!ret.empty() && ret.at(0) == ch) {
            ret.erase(ret.begin());

            // ok, matched, should reset the search
            i = 0;
        }
    }

    return ret;
}

std::string srs_string_remove(std::string str, std::string remove_chars)
{
    std::string ret = str;

    for (int i = 0; i < (int)remove_chars.length(); i++) {
        char ch = remove_chars.at(i);

        for (std::string::iterator it = ret.begin(); it != ret.end();) {
            if (ch == *it) {
                it = ret.erase(it);

                // ok, matched, should reset the search
                i = 0;
            } else {
                ++it;
            }
        }
    }

    return ret;
}

bool srs_string_ends_with(string str, string flag)
{
    return str.rfind(flag) == str.length() - flag.length();
}

bool srs_string_ends_with(string str, string flag0, string flag1)
{
    return srs_string_ends_with(str, flag0) || srs_string_ends_with(str, flag1);
}

bool srs_string_ends_with(string str, string flag0, string flag1, string flag2)
{
    return srs_string_ends_with(str, flag0) || srs_string_ends_with(str, flag1) || srs_string_ends_with(str, flag2);
}

bool srs_string_ends_with(string str, string flag0, string flag1, string flag2, string flag3)
{
    return srs_string_ends_with(str, flag0) || srs_string_ends_with(str, flag1) || srs_string_ends_with(str, flag2) || srs_string_ends_with(str, flag3);
}

bool srs_string_starts_with(string str, string flag)
{
    return str.find(flag) == 0;
}

bool srs_string_starts_with(string str, string flag0, string flag1)
{
    return srs_string_starts_with(str, flag0) || srs_string_starts_with(str, flag1);
}

bool srs_string_starts_with(string str, string flag0, string flag1, string flag2)
{
    return srs_string_starts_with(str, flag0, flag1) || srs_string_starts_with(str, flag2);
}

bool srs_string_starts_with(string str, string flag0, string flag1, string flag2, string flag3)
{
    return srs_string_starts_with(str, flag0, flag1, flag2) || srs_string_starts_with(str, flag3);
}

bool srs_string_contains(string str, string flag)
{
    return str.find(flag) != string::npos;
}

bool srs_string_contains(string str, string flag0, string flag1)
{
    return str.find(flag0) != string::npos || str.find(flag1) != string::npos;
}

bool srs_string_contains(string str, string flag0, string flag1, string flag2)
{
    return str.find(flag0) != string::npos || str.find(flag1) != string::npos || str.find(flag2) != string::npos;
}

vector<string> srs_string_split(string str, string flag)
{
    vector<string> arr;

    size_t pos;
    string s = str;

    while ((pos = s.find(flag)) != string::npos) {
        if (pos != 0) {
            arr.push_back(s.substr(0, pos));
        }
        s = s.substr(pos + flag.length());
    }

    if (!s.empty()) {
        arr.push_back(s);
    }

    return arr;
}

string srs_string_min_match(string str, vector<string> flags)
{
    string match;

    size_t min_pos = string::npos;
    for (vector<string>::iterator it = flags.begin(); it != flags.end(); ++it) {
        string flag = *it;

        size_t pos = str.find(flag);
        if (pos == string::npos) {
            continue;
        }

        if (min_pos == string::npos || pos < min_pos) {
            min_pos = pos;
            match = flag;
        }
    }

    return match;
}

vector<string> srs_string_split(string str, vector<string> flags)
{
    vector<string> arr;

    size_t pos = string::npos;
    string s = str;

    while (true) {
        string flag = srs_string_min_match(s, flags);
        if (flag.empty()) {
            break;
        }

        if ((pos = s.find(flag)) == string::npos) {
            break;
        }

        if (pos != 0) {
            arr.push_back(s.substr(0, pos));
        }
        s = s.substr(pos + flag.length());
    }

    if (!s.empty()) {
        arr.push_back(s);
    }

    return arr;
}

int srs_do_create_dir_recursively(string dir)
{
    int ret = ERROR_SUCCESS;

    // stat current dir, if exists, return error.
    if (srs_path_exists(dir)) {
        return ERROR_SYSTEM_DIR_EXISTS;
    }

    // create parent first.
    size_t pos;
    if ((pos = dir.rfind("/")) != std::string::npos) {
        std::string parent = dir.substr(0, pos);
        ret = srs_do_create_dir_recursively(parent);
        // return for error.
        if (ret != ERROR_SUCCESS && ret != ERROR_SYSTEM_DIR_EXISTS) {
            return ret;
        }
        // parent exists, set to ok.
        ret = ERROR_SUCCESS;
    }

    // create curren dir.
    // for srs-librtmp, @see https://github.com/ossrs/srs/issues/213
#ifndef _WIN32
    mode_t mode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH;
    if (::mkdir(dir.c_str(), mode) < 0) {
#else
    if (::mkdir(dir.c_str()) < 0) {
#endif
        if (errno == EEXIST) {
            return ERROR_SYSTEM_DIR_EXISTS;
        }

        ret = ERROR_SYSTEM_CREATE_DIR;
        srs_error("create dir %s failed. ret=%d", dir.c_str(), ret);
        return ret;
    }
    srs_info("create dir %s success.", dir.c_str());

    return ret;
}

bool srs_bytes_equals(void* pa, void* pb, int size)
{
    uint8_t* a = (uint8_t*)pa;
    uint8_t* b = (uint8_t*)pb;

    if (!a && !b) {
        return true;
    }

    if (!a || !b) {
        return false;
    }

    for(int i = 0; i < size; i++){
        if(a[i] != b[i]){
            return false;
        }
    }

    return true;
}

int srs_create_dir_recursively(string dir)
{
    int ret = ERROR_SUCCESS;

    ret = srs_do_create_dir_recursively(dir);

    if (ret == ERROR_SYSTEM_DIR_EXISTS) {
        return ERROR_SUCCESS;
    }

    return ret;
}

bool srs_path_exists(std::string path)
{
    struct stat st;

    // stat current dir, if exists, return error.
    if (stat(path.c_str(), &st) == 0) {
        return true;
    }

    return false;
}

string srs_path_dirname(string path)
{
    std::string dirname = path;
    size_t pos = string::npos;

    if ((pos = dirname.rfind("/")) != string::npos) {
        if (pos == 0) {
            return "/";
        }
        dirname = dirname.substr(0, pos);
    }

    return dirname;
}

string srs_path_basename(string path)
{
    std::string dirname = path;
    size_t pos = string::npos;

    if ((pos = dirname.rfind("/")) != string::npos) {
        // the basename("/") is "/"
        if (dirname.length() == 1) {
            return dirname;
        }
        dirname = dirname.substr(pos + 1);
    }

    return dirname;
}

string srs_path_filename(string path)
{
    std::string filename = path;
    size_t pos = string::npos;

    if ((pos = filename.rfind(".")) != string::npos) {
        return filename.substr(0, pos);
    }

    return filename;
}

string srs_path_filext(string path)
{
    size_t pos = string::npos;

    if ((pos = path.rfind(".")) != string::npos) {
        return path.substr(pos);
    }

    return "";
}