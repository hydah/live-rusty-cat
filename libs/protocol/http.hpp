#ifndef HTTP_HPP
#define HTTP_HPP

#include <map>
#include <vector>
#include <sys/uio.h>
#include "rtmp_tool.h"
#include "basic/file.hpp"
#include "http_parser.h"
#include "defines.hpp"


///////////////////////////////////////////////////////////
// HTTP consts values
///////////////////////////////////////////////////////////
// the default http port.
#define SRS_CONSTS_HTTP_DEFAULT_PORT 80
// linux path seprator
#define SRS_CONSTS_HTTP_PATH_SEP '/'
// query string seprator
#define SRS_CONSTS_HTTP_QUERY_SEP '?'

// the default recv timeout.
#define SRS_HTTP_RECV_TMMS (60 * 1000)
#define SRS_DEFAULT_HTTP_PORT 80

// 6.1.1 Status Code and Reason Phrase
#define SRS_CONSTS_HTTP_Continue                       100
#define SRS_CONSTS_HTTP_SwitchingProtocols             101
#define SRS_CONSTS_HTTP_OK                             200
#define SRS_CONSTS_HTTP_Created                        201
#define SRS_CONSTS_HTTP_Accepted                       202
#define SRS_CONSTS_HTTP_NonAuthoritativeInformation    203
#define SRS_CONSTS_HTTP_NoContent                      204
#define SRS_CONSTS_HTTP_ResetContent                   205
#define SRS_CONSTS_HTTP_PartialContent                 206
#define SRS_CONSTS_HTTP_MultipleChoices                300
#define SRS_CONSTS_HTTP_MovedPermanently               301
#define SRS_CONSTS_HTTP_Found                          302
#define SRS_CONSTS_HTTP_SeeOther                       303
#define SRS_CONSTS_HTTP_NotModified                    304
#define SRS_CONSTS_HTTP_UseProxy                       305
#define SRS_CONSTS_HTTP_TemporaryRedirect              307
#define SRS_CONSTS_HTTP_BadRequest                     400
#define SRS_CONSTS_HTTP_Unauthorized                   401
#define SRS_CONSTS_HTTP_PaymentRequired                402
#define SRS_CONSTS_HTTP_Forbidden                      403
#define SRS_CONSTS_HTTP_NotFound                       404
#define SRS_CONSTS_HTTP_MethodNotAllowed               405
#define SRS_CONSTS_HTTP_NotAcceptable                  406
#define SRS_CONSTS_HTTP_ProxyAuthenticationRequired    407
#define SRS_CONSTS_HTTP_RequestTimeout                 408
#define SRS_CONSTS_HTTP_Conflict                       409
#define SRS_CONSTS_HTTP_Gone                           410
#define SRS_CONSTS_HTTP_LengthRequired                 411
#define SRS_CONSTS_HTTP_PreconditionFailed             412
#define SRS_CONSTS_HTTP_RequestEntityTooLarge          413
#define SRS_CONSTS_HTTP_RequestURITooLarge             414
#define SRS_CONSTS_HTTP_UnsupportedMediaType           415
#define SRS_CONSTS_HTTP_RequestedRangeNotSatisfiable   416
#define SRS_CONSTS_HTTP_ExpectationFailed              417
#define SRS_CONSTS_HTTP_InternalServerError            500
#define SRS_CONSTS_HTTP_NotImplemented                 501
#define SRS_CONSTS_HTTP_BadGateway                     502
#define SRS_CONSTS_HTTP_ServiceUnavailable             503
#define SRS_CONSTS_HTTP_GatewayTimeout                 504
#define SRS_CONSTS_HTTP_HTTPVersionNotSupported        505

#define SRS_CONSTS_HTTP_Continue_str                           "Continue"
#define SRS_CONSTS_HTTP_SwitchingProtocols_str                 "Switching Protocols"
#define SRS_CONSTS_HTTP_OK_str                                 "OK"
#define SRS_CONSTS_HTTP_Created_str                            "Created"
#define SRS_CONSTS_HTTP_Accepted_str                           "Accepted"
#define SRS_CONSTS_HTTP_NonAuthoritativeInformation_str        "Non Authoritative Information"
#define SRS_CONSTS_HTTP_NoContent_str                          "No Content"
#define SRS_CONSTS_HTTP_ResetContent_str                       "Reset Content"
#define SRS_CONSTS_HTTP_PartialContent_str                     "Partial Content"
#define SRS_CONSTS_HTTP_MultipleChoices_str                    "Multiple Choices"
#define SRS_CONSTS_HTTP_MovedPermanently_str                   "Moved Permanently"
#define SRS_CONSTS_HTTP_Found_str                              "Found"
#define SRS_CONSTS_HTTP_SeeOther_str                           "See Other"
#define SRS_CONSTS_HTTP_NotModified_str                        "Not Modified"
#define SRS_CONSTS_HTTP_UseProxy_str                           "Use Proxy"
#define SRS_CONSTS_HTTP_TemporaryRedirect_str                  "Temporary Redirect"
#define SRS_CONSTS_HTTP_BadRequest_str                         "Bad Request"
#define SRS_CONSTS_HTTP_Unauthorized_str                       "Unauthorized"
#define SRS_CONSTS_HTTP_PaymentRequired_str                    "Payment Required"
#define SRS_CONSTS_HTTP_Forbidden_str                          "Forbidden"
#define SRS_CONSTS_HTTP_NotFound_str                           "Not Found"
#define SRS_CONSTS_HTTP_MethodNotAllowed_str                   "Method Not Allowed"
#define SRS_CONSTS_HTTP_NotAcceptable_str                      "Not Acceptable"
#define SRS_CONSTS_HTTP_ProxyAuthenticationRequired_str        "Proxy Authentication Required"
#define SRS_CONSTS_HTTP_RequestTimeout_str                     "Request Timeout"
#define SRS_CONSTS_HTTP_Conflict_str                           "Conflict"
#define SRS_CONSTS_HTTP_Gone_str                               "Gone"
#define SRS_CONSTS_HTTP_LengthRequired_str                     "Length Required"
#define SRS_CONSTS_HTTP_PreconditionFailed_str                 "Precondition Failed"
#define SRS_CONSTS_HTTP_RequestEntityTooLarge_str              "Request Entity Too Large"
#define SRS_CONSTS_HTTP_RequestURITooLarge_str                 "Request URI Too Large"
#define SRS_CONSTS_HTTP_UnsupportedMediaType_str               "Unsupported Media Type"
#define SRS_CONSTS_HTTP_RequestedRangeNotSatisfiable_str       "Requested Range Not Satisfiable"
#define SRS_CONSTS_HTTP_ExpectationFailed_str                  "Expectation Failed"
#define SRS_CONSTS_HTTP_InternalServerError_str                "Internal Server Error"
#define SRS_CONSTS_HTTP_NotImplemented_str                     "Not Implemented"
#define SRS_CONSTS_HTTP_BadGateway_str                         "Bad Gateway"
#define SRS_CONSTS_HTTP_ServiceUnavailable_str                 "Service Unavailable"
#define SRS_CONSTS_HTTP_GatewayTimeout_str                     "Gateway Timeout"
#define SRS_CONSTS_HTTP_HTTPVersionNotSupported_str            "HTTP Version Not Supported"


// http specification
// CR             = <US-ASCII CR, carriage return (13)>
#define SRS_HTTP_CR SRS_CONSTS_CR // 0x0D
// LF             = <US-ASCII LF, linefeed (10)>
#define SRS_HTTP_LF SRS_CONSTS_LF // 0x0A
// SP             = <US-ASCII SP, space (32)>
#define SRS_HTTP_SP ' ' // 0x20
// HT             = <US-ASCII HT, horizontal-tab (9)>
#define SRS_HTTP_HT '\x09' // 0x09

// HTTP/1.1 defines the sequence CR LF as the end-of-line marker for all
// protocol elements except the entity-body (see appendix 19.3 for
// tolerant applications).
#define SRS_HTTP_CRLF "\r\n" // 0x0D0A
#define SRS_HTTP_CRLFCRLF "\r\n\r\n" // 0x0D0A0D0A

// @see ISrsHttpMessage._http_ts_send_buffer
#define SRS_HTTP_TS_SEND_BUFFER_SIZE 4096

// for ead all of http body, read each time.
#define SRS_HTTP_READ_CACHE_BYTES 4096

// for http parser macros
#define SRS_CONSTS_HTTP_OPTIONS HTTP_OPTIONS
#define SRS_CONSTS_HTTP_GET HTTP_GET
#define SRS_CONSTS_HTTP_POST HTTP_POST
#define SRS_CONSTS_HTTP_PUT HTTP_PUT
#define SRS_CONSTS_HTTP_DELETE HTTP_DELETE



#define SRS_HTTP_CLIENT_TIMEOUT_US (int64_t)(1*1000*1000LL)


// the http chunked header size,
// for writev, there always one chunk to send it.
#define SRS_HTTP_HEADER_CACHE_SIZE 64

#define ERROR_HTTP_OK                               200
#define ERROR_HTTP_NO_CONTENT                       204

#define ERROR_HTTP_REQUEST_ERR                      400
#define ERROR_HTTP_REQUEST_NOT_AUTHORIZED           401
#define ERROR_HTTP_REQUEST_FORBIDDEN                403
#define ERROR_HTTP_REQUEST_NOT_FOUND                404
#define ERROR_HTTP_REQUEST_NOT_ACCEPT               406
#define ERROR_HTTP_REQUEST_TIME_EXPIRED             408
#define ERROR_HTTP_REQUEST_NOT_SATISFIED_CONDITION  412
#define ERROR_HTTP_SERVICE_INNER_ERROR              900
#define ERROR_HTTP_SERVICE_NOT_AVAILABLE            903


// state of message
enum SrsHttpParseState {
    SrsHttpParseStateInit = 0,
    SrsHttpParseStateStart,
    SrsHttpParseStateHeaderComplete,
    SrsHttpParseStateMessageComplete
};

class ISrsHttpResponseWriter;
class ISrsHttpResponseReader;
class ISrsHttpHandler;
class ISrsHttpMatchHijacker;
class ISrsHttpServeMux;
class ISrsHttpMessage;

class ISrsProtocolReaderWriter;
class SrsHttpResponseReader;
class SimpleSocketStream;
class SrsRequest;
class SrsFastBuffer;
// A Header represents the key-value pairs in an HTTP header.
class SrsHttpHeader
{
private:
    std::map<std::string, std::string> headers;
public:
    SrsHttpHeader();
    virtual ~SrsHttpHeader();
public:
    // Add adds the key, value pair to the header.
    // It appends to any existing values associated with key.
    virtual void set(std::string key, std::string value);
    // Get gets the first value associated with the given key.
    // If there are no values associated with the key, Get returns "".
    // To access multiple values of a key, access the map directly
    // with CanonicalHeaderKey.
    virtual std::string get(std::string key);
public:
    /**
     * get the content length. -1 if not set.
     */
    virtual int64_t content_length();
    /**
     * set the content length by header "Content-Length"
     */
    virtual void set_content_length(int64_t size);
public:
    /**
     * get the content type. empty string if not set.
     */
    virtual std::string content_type();
    /**
     * set the content type by header "Content-Type"
     */
    virtual void set_content_type(std::string ct);
public:
    /**
     * write all headers to string stream.
     */
    virtual void write(std::stringstream& ss);
};

// the mux entry for server mux.
// the matcher info, for example, the pattern and handler.
class SrsHttpMuxEntry
{
public:
    bool explicit_match;
    ISrsHttpHandler* handler;
    std::string pattern;
    bool enabled;
public:
    SrsHttpMuxEntry();
    virtual ~SrsHttpMuxEntry();
};

/*****************************************************************/
/******************Http Interface*********************************/
/*****************************************************************/

// A ResponseWriter interface is used by an HTTP handler to
// construct an HTTP response.
// Usage 1, response with specified length content:
//      ISrsHttpResponseWriter* w; // create or get response.
//      std::string msg = "Hello, HTTP!";
//      w->header()->set_content_type("text/plain; charset=utf-8");
//      w->header()->set_content_length(msg.length());
//      w->write_header(SRS_CONSTS_HTTP_OK);
//      w->write((char*)msg.data(), (int)msg.length());
//      w->final_request(); // optional flush.
// Usage 2, response with HTTP code only, zero content length.
//      ISrsHttpResponseWriter* w; // create or get response.
//      w->header()->set_content_length(0);
//      w->write_header(SRS_CONSTS_HTTP_OK);
//      w->final_request();
// Usage 3, response in chunked encoding.
//      ISrsHttpResponseWriter* w; // create or get response.
//      std::string msg = "Hello, HTTP!";
//      w->header()->set_content_type("application/octet-stream");
//      w->write_header(SRS_CONSTS_HTTP_OK);
//      w->write((char*)msg.data(), (int)msg.length());
//      w->write((char*)msg.data(), (int)msg.length());
//      w->write((char*)msg.data(), (int)msg.length());
//      w->write((char*)msg.data(), (int)msg.length());
//      w->final_request(); // required to end the chunked and flush.
class ISrsHttpResponseWriter
{
public:
    ISrsHttpResponseWriter();
    virtual ~ISrsHttpResponseWriter();
public:
    // when chunked mode,
    // final the request to complete the chunked encoding.
    // for no-chunked mode,
    // final to send request, for example, content-length is 0.
    virtual int final_request() = 0;

    // Header returns the header map that will be sent by WriteHeader.
    // Changing the header after a call to WriteHeader (or Write) has
    // no effect.
    virtual SrsHttpHeader* header() = 0;

    // Write writes the data to the connection as part of an HTTP reply.
    // If WriteHeader has not yet been called, Write calls WriteHeader(http.StatusOK)
    // before writing the data.  If the Header does not contain a
    // Content-Type line, Write adds a Content-Type set to the result of passing
    // the initial 512 bytes of written data to DetectContentType.
    // @param data, the data to send. NULL to flush header only.
    virtual int write(char* data, int size) = 0;
    /**
     * for the HTTP FLV, to writev to improve performance.
     * @see https://github.com/ossrs/srs/issues/405
     */
    virtual int writev(const iovec* iov, int iovcnt, ssize_t* pnwrite) = 0;

    // WriteHeader sends an HTTP response header with status code.
    // If WriteHeader is not called explicitly, the first call to Write
    // will trigger an implicit WriteHeader(http.StatusOK).
    // Thus explicit calls to WriteHeader are mainly used to
    // send error codes.
    // @remark, user must set header then write or write_header.
    virtual void write_header(int code) = 0;
};

/**
 * the reader interface for http response.
 */
class ISrsHttpResponseReader
{
public:
    ISrsHttpResponseReader();
    virtual ~ISrsHttpResponseReader();
public:
    /**
     * whether response read EOF.
     */
    virtual bool eof() = 0;
    /**
     * read from the response body.
     * @param data, the buffer to read data buffer to.
     * @param nb_data, the max size of data buffer.
     * @param nb_read, the actual read size of bytes. NULL to ignore.
     * @remark when eof(), return error.
     * @remark for some server, the content-length not specified and not chunked,
     *      which is actually the infinite chunked encoding, which after http header
     *      is http response data, it's ok for browser. that is,
     *      when user call this read, please ensure there is data to read(by content-length
     *      or by chunked), because the sdk never know whether there is no data or
     *      infinite chunked.
     */
    virtual int read(char* data, int nb_data, int* nb_read) = 0;
};

// Objects implementing the Handler interface can be
// registered to serve a particular path or subtree
// in the HTTP server.
//
// ServeHTTP should write reply headers and data to the ResponseWriter
// and then return.  Returning signals that the request is finished
// and that the HTTP server can move on to the next request on
// the connection.
class ISrsHttpHandler
{
public:
    SrsHttpMuxEntry* entry;
public:
    ISrsHttpHandler();
    virtual ~ISrsHttpHandler();
public:
    virtual bool is_not_found();
    virtual int serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r) = 0;
};

/**
 * the hijacker for http pattern match.
 */
class ISrsHttpMatchHijacker
{
public:
    ISrsHttpMatchHijacker();
    virtual ~ISrsHttpMatchHijacker();
public:
    /**
     * when match the request failed, no handler to process request.
     * @param request the http request message to match the handler.
     * @param ph the already matched handler, hijack can rewrite it.
     */
    virtual int hijack(ISrsHttpMessage* request, ISrsHttpHandler** ph) = 0;
};

/**
 * the server mux, all http server should implements it.
 */
class ISrsHttpServeMux
{
public:
    ISrsHttpServeMux();
    virtual ~ISrsHttpServeMux();
public:
    virtual int serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r) = 0;
};

// A Request represents an HTTP request received by a server
// or to be sent by a client.
//
// The field semantics differ slightly between client and server
// usage. In addition to the notes on the fields below, see the
// documentation for Request.Write and RoundTripper.
//
// There are some modes to determine the length of body:
//      1. content-length and chunked.
//      2. user confirmed infinite chunked.
//      3. no body or user not confirmed infinite chunked.
// For example:
//      ISrsHttpMessage* r = ...;
//      while (!r->eof()) r->read(); // read in mode 1 or 3.
// For some server, we can confirm the body is infinite chunked:
//      ISrsHttpMessage* r = ...;
//      r->enter_infinite_chunked();
//      while (!r->eof()) r->read(); // read in mode 2
// @rmark for mode 2, the infinite chunked, all left data is body.
class ISrsHttpMessage
{
private:
    /**
     * use a buffer to read and send ts file.
     */
    // TODO: FIXME: remove it.
    char* _http_ts_send_buffer;
public:
    ISrsHttpMessage();
    virtual ~ISrsHttpMessage();
public:
    /**
     * the http request level cache.
     */
    virtual char* http_ts_send_buffer();
public:
    virtual uint8_t method() = 0;
    virtual uint16_t status_code() = 0;
    /**
     * method helpers.
     */
    virtual std::string method_str() = 0;
    virtual bool is_http_get() = 0;
    virtual bool is_http_put() = 0;
    virtual bool is_http_post() = 0;
    virtual bool is_http_delete() = 0;
    virtual bool is_http_options() = 0;
public:
    /**
     * whether should keep the connection alive.
     */
    virtual bool is_keep_alive() = 0;
    /**
     * the uri contains the host and path.
     */
    virtual std::string uri() = 0;
    /**
     * the url maybe the path.
     */
    virtual std::string url() = 0;
    virtual std::string host() = 0;
    virtual std::string path() = 0;
    virtual std::string query() = 0;
    virtual std::string ext() = 0;
    /**
     * get the RESTful id,
     * for example, pattern is /api/v1/streams, path is /api/v1/streams/100,
     * then the rest id is 100.
     * @param pattern the handler pattern which will serve the request.
     * @return the REST id; -1 if not matched.
     */
    virtual int parse_rest_id(std::string pattern) = 0;
public:
    /**
     * the left all data is chunked body, the infinite chunked mode,
     * which is chunked encoding without chunked header.
     * @remark error when message is in chunked or content-length specified.
     */
    virtual int enter_infinite_chunked() = 0;
    /**
     * read body to string.
     * @remark for small http body.
     */
    virtual int body_read_all(std::string& body) = 0;
    /**
     * get the body reader, to read one by one.
     * @remark when body is very large, or chunked, use this.
     */
    virtual ISrsHttpResponseReader* body_reader() = 0;
    /**
     * the content length, -1 for chunked or not set.
     */
    virtual int64_t content_length() = 0;
public:
    /**
     * get the param in query string,
     * for instance, query is "start=100&end=200",
     * then query_get("start") is "100", and query_get("end") is "200"
     */
    virtual std::string query_get(std::string key) = 0;
    /**
     * get the headers.
     */
    virtual int request_header_count() = 0;
    virtual std::string request_header_key_at(int index) = 0;
    virtual std::string request_header_value_at(int index) = 0;
public:
    /**
     * whether the current request is JSONP,
     * which has a "callback=xxx" in QueryString.
     */
    virtual bool is_jsonp() = 0;
};

/***************************************************************************/
/**************************** End of Http Interface ************************/
/***************************************************************************/

// Redirect to a fixed URL
class SrsHttpRedirectHandler : public ISrsHttpHandler
{
private:
    std::string url;
    int code;
public:
    SrsHttpRedirectHandler(std::string u, int c);
    virtual ~SrsHttpRedirectHandler();
public:
    virtual int serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r);
};

// NotFound replies to the request with an HTTP 404 not found error.
class SrsHttpNotFoundHandler : public ISrsHttpHandler
{
public:
    SrsHttpNotFoundHandler();
    virtual ~SrsHttpNotFoundHandler();
public:
    virtual bool is_not_found();
    virtual int serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r);
};

// FileServer returns a handler that serves HTTP requests
// with the contents of the file system rooted at root.
//
// To use the operating system's file system implementation,
// use http.Dir:
//
//     http.Handle("/", SrsHttpFileServer("/tmp"))
//     http.Handle("/", SrsHttpFileServer("static-dir"))
class SrsHttpFileServer : public ISrsHttpHandler
{
protected:
    std::string dir;
public:
    SrsHttpFileServer(std::string root_dir);
    virtual ~SrsHttpFileServer();
public:
    virtual int serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r);
private:
    /**
     * serve the file by specified path
     */
    virtual int serve_file(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, std::string fullpath);
    virtual int serve_flv_file(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, std::string fullpath);
    virtual int serve_mp4_file(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, std::string fullpath);
protected:
    /**
     * when access flv file with x.flv?start=xxx
     */
    virtual int serve_flv_stream(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, std::string fullpath, int offset);
    /**
     * when access mp4 file with x.mp4?range=start-end
     * @param start the start offset in bytes.
     * @param end the end offset in bytes. -1 to end of file.
     * @remark response data in [start, end].
     */
    virtual int serve_mp4_stream(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, std::string fullpath, int start, int end);
protected:
    /**
     * copy the fs to response writer in size bytes.
     */
    virtual int copy(ISrsHttpResponseWriter* w, SrsFileReader* fs, ISrsHttpMessage* r, int size);
};



// ServeMux is an HTTP request multiplexer.
// It matches the URL of each incoming request against a list of registered
// patterns and calls the handler for the pattern that
// most closely matches the URL.
//
// Patterns name fixed, rooted paths, like "/favicon.ico",
// or rooted subtrees, like "/images/" (note the trailing slash).
// Longer patterns take precedence over shorter ones, so that
// if there are handlers registered for both "/images/"
// and "/images/thumbnails/", the latter handler will be
// called for paths beginning "/images/thumbnails/" and the
// former will receive requests for any other paths in the
// "/images/" subtree.
//
// Note that since a pattern ending in a slash names a rooted subtree,
// the pattern "/" matches all paths not matched by other registered
// patterns, not just the URL with Path == "/".
//
// Patterns may optionally begin with a host name, restricting matches to
// URLs on that host only.  Host-specific patterns take precedence over
// general patterns, so that a handler might register for the two patterns
// "/codesearch" and "codesearch.google.com/" without also taking over
// requests for "http://www.google.com/".
//
// ServeMux also takes care of sanitizing the URL request path,
// redirecting any request containing . or .. elements to an
// equivalent .- and ..-free URL.
class SrsHttpServeMux : public ISrsHttpServeMux
{
private:
    // the pattern handler, to handle the http request.
    std::map<std::string, SrsHttpMuxEntry*> entries;
    // the vhost handler.
    // when find the handler to process the request,
    // append the matched vhost when pattern not starts with /,
    // for example, for pattern /live/livestream.flv of vhost ossrs.net,
    // the path will rewrite to ossrs.net/live/livestream.flv
    std::map<std::string, ISrsHttpHandler*> vhosts;
    // all hijackers for http match.
    // for example, the hstrs(http stream trigger rtmp source)
    // can hijack and install handler when request incoming and no handler.
    std::vector<ISrsHttpMatchHijacker*> hijackers;
public:
    SrsHttpServeMux();
    virtual ~SrsHttpServeMux();
public:
    /**
     * initialize the http serve mux.
     */
    virtual int initialize();
    /**
     * hijack the http match.
     */
    virtual void hijack(ISrsHttpMatchHijacker* h);
    virtual void unhijack(ISrsHttpMatchHijacker* h);
public:
    // Handle registers the handler for the given pattern.
    // If a handler already exists for pattern, Handle panics.
    virtual int handle(std::string pattern, ISrsHttpHandler* handler);
    // whether the http muxer can serve the specified message,
    // if not, user can try next muxer.
    virtual bool can_serve(ISrsHttpMessage* r);
// interface ISrsHttpServeMux
public:
    virtual int serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r);
private:
    virtual int find_handler(ISrsHttpMessage* r, ISrsHttpHandler** ph);
    virtual int match(ISrsHttpMessage* r, ISrsHttpHandler** ph);
    virtual bool path_match(std::string pattern, std::string path);
};

/**
 * The filter http mux, directly serve the http CORS requests,
 * while proxy to the worker mux for services.
 */
class SrsHttpCorsMux : public ISrsHttpServeMux
{
private:
    bool required;
    bool enabled;
    ISrsHttpServeMux* next;
public:
    SrsHttpCorsMux();
    virtual ~SrsHttpCorsMux();
public:
    virtual int initialize(ISrsHttpServeMux* worker, bool cros_enabled);
// interface ISrsHttpServeMux
public:
    virtual int serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r);
};

// for http header.
typedef std::pair<std::string, std::string> SrsHttpHeaderField;


/**
 * used to resolve the http uri.
 */
class SrsHttpUri
{
private:
    std::string url;
    std::string schema;
    std::string host;
    int port;
    std::string path;
    std::string raw_path;
    std::string query;
public:
    SrsHttpUri();
    virtual ~SrsHttpUri();
public:
    /**
     * initialize the http uri.
     */
    virtual int initialize(std::string _url);
public:
    virtual const char* get_url();
    virtual const char* get_schema();
    virtual const char* get_host();
    virtual int get_port();
    virtual const char* get_path();
    virtual const char* get_query();
    virtual const char* get_raw_path();
private:
    /**
     * get the parsed url field.
     * @return return empty string if not set.
     */
    virtual std::string get_uri_field(std::string uri, http_parser_url* hp_u, http_parser_url_fields field);
};

typedef std::pair<std::string, std::string> SrsHttpHeaderField;

// A Request represents an HTTP request received by a server
// or to be sent by a client.
//
// The field semantics differ slightly between client and server
// usage. In addition to the notes on the fields below, see the
// documentation for Request.Write and RoundTripper.
class SrsHttpMessage : public ISrsHttpMessage
{
private:
    /**
     * parsed url.
     */
    std::string _url;
    /**
     * the extension of file, for example, .flv
     */
    std::string _ext;
    /**
     * parsed http header.
     */
    http_parser _header;
    /**
     * body object, reader object.
     * @remark, user can get body in string by get_body().
     */
    SrsHttpResponseReader* _body;
    /**
     * whether the body is chunked.
     */
    bool chunked;
    /**
     * whether the body is infinite chunked.
     */
     bool infinite_chunked;
    /**
     * whether the request indicates should keep alive
     * for the http connection.
     */
    bool keep_alive;
    /**
     * uri parser
     */
    SrsHttpUri* _uri;
    /**
     * use a buffer to read and send ts file.
     */
    // TODO: FIXME: remove it.
    char* _http_ts_send_buffer;
    // http headers
    std::vector<SrsHttpHeaderField> _headers;
    // the query map
    std::map<std::string, std::string> _query;
    // the transport connection, can be NULL.
    //SrsConnection* conn;
    // whether request is jsonp.
    bool jsonp;
    // the method in QueryString will override the HTTP method.
    std::string jsonp_method;
public:
    SrsHttpMessage(SimpleSocketStream* io);
    virtual ~SrsHttpMessage();
public:
    /**
     * set the original messages, then update the message.
     */
    virtual int update(std::string url, http_parser* header,
        SrsFastBuffer* body, std::vector<SrsHttpHeaderField>& headers
    );
    virtual SrsHttpResponseReader* get_http_response_reader();
public:
    //virtual SrsConnection* connection();
public:
    virtual u_int8_t method();
    virtual u_int16_t status_code();
    /**
     * method helpers.
     */
    virtual std::string method_str();
    virtual bool is_http_get();
    virtual bool is_http_put();
    virtual bool is_http_post();
    virtual bool is_http_delete();
    virtual bool is_http_options();
    /**
     * whether body is chunked encoding, for reader only.
     */
    virtual bool is_chunked();
    /**
     * whether body is infinite chunked encoding.
     * @remark set by enter_infinite_chunked.
     */
     virtual bool is_infinite_chunked();
    /**
     * whether should keep the connection alive.
     */
    virtual bool is_keep_alive();
    /**
     * the uri contains the host and path.
     */
    virtual std::string uri();
    /**
     * the url maybe the path.
     */
    virtual std::string url();
    virtual std::string host();
    virtual std::string path();
    virtual std::string query();
    virtual std::string ext();
    /**
     * get the RESTful matched id.
     */
    virtual int parse_rest_id(std::string pattern);
public:
    virtual int enter_infinite_chunked();
public:
    /**
     * read body to string.
     * @remark for small http body.
     */
    virtual int body_read_all(std::string& body);
    /**
     * get the body reader, to read one by one.
     * @remark when body is very large, or chunked, use this.
     */
    virtual ISrsHttpResponseReader* body_reader();
    /**
     * the content length, -1 for chunked or not set.
     */
    virtual int64_t content_length();
    /**
     * get the param in query string,
     * for instance, query is "start=100&end=200",
     * then query_get("start") is "100", and query_get("end") is "200"
     */
    virtual std::string query_get(std::string key);
    /**
     * get the headers.
     */
    virtual int request_header_count();
    virtual std::string request_header_key_at(int index);
    virtual std::string request_header_value_at(int index);
    virtual std::string get_request_header(std::string name);
public:
    /**
     * convert the http message to a request.
     * @remark user must free the return request.
     */
    virtual SrsRequest* to_request(std::string vhost);
public:
    virtual bool is_jsonp();
};

/**
 * response writer use st socket
 */
class SrsHttpResponseWriter : public ISrsHttpResponseWriter
{
private:
    SimpleSocketStream* skt;
    SrsHttpHeader* hdr;
private:
    char header_cache[SRS_HTTP_HEADER_CACHE_SIZE];
    iovec* iovss_cache;
    int nb_iovss_cache;
private:
    // reply header has been (logically) written
    bool header_wrote;
    // status code passed to WriteHeader
    int status;
private:
    // explicitly-declared Content-Length; or -1
    int64_t content_length;
    // number of bytes written in body
    int64_t written;
private:
    // wroteHeader tells whether the header's been written to "the
    // wire" (or rather: w.conn.buf). this is unlike
    // (*response).wroteHeader, which tells only whether it was
    // logically written.
    bool header_sent;
public:
    SrsHttpResponseWriter(SimpleSocketStream* io);
    virtual ~SrsHttpResponseWriter();
public:
    virtual int final_request();
    virtual SrsHttpHeader* header();
    virtual int write(char* data, int size);
    virtual int writev(iovec* iov, int iovcnt, ssize_t* pnwrite);
    virtual void write_header(int code);
    virtual int send_header(char* data, int size);
    virtual SimpleSocketStream*  srs_st_socket();
};

#ifdef SRS_PERF_MERGED_READ
/**
* to improve read performance, merge some packets then read,
* when it on and read small bytes, we sleep to wait more data.,
* that is, we merge some data to read together.
* @see https://github.com/ossrs/srs/issues/241
*/
class IMergeReadHandler
{
public:
    IMergeReadHandler();
    virtual ~IMergeReadHandler();
public:
    /**
    * when read from channel, notice the merge handler to sleep for
    * some small bytes.
    * @remark, it only for server-side, client srs-librtmp just ignore.
    */
    virtual void on_read(ssize_t nread) = 0;
};
#endif


/**
* the buffer provices bytes cache for protocol. generally,
* protocol recv data from socket, put into buffer, decode to RTMP message.
* Usage:
*       ISrsBufferReader* r = ......;
*       SrsFastBuffer* fb = ......;
*       fb->grow(r, 1024);
*       char* header = fb->read_slice(100);
*       char* payload = fb->read_payload(924);
*/
// TODO: FIXME: add utest for it.
class SrsFastBuffer
{
private:
#ifdef SRS_PERF_MERGED_READ
    // the merged handler
    bool merged_read;
    IMergeReadHandler* _handler;
#endif
    // the user-space buffer to fill by reader,
    // which use fast index and reset when chunk body read ok.
    // @see https://github.com/ossrs/srs/issues/248
    // ptr to the current read position.
    char* p;
    // ptr to the content end.
    char* end;
    // ptr to the buffer.
    //      buffer <= p <= end <= buffer+nb_buffer
    char* buffer;
    // the size of buffer.
    int nb_buffer;
public:
    SrsFastBuffer();
    virtual ~SrsFastBuffer();
public:
    /**
    * get the size of current bytes in buffer.
    */
    virtual int size();
    /**
    * get the current bytes in buffer.
    * @remark user should use read_slice() if possible,
    *       the bytes() is used to test bytes, for example, to detect the bytes schema.
    */
    virtual char* bytes();
    /**
    * create buffer with specifeid size.
    * @param buffer the size of buffer. ignore when smaller than SRS_MAX_SOCKET_BUFFER.
    * @remark when MR(SRS_PERF_MERGED_READ) disabled, always set to 8K.
    * @remark when buffer changed, the previous ptr maybe invalid.
    * @see https://github.com/ossrs/srs/issues/241
    */
    virtual void set_buffer(int buffer_size);
public:
    /**
    * read 1byte from buffer, move to next bytes.
    * @remark assert buffer already grow(1).
    */
    virtual char read_1byte();
    /**
    * read a slice in size bytes, move to next bytes.
    * user can use this char* ptr directly, and should never free it.
    * @remark user can use the returned ptr util grow(size),
    *       for the ptr returned maybe invalid after grow(x).
    */
    virtual char* read_slice(int size);
    /**
    * skip some bytes in buffer.
    * @param size the bytes to skip. positive to next; negative to previous.
    * @remark assert buffer already grow(size).
    * @remark always use read_slice to consume bytes, which will reset for EOF.
    *       while skip never consume bytes.
    */
    virtual void skip(int size);
    virtual int update(char* data, int required_size);
public:
    /**
    * grow buffer to the required size, loop to read from skt to fill.
    * @param reader, read more bytes from reader to fill the buffer to required size.
    * @param required_size, loop to fill to ensure buffer size to required.
    * @return an int error code, error if required_size negative.
    * @remark, we actually maybe read more than required_size, maybe 4k for example.
    */
    //virtual int grow(ISrsBufferReader* reader, int required_size);
    virtual int grow(ISrsHttpResponseReader* reader, int required_size);
    virtual int grow(ISrsProtocolReaderWriter* reader, int required_size);
public:
#ifdef SRS_PERF_MERGED_READ
    /**
    * to improve read performance, merge some packets then read,
    * when it on and read small bytes, we sleep to wait more data.,
    * that is, we merge some data to read together.
    * @param v true to ename merged read.
    * @param handler the handler when merge read is enabled.
    * @see https://github.com/ossrs/srs/issues/241
    * @remark the merged read is optional, ignore if not specifies.
    */
    virtual void set_merge_read(bool v, IMergeReadHandler* handler);
#endif
};


/**
 * response reader use st socket.
 */
class SrsHttpResponseReader : virtual public ISrsHttpResponseReader
{
private:
    SimpleSocketStream* skt;
    SrsHttpMessage* owner;
    SrsFastBuffer* buffer;
    bool is_eof;
    // the left bytes in chunk.
    int nb_left_chunk;
    // the number of bytes of current chunk.
    int nb_chunk;
    // already read total bytes.
    int64_t nb_total_read;
public:
    SrsHttpResponseReader(SrsHttpMessage* msg, SimpleSocketStream* io);
    virtual ~SrsHttpResponseReader();
public:
    /**
     * initialize the response reader with buffer.
     */
    virtual int initialize(SrsFastBuffer* buffer);
    // interface ISrsHttpResponseReader
public:
    virtual bool eof();
    virtual int read(char* data, int nb_data, int* nb_read);
    virtual int read_full(char* data, int nb_data, int* nb_read);
private:
    virtual int read_chunked(char* data, int nb_data, int* nb_read);
    virtual int read_specified(char* data, int nb_data, int* nb_read);
};


/**
 * wrapper for http-parser,
 * provides HTTP message originted service.
 */
class SrsHttpParser
{
private:
    http_parser_settings settings;
    http_parser parser;
    // the global parse buffer.
    SrsFastBuffer* buffer;
private:
    // http parse data, reset before parse message.
    bool expect_field_name;
    std::string field_name;
    std::string field_value;
    SrsHttpParseState state;
    http_parser header;
    std::string url;
    std::vector<SrsHttpHeaderField> headers;
    int header_parsed;
public:
    SrsHttpParser();
    virtual ~SrsHttpParser();
public:
    /**
     * initialize the http parser with specified type,
     * one parser can only parse request or response messages.
     */
    virtual int initialize(enum http_parser_type type);
    /**
     * always parse a http message,
     * that is, the *ppmsg always NOT-NULL when return success.
     * or error and *ppmsg must be NULL.
     * @remark, if success, *ppmsg always NOT-NULL, *ppmsg always is_complete().
     */
    virtual int parse_message(SimpleSocketStream* skt,      ISrsHttpMessage** ppmsg);
private:
    /**
     * parse the HTTP message to member field: msg.
     */
    virtual int parse_message_imp(SimpleSocketStream* skt);
private:
    static int on_message_begin(http_parser* parser);
    static int on_headers_complete(http_parser* parser);
    static int on_message_complete(http_parser* parser);
    static int on_url(http_parser* parser, const char* at, size_t length);
    static int on_header_field(http_parser* parser, const char* at, size_t length);
    static int on_header_value(http_parser* parser, const char* at, size_t length);
    static int on_body(http_parser* parser, const char* at, size_t length);
};

// Error replies to the request with the specified error message and HTTP code.
// The error message should be plain text.
extern int srs_go_http_error(ISrsHttpResponseWriter* w, int code);
extern int srs_go_http_error(ISrsHttpResponseWriter* w, int code, std::string error);

// get the status text of code.
extern std::string srs_generate_http_status_text(int status);

// bodyAllowedForStatus reports whether a given response status code
// permits a body.  See RFC2616, section 4.4.
extern bool srs_go_http_body_allowd(int status);

// DetectContentType implements the algorithm described
// at http://mimesniff.spec.whatwg.org/ to determine the
// Content-Type of the given data.  It considers at most the
// first 512 bytes of data.  DetectContentType always returns
// a valid MIME type: if it cannot determine a more specific one, it
// returns "application/octet-stream".
extern std::string srs_go_http_detect(char* data, int size);

#endif