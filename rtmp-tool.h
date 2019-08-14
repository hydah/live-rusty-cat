#include <sys/types.h>
// typedef unsigned long long u_int64_t;
// typedef u_int64_t uint64_t;
// typedef long long int64_t;
typedef unsigned int u_int32_t;
typedef u_int32_t uint32_t;
// typedef int int32_t;
// typedef unsigned char u_int8_t;
// typedef u_int8_t uint8_t;
// typedef char int8_t;
// typedef unsigned short u_int16_t;
// typedef u_int16_t uint16_t;
// typedef short int16_t;
// typedef int64_t ssize_t;

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

#define SrsVideoAvcFrameTraitNALU 1
#define SrsVideoAvcFrameTypeKeyFrame 1
#define SrsVideoAvcFrameTypeInterFrame 2

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
#define ERROR_SUCCESS  0

