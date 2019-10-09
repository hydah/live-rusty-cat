#ifndef RTMP_TOOL_HPP
#define RTMP_TOOL_HPP

#include <sys/types.h>
#include <assert.h>
#include <string>
#include <vector>
#define srs_assert(expression) assert(expression)

// typedef unsigned long long u_int64_t;
// typedef u_int64_t uint64_t;
// typedef long long int64_t;
typedef unsigned int u_int32_t;
typedef u_int32_t uint32_t;
typedef int int32_t;
typedef unsigned char u_int8_t;
typedef u_int8_t uint8_t;
// typedef char int8_t;
typedef unsigned short u_int16_t;
typedef u_int16_t uint16_t;
typedef short int16_t;
typedef int64_t ssize_t;

#define SRS_JOBJECT_START "{"
#define SRS_JFIELD_NAME(k) "\"" << k << "\":"
#define SRS_JFIELD_OBJ(k) SRS_JFIELD_NAME(k) << SRS_JOBJECT_START
#define SRS_JFIELD_STR(k, v) SRS_JFIELD_NAME(k) << "\"" << v << "\""
#define SRS_JFIELD_ORG(k, v) SRS_JFIELD_NAME(k) << std::dec << v
#define SRS_JFIELD_BOOL(k, v) SRS_JFIELD_ORG(k, (v? "true":"false"))
#define SRS_JFIELD_NULL(k) SRS_JFIELD_NAME(k) << "null"
#define SRS_JFIELD_ERROR(ret) "\"" << "code" << "\":" << ret
#define SRS_JFIELD_CONT ","
#define SRS_JOBJECT_END "}"
#define SRS_JARRAY_START "["
#define SRS_JARRAY_END "]"

// #define SrsVideoAvcFrameTraitNALU 1
// #define SrsVideoAvcFrameTypeKeyFrame 1
// #define SrsVideoAvcFrameTypeInterFrame 2

enum log_level {
    SUM = 0,
    ERROR,
    WARNING,
    INFO,
    DEBUG,
};
extern int LOG_LEVEL;

void log(int loglevel, const char *format, ...);
void init_loglevel(int loglevel);
int64_t re_create(int startup_time);
void re_update(int64_t re, int32_t starttime, uint32_t time);
void re_cleanup(int64_t re, int32_t starttime, uint32_t time);
bool srs_is_little_endian();
#define ERROR_SUCCESS  0

// important performance options.
//#include <srs_core_performance.hpp>

// free the p and set to NULL.
// p must be a T*.
#define srs_freep(p) \
    if (p) { \
        delete p; \
        p = NULL; \
    } \
    (void)0
// please use the freepa(T[]) to free an array,
// or the behavior is undefined.
#define srs_freepa(pa) \
    if (pa) { \
        delete[] pa; \
        pa = NULL; \
    } \
    (void)0

// compare
#define srs_min(a, b) (((a) < (b))? (a) : (b))
#define srs_max(a, b) (((a) < (b))? (b) : (a))

#define SYS_TIME_RESOLUTION_US 300*1000

#define PRId64 "lld"

bool srs_is_system_control_error(int error_code);
bool srs_is_client_gracefully_close(int error_code);
/**
* get the current system time in ms.
* use gettimeofday() to get system time.
*/
int64_t srs_utils_time_ms();

// get current system time in ms, use cache to avoid performance problem
extern int64_t srs_get_system_time_ms();
extern int64_t srs_get_system_startup_time_ms();
// the deamon st-thread will update it.
extern int64_t srs_update_system_time_ms();

// dns resolve utility, return the resolved ip address.
extern std::string srs_dns_resolve(std::string host);

// split the host:port to host and port.
// @remark the hostport format in <host[:port]>, where port is optional.
extern void srs_parse_hostport(const std::string& hostport, std::string& host, int& port);

// parse the endpoint to ip and port.
// @remark hostport format in <[ip:]port>, where ip is default to "0.0.0.0".
extern void srs_parse_endpoint(std::string hostport, std::string& ip, int& port);

// parse the int64 value to string.
extern std::string srs_int2str(int64_t value);
// parse the float value to string, precise is 2.
extern std::string srs_float2str(double value);
// convert bool to switch value, true to "on", false to "off".
extern std::string srs_bool2switch(bool v);

// whether system is little endian
extern bool srs_is_little_endian();

// replace old_str to new_str of str
extern std::string srs_string_replace(std::string str, std::string old_str, std::string new_str);
// trim char in trim_chars of str
extern std::string srs_string_trim_end(std::string str, std::string trim_chars);
// trim char in trim_chars of str
extern std::string srs_string_trim_start(std::string str, std::string trim_chars);
// remove char in remove_chars of str
extern std::string srs_string_remove(std::string str, std::string remove_chars);
// whether string end with
extern bool srs_string_ends_with(std::string str, std::string flag);
extern bool srs_string_ends_with(std::string str, std::string flag0, std::string flag1);
extern bool srs_string_ends_with(std::string str, std::string flag0, std::string flag1, std::string flag2);
extern bool srs_string_ends_with(std::string str, std::string flag0, std::string flag1, std::string flag2, std::string flag3);
// whether string starts with
extern bool srs_string_starts_with(std::string str, std::string flag);
extern bool srs_string_starts_with(std::string str, std::string flag0, std::string flag1);
extern bool srs_string_starts_with(std::string str, std::string flag0, std::string flag1, std::string flag2);
extern bool srs_string_starts_with(std::string str, std::string flag0, std::string flag1, std::string flag2, std::string flag3);
// whether string contains with
extern bool srs_string_contains(std::string str, std::string flag);
extern bool srs_string_contains(std::string str, std::string flag0, std::string flag1);
extern bool srs_string_contains(std::string str, std::string flag0, std::string flag1, std::string flag2);
// find the min match in str for flags.
extern std::string srs_string_min_match(std::string str, std::vector<std::string> flags);
// split the string by flag to array.
extern std::vector<std::string> srs_string_split(std::string str, std::string flag);
extern std::vector<std::string> srs_string_split(std::string str, std::vector<std::string> flags);

/**
 * compare the memory in bytes.
 * @return true if completely equal; otherwise, false.
 */
extern bool srs_bytes_equals(void* pa, void* pb, int size);

// create dir recursively
extern int srs_create_dir_recursively(std::string dir);

// whether path exists.
extern bool srs_path_exists(std::string path);
// get the dirname of path, for instance, dirname("/live/livestream")="/live"
extern std::string srs_path_dirname(std::string path);
// get the basename of path, for instance, basename("/live/livestream")="livestream"
extern std::string srs_path_basename(std::string path);
// get the filename of path, for instance, filename("livestream.flv")="livestream"
extern std::string srs_path_filename(std::string path);
// get the file extension of path, for instance, filext("live.flv")=".flv"
extern std::string srs_path_filext(std::string path);
extern std::string srs_dns_resolve(std::string host);
#endif
