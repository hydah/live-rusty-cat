#ifndef LOG_HPP
#define LOG_HPP

#include <stdio.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>

//#include <srs_kernel_consts.hpp>

/**
* the log level, for example:
* if specified Debug level, all level messages will be logged.
* if specified Warn level, only Warn/Error/Fatal level messages will be logged.
*/
class SrsLogLevel
{
public:
    // only used for very verbose debug, generally,
    // we compile without this level for high performance.
    static const int Verbose = 0x01;
    static const int Info = 0x02;
    static const int Trace = 0x03;
    static const int Warn = 0x04;
    static const int Error = 0x05;
    // specified the disabled level, no log, for utest.
    static const int Disabled = 0x06;
};

/**
* the log interface provides method to write log.
* but we provides some macro, which enable us to disable the log when compile.
* @see also SmtDebug/SmtTrace/SmtWarn/SmtError which is corresponding to Debug/Trace/Warn/Fatal.
*/
class ISrsLog
{
public:
    ISrsLog();
    virtual ~ISrsLog();
public:
    /**
    * initialize log utilities.
    */
    virtual int initialize();
    /**
     * reopen the log file for log rotate.
     */
    virtual void reopen();
public:
    /**
    * log for verbose, very verbose information.
    */
    virtual void verbose(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for debug, detail information.
    */
    virtual void info(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for trace, important information.
    */
    virtual void trace(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for warn, warn is something should take attention, but not a error.
    */
    virtual void warn(const char* tag, int context_id, const char* fmt, ...);
    /**
    * log for error, something error occur, do something about the error, ie. close the connection,
    * but we will donot abort the program.
    */
    virtual void error(const char* tag, int context_id, const char* fmt, ...);
};

/**
 * the context id manager to identify context, for instance, the green-thread.
 * usage:
 *      _srs_context->generate_id(); // when thread start.
 *      _srs_context->get_id(); // get current generated id.
 *      int old_id = _srs_context->set_id(1000); // set context id if need to merge thread context.
 */
// the context for multiple clients.
class ISrsThreadContext
{
public:
    ISrsThreadContext();
    virtual ~ISrsThreadContext();
public:
    /**
     * generate the id for current context.
     */
    virtual int generate_id();
    /**
     * get the generated id of current context.
     */
    virtual int get_id();
    /**
     * set the id of current context.
     * @return the previous id value; 0 if no context.
     */
    virtual int set_id(int v);
};

// @global user must provides a log object
extern ISrsLog* _srs_log;

// @global user must implements the LogContext and define a global instance.
extern ISrsThreadContext* _srs_context;

// donot print method
#if 1
    #define srs_verbose(msg, ...) _srs_log->verbose(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _srs_log->info(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _srs_log->trace(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
    /*
    #define srs_trace(msg, ...) \
        fprintf(stdout, "[T][%d][%s] ", getpid(), srs_human_format_time());\
        fprintf(stdout, msg, ##__VA_ARGS__); fprintf(stdout, "\n")
        */
    #define srs_warn(msg, ...)    _srs_log->warn(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _srs_log->error(NULL, _srs_context->get_id(), msg, ##__VA_ARGS__)
#endif
// use __FUNCTION__ to print c method
#if 0
    #define srs_verbose(msg, ...) _srs_log->verbose(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _srs_log->info(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _srs_log->trace(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _srs_log->warn(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _srs_log->error(__FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
#endif
// use __PRETTY_FUNCTION__ to print c++ class:method
#if 0
    #define srs_verbose(msg, ...) _srs_log->verbose(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_info(msg, ...)    _srs_log->info(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_trace(msg, ...)   _srs_log->trace(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_warn(msg, ...)    _srs_log->warn(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
    #define srs_error(msg, ...)   _srs_log->error(__PRETTY_FUNCTION__, _srs_context->get_id(), msg, ##__VA_ARGS__)
#endif

/**
 * Format current time to human readable string.
 * @return A NULL terminated string.
 */
const char* srs_human_format_time();

// The log function for librtmp.
// User can disable it by define macro SRS_DISABLE_LOG.
// Or user can directly use them, or define the alias by:
//      #define trace(msg, ...) srs_human_trace(msg, ##__VA_ARGS__)
//      #define warn(msg, ...) srs_human_warn(msg, ##__VA_ARGS__)
//      #define error(msg, ...) srs_human_error(msg, ##__VA_ARGS__)
#ifdef SRS_DISABLE_LOG
    #define srs_human_trace(msg, ...) (void)0
    #define srs_human_warn(msg, ...) (void)0
    #define srs_human_error(msg, ...) (void)0
    #define srs_human_verbose(msg, ...) (void)0
    #define srs_human_raw(msg, ...) (void)0
#else
    #include <string.h>
    #include <errno.h>
    #define srs_human_trace(msg, ...) \
        fprintf(stdout, "[T][%d][%s] ", getpid(), srs_human_format_time());\
        fprintf(stdout, msg, ##__VA_ARGS__); fprintf(stdout, "\n")
    #define srs_human_warn(msg, ...) \
        fprintf(stdout, "[W][%d][%s][%d] ", getpid(), srs_human_format_time(), errno); \
        fprintf(stdout, msg, ##__VA_ARGS__); \
        fprintf(stdout, "\n")
    #define srs_human_error(msg, ...) \
        fprintf(stderr, "[E][%d][%s][%d] ", getpid(), srs_human_format_time(), errno);\
        fprintf(stderr, msg, ##__VA_ARGS__); \
        fprintf(stderr, " (%s)\n", strerror(errno))
    #define srs_human_verbose(msg, ...) (void)0
    #define srs_human_raw(msg, ...) printf(msg, ##__VA_ARGS__)
#endif
#endif