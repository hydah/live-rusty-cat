
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
    ERROR = 0,
    WARNING,
    SUM,
    INFO,
    DEBUG,
};
extern int LOG_LEVEL;

void log(int loglevel, const char *format, ...);
void init_loglevel(int loglevel);
