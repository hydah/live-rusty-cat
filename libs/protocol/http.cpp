#include <stdlib.h>
#include <sstream>
#include <algorithm>
#include "http_parser.h"
#include "http.hpp"
#include "rtmp.hpp"


using namespace std;

#define SRS_HTTP_DEFAULT_PAGE "index.html"


ISrsHttpResponseWriter::ISrsHttpResponseWriter()
{
}

ISrsHttpResponseWriter::~ISrsHttpResponseWriter()
{
}

ISrsHttpResponseReader::ISrsHttpResponseReader()
{
}

ISrsHttpResponseReader::~ISrsHttpResponseReader()
{
}

ISrsHttpHandler::ISrsHttpHandler()
{
    entry = NULL;
}

ISrsHttpHandler::~ISrsHttpHandler()
{
}

bool ISrsHttpHandler::is_not_found()
{
    return false;
}

ISrsHttpMatchHijacker::ISrsHttpMatchHijacker()
{
}

ISrsHttpMatchHijacker::~ISrsHttpMatchHijacker()
{
}

ISrsHttpServeMux::ISrsHttpServeMux()
{
}

ISrsHttpServeMux::~ISrsHttpServeMux()
{
}


SrsHttpHeader::SrsHttpHeader()
{
}

SrsHttpHeader::~SrsHttpHeader()
{
}


ISrsHttpMessage::ISrsHttpMessage()
{
    _http_ts_send_buffer = new char[SRS_HTTP_TS_SEND_BUFFER_SIZE];
}

ISrsHttpMessage::~ISrsHttpMessage()
{
    srs_freepa(_http_ts_send_buffer);
}

char* ISrsHttpMessage::http_ts_send_buffer()
{
    return _http_ts_send_buffer;
}


void SrsHttpHeader::set(string key, string value)
{
    headers[key] = value;
}

string SrsHttpHeader::get(string key)
{
    std::string v;

    if (headers.find(key) != headers.end()) {
        v = headers[key];
    }

    return v;
}

int64_t SrsHttpHeader::content_length()
{
    std::string cl = get("Content-Length");

    if (cl.empty()) {
        return -1;
    }

    return (int64_t)::atof(cl.c_str());
}

void SrsHttpHeader::set_content_length(int64_t size)
{
    set("Content-Length", srs_int2str(size));
}

string SrsHttpHeader::content_type()
{
    return get("Content-Type");
}

void SrsHttpHeader::set_content_type(string ct)
{
    set("Content-Type", ct);
}

void SrsHttpHeader::write(stringstream& ss)
{
    std::map<std::string, std::string>::iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
        ss << it->first << ": " << it->second << SRS_HTTP_CRLF;
    }
}



SrsHttpRedirectHandler::SrsHttpRedirectHandler(string u, int c)
{
    url = u;
    code = c;
}

SrsHttpRedirectHandler::~SrsHttpRedirectHandler()
{
}

int SrsHttpRedirectHandler::serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r)
{
    int ret = ERROR_SUCCESS;

    string location = url;
    if (!r->query().empty()) {
        location += "?" + r->query();
    }

    string msg = "Redirect to" + location;

    w->header()->set_content_type("text/plain; charset=utf-8");
    w->header()->set_content_length(msg.length());
    w->header()->set("Location", location);
    w->write_header(code);

    w->write((char*)msg.data(), (int)msg.length());
    w->final_request();

    srs_info("redirect to %s.", location.c_str());
    return ret;
}

SrsHttpNotFoundHandler::SrsHttpNotFoundHandler()
{
}

SrsHttpNotFoundHandler::~SrsHttpNotFoundHandler()
{
}

bool SrsHttpNotFoundHandler::is_not_found()
{
    return true;
}

int SrsHttpNotFoundHandler::serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r)
{
    return srs_go_http_error(w, SRS_CONSTS_HTTP_NotFound);
}

SrsHttpFileServer::SrsHttpFileServer(string root_dir)
{
    dir = root_dir;
}

SrsHttpFileServer::~SrsHttpFileServer()
{
}

int SrsHttpFileServer::serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r)
{
    string upath = r->path();

    // add default pages.
    if (srs_string_ends_with(upath, "/")) {
        upath += SRS_HTTP_DEFAULT_PAGE;
    }

    string fullpath = dir + "/";

    // remove the virtual directory.
    srs_assert(entry);
    size_t pos = entry->pattern.find("/");
    if (upath.length() > entry->pattern.length() && pos != string::npos) {
        fullpath += upath.substr(entry->pattern.length() - pos);
    } else {
        fullpath += upath;
    }

    // stat current dir, if exists, return error.
    if (!srs_path_exists(fullpath)) {
        srs_warn("http miss file=%s, pattern=%s, upath=%s",
                 fullpath.c_str(), entry->pattern.c_str(), upath.c_str());
        return SrsHttpNotFoundHandler().serve_http(w, r);
    }
    srs_trace("http match file=%s, pattern=%s, upath=%s",
              fullpath.c_str(), entry->pattern.c_str(), upath.c_str());

    // handle file according to its extension.
    // use vod stream for .flv/.fhv
    if (srs_string_ends_with(fullpath, ".flv") || srs_string_ends_with(fullpath, ".fhv")) {
        return serve_flv_file(w, r, fullpath);
    } else if (srs_string_ends_with(fullpath, ".mp4")) {
        return serve_mp4_file(w, r, fullpath);
    }

    // serve common static file.
    return serve_file(w, r, fullpath);
}

int SrsHttpFileServer::serve_file(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, string fullpath)
{
    int ret = ERROR_SUCCESS;

    // open the target file.
    SrsFileReader fs;

    if ((ret = fs.open(fullpath)) != ERROR_SUCCESS) {
        srs_warn("open file %s failed, ret=%d", fullpath.c_str(), ret);
        return ret;
    }

    int64_t length = fs.filesize();

    // unset the content length to encode in chunked encoding.
    w->header()->set_content_length(length);

    static std::map<std::string, std::string> _mime;
    if (_mime.empty()) {
        _mime[".ts"] = "video/MP2T";
        _mime[".flv"] = "video/x-flv";
        _mime[".m4v"] = "video/x-m4v";
        _mime[".3gpp"] = "video/3gpp";
        _mime[".3gp"] = "video/3gpp";
        _mime[".mp4"] = "video/mp4";
        _mime[".aac"] = "audio/x-aac";
        _mime[".mp3"] = "audio/mpeg";
        _mime[".m4a"] = "audio/x-m4a";
        _mime[".ogg"] = "audio/ogg";
        // @see hls-m3u8-draft-pantos-http-live-streaming-12.pdf, page 5.
        _mime[".m3u8"] = "application/vnd.apple.mpegurl"; // application/x-mpegURL
        _mime[".rss"] = "application/rss+xml";
        _mime[".json"] = "application/json";
        _mime[".swf"] = "application/x-shockwave-flash";
        _mime[".doc"] = "application/msword";
        _mime[".zip"] = "application/zip";
        _mime[".rar"] = "application/x-rar-compressed";
        _mime[".xml"] = "text/xml";
        _mime[".html"] = "text/html";
        _mime[".js"] = "text/javascript";
        _mime[".css"] = "text/css";
        _mime[".ico"] = "image/x-icon";
        _mime[".png"] = "image/png";
        _mime[".jpeg"] = "image/jpeg";
        _mime[".jpg"] = "image/jpeg";
        _mime[".gif"] = "image/gif";
        // For MPEG-DASH.
        //_mime[".mpd"] = "application/dash+xml";
        _mime[".mpd"] = "text/xml";
        _mime[".m4s"] = "video/iso.segment";
        _mime[".mp4v"] = "video/mp4";
    }

    if (true) {
        std::string ext = srs_path_filext(fullpath);

        if (_mime.find(ext) == _mime.end()) {
            w->header()->set_content_type("application/octet-stream");
        } else {
            w->header()->set_content_type(_mime[ext]);
        }
    }

    // write body.
    int64_t left = length;
    if ((ret = copy(w, &fs, r, (int)left)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("read file=%s size=%d failed, ret=%d", fullpath.c_str(), left, ret);
        }
        return ret;
    }

    return w->final_request();
}

int SrsHttpFileServer::serve_flv_file(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, string fullpath)
{
    std::string start = r->query_get("start");
    if (start.empty()) {
        return serve_file(w, r, fullpath);
    }

    int offset = ::atoi(start.c_str());
    if (offset <= 0) {
        return serve_file(w, r, fullpath);
    }

    return serve_flv_stream(w, r, fullpath, offset);
}

int SrsHttpFileServer::serve_mp4_file(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, string fullpath)
{
    // for flash to request mp4 range in query string.
    // for example, http://digitalprimates.net/dash/DashTest.html?url=http://dashdemo.edgesuite.net/digitalprimates/nexus/oops-20120802-manifest.mpd
    std::string range = r->query_get("range");
    // or, use bytes to request range,
    // for example, http://dashas.castlabs.com/demo/try.html
    if (range.empty()) {
        range = r->query_get("bytes");
    }

    // rollback to serve whole file.
    size_t pos = string::npos;
    if (range.empty() || (pos = range.find("-")) == string::npos) {
        return serve_file(w, r, fullpath);
    }

    // parse the start in query string
    int start = 0;
    if (pos > 0) {
        start = ::atoi(range.substr(0, pos).c_str());
    }

    // parse end in query string.
    int end = -1;
    if (pos < range.length() - 1) {
        end = ::atoi(range.substr(pos + 1).c_str());
    }

    // invalid param, serve as whole mp4 file.
    if (start < 0 || (end != -1 && start > end)) {
        return serve_file(w, r, fullpath);
    }

    return serve_mp4_stream(w, r, fullpath, start, end);
}

int SrsHttpFileServer::serve_flv_stream(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, string fullpath, int offset)
{
    return serve_file(w, r, fullpath);
}

int SrsHttpFileServer::serve_mp4_stream(ISrsHttpResponseWriter* w, ISrsHttpMessage* r, string fullpath, int start, int end)
{
    return serve_file(w, r, fullpath);
}

int SrsHttpFileServer::copy(ISrsHttpResponseWriter* w, SrsFileReader* fs, ISrsHttpMessage* r, int size)
{
    int ret = ERROR_SUCCESS;

    int left = size;
    char* buf = r->http_ts_send_buffer();

    while (left > 0) {
        ssize_t nread = -1;
        int max_read = srs_min(left, SRS_HTTP_TS_SEND_BUFFER_SIZE);
        if ((ret = fs->read(buf, max_read, &nread)) != ERROR_SUCCESS) {
            break;
        }

        left -= nread;
        if ((ret = w->write(buf, (int)nread)) != ERROR_SUCCESS) {
            break;
        }
    }

    return ret;
}

SrsHttpMuxEntry::SrsHttpMuxEntry()
{
    enabled = true;
    explicit_match = false;
    handler = NULL;
}

SrsHttpMuxEntry::~SrsHttpMuxEntry()
{
    srs_freep(handler);
}

SrsHttpServeMux::SrsHttpServeMux()
{
}

SrsHttpServeMux::~SrsHttpServeMux()
{
    std::map<std::string, SrsHttpMuxEntry*>::iterator it;
    for (it = entries.begin(); it != entries.end(); ++it) {
        SrsHttpMuxEntry* entry = it->second;
        srs_freep(entry);
    }
    entries.clear();

    vhosts.clear();
    hijackers.clear();
}

int SrsHttpServeMux::initialize()
{
    int ret = ERROR_SUCCESS;
    // TODO: FIXME: implements it.
    return ret;
}

void SrsHttpServeMux::hijack(ISrsHttpMatchHijacker* h)
{
    std::vector<ISrsHttpMatchHijacker*>::iterator it = ::find(hijackers.begin(), hijackers.end(), h);
    if (it != hijackers.end()) {
        return;
    }
    hijackers.push_back(h);
}

void SrsHttpServeMux::unhijack(ISrsHttpMatchHijacker* h)
{
    std::vector<ISrsHttpMatchHijacker*>::iterator it = ::find(hijackers.begin(), hijackers.end(), h);
    if (it == hijackers.end()) {
        return;
    }
    hijackers.erase(it);
}

int SrsHttpServeMux::handle(std::string pattern, ISrsHttpHandler* handler)
{
    int ret = ERROR_SUCCESS;

    srs_assert(handler);

    if (pattern.empty()) {
        ret = ERROR_HTTP_PATTERN_EMPTY;
        srs_error("http: empty pattern. ret=%d", ret);
        return ret;
    }

    if (entries.find(pattern) != entries.end()) {
        SrsHttpMuxEntry* exists = entries[pattern];
        if (exists->explicit_match) {
            ret = ERROR_HTTP_PATTERN_DUPLICATED;
            srs_error("http: multiple registrations for %s. ret=%d", pattern.c_str(), ret);
            return ret;
        }
    }

    std::string vhost = pattern;
    if (pattern.at(0) != '/') {
        if (pattern.find("/") != string::npos) {
            vhost = pattern.substr(0, pattern.find("/"));
        }
        vhosts[vhost] = handler;
    }

    if (true) {
        SrsHttpMuxEntry* entry = new SrsHttpMuxEntry();
        entry->explicit_match = true;
        entry->handler = handler;
        entry->pattern = pattern;
        entry->handler->entry = entry;

        if (entries.find(pattern) != entries.end()) {
            SrsHttpMuxEntry* exists = entries[pattern];
            srs_freep(exists);
        }
        entries[pattern] = entry;
    }

    // Helpful behavior:
    // If pattern is /tree/, insert an implicit permanent redirect for /tree.
    // It can be overridden by an explicit registration.
    if (pattern != "/" && !pattern.empty() && pattern.at(pattern.length() - 1) == '/') {
        std::string rpattern = pattern.substr(0, pattern.length() - 1);
        SrsHttpMuxEntry* entry = NULL;

        // free the exists not explicit entry
        if (entries.find(rpattern) != entries.end()) {
            SrsHttpMuxEntry* exists = entries[rpattern];
            if (!exists->explicit_match) {
                entry = exists;
            }
        }

        // create implicit redirect.
        if (!entry || entry->explicit_match) {
            srs_freep(entry);

            entry = new SrsHttpMuxEntry();
            entry->explicit_match = false;
            entry->handler = new SrsHttpRedirectHandler(pattern, SRS_CONSTS_HTTP_Found);
            entry->pattern = pattern;
            entry->handler->entry = entry;

            entries[rpattern] = entry;
        }
    }

    return ret;
}

bool SrsHttpServeMux::can_serve(ISrsHttpMessage* r)
{
    int ret = ERROR_SUCCESS;

    ISrsHttpHandler* h = NULL;
    if ((ret = find_handler(r, &h)) != ERROR_SUCCESS) {
        return false;
    }

    srs_assert(h);
    return !h->is_not_found();
}

int SrsHttpServeMux::serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r)
{
    int ret = ERROR_SUCCESS;

    ISrsHttpHandler* h = NULL;
    if ((ret = find_handler(r, &h)) != ERROR_SUCCESS) {
        srs_error("find handler failed. ret=%d", ret);
        return ret;
    }

    srs_assert(h);
    if ((ret = h->serve_http(w, r)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("handler serve http failed. ret=%d", ret);
        }
        return ret;
    }

    return ret;
}

int SrsHttpServeMux::find_handler(ISrsHttpMessage* r, ISrsHttpHandler** ph)
{
    int ret = ERROR_SUCCESS;

    // TODO: FIXME: support the path . and ..
    if (r->url().find("..") != std::string::npos) {
        ret = ERROR_HTTP_URL_NOT_CLEAN;
        srs_error("htt url not canonical, url=%s. ret=%d", r->url().c_str(), ret);
        return ret;
    }

    if ((ret = match(r, ph)) != ERROR_SUCCESS) {
        srs_error("http match handler failed. ret=%d", ret);
        return ret;
    }

    // always hijack.
    if (!hijackers.empty()) {
        // notice all hijacker the match failed.
        std::vector<ISrsHttpMatchHijacker*>::iterator it;
        for (it = hijackers.begin(); it != hijackers.end(); ++it) {
            ISrsHttpMatchHijacker* hijacker = *it;
            if ((ret = hijacker->hijack(r, ph)) != ERROR_SUCCESS) {
                srs_error("hijacker match failed. ret=%d", ret);
                return ret;
            }
        }
    }

    static ISrsHttpHandler* h404 = new SrsHttpNotFoundHandler();
    if (*ph == NULL) {
        *ph = h404;
    }

    return ret;
}

int SrsHttpServeMux::match(ISrsHttpMessage* r, ISrsHttpHandler** ph)
{
    int ret = ERROR_SUCCESS;

    std::string path = r->path();

    // Host-specific pattern takes precedence over generic ones
    if (!vhosts.empty() && vhosts.find(r->host()) != vhosts.end()) {
        path = r->host() + path;
    }

    int nb_matched = 0;
    ISrsHttpHandler* h = NULL;

    std::map<std::string, SrsHttpMuxEntry*>::iterator it;
    for (it = entries.begin(); it != entries.end(); ++it) {
        std::string pattern = it->first;
        SrsHttpMuxEntry* entry = it->second;

        if (!entry->enabled) {
            continue;
        }

        if (!path_match(pattern, path)) {
            continue;
        }

        if (!h || (int)pattern.length() > nb_matched) {
            nb_matched = (int)pattern.length();
            h = entry->handler;
        }
    }

    *ph = h;

    return ret;
}

bool SrsHttpServeMux::path_match(string pattern, string path)
{
    if (pattern.empty()) {
        return false;
    }

    int n = (int)pattern.length();

    // not endswith '/', exactly match.
    if (pattern.at(n - 1) != '/') {
        return pattern == path;
    }

    // endswith '/', match any,
    // for example, '/api/' match '/api/[N]'
    if ((int)path.length() >= n) {
        if (memcmp(pattern.data(), path.data(), n) == 0) {
            return true;
        }
    }

    return false;
}

SrsHttpCorsMux::SrsHttpCorsMux()
{
    next = NULL;
    enabled = false;
    required = false;
}

SrsHttpCorsMux::~SrsHttpCorsMux()
{
}

int SrsHttpCorsMux::initialize(ISrsHttpServeMux* worker, bool cros_enabled)
{
    next = worker;
    enabled = cros_enabled;

    return ERROR_SUCCESS;
}

int SrsHttpCorsMux::serve_http(ISrsHttpResponseWriter* w, ISrsHttpMessage* r)
{
    // If CORS enabled, and there is a "Origin" header, it's CORS.
    if (enabled) {
        for (int i = 0; i < r->request_header_count(); i++) {
            string k = r->request_header_key_at(i);
            if (k == "Origin" || k == "origin") {
                required = true;
                break;
            }
        }
    }

    // When CORS required, set the CORS headers.
    if (required) {
        SrsHttpHeader* h = w->header();
        h->set("Access-Control-Allow-Origin", "*");
        h->set("Access-Control-Allow-Methods", "GET, POST, HEAD, PUT, DELETE, OPTIONS");
        h->set("Access-Control-Expose-Headers", "Server,range,Content-Length,Content-Range");
        h->set("Access-Control-Allow-Headers", "origin,range,accept-encoding,referer,Cache-Control,X-Proxy-Authorization,X-Requested-With,Content-Type");
    }

    // handle the http options.
    if (r->is_http_options()) {
        w->header()->set_content_length(0);
        if (enabled) {
            w->write_header(SRS_CONSTS_HTTP_OK);
        } else {
            w->write_header(SRS_CONSTS_HTTP_MethodNotAllowed);
        }
        return w->final_request();
    }

    srs_assert(next);
    return next->serve_http(w, r);
}

SrsHttpUri::SrsHttpUri()
{
    port = SRS_DEFAULT_HTTP_PORT;
}

SrsHttpUri::~SrsHttpUri()
{
}

int SrsHttpUri::initialize(string _url)
{
    int ret = ERROR_SUCCESS;

    schema = host = path = query = "";

    url = _url;
    const char* purl = url.c_str();

    http_parser_url hp_u;
    if((ret = http_parser_parse_url(purl, url.length(), 0, &hp_u)) != 0){
        int code = ret;
        ret = ERROR_HTTP_PARSE_URI;

        srs_error("parse url %s failed, code=%d, ret=%d", purl, code, ret);
        return ret;
    }

    std::string field = get_uri_field(url, &hp_u, UF_SCHEMA);
    if(!field.empty()){
        schema = field;
    }

    host = get_uri_field(url, &hp_u, UF_HOST);

    field = get_uri_field(url, &hp_u, UF_PORT);
    if(!field.empty()){
        port = atoi(field.c_str());
    }
    if(port<=0){
        port = SRS_DEFAULT_HTTP_PORT;
    }

    path = get_uri_field(url, &hp_u, UF_PATH);
    srs_info("parse url %s success", purl);

    query = get_uri_field(url, &hp_u, UF_QUERY);
    srs_info("parse query %s success", query.c_str());

    raw_path = path;
    if (!query.empty()) {
       raw_path += "?" + query;
    }

    return ret;
}

const char* SrsHttpUri::get_url()
{
    return url.data();
}

const char* SrsHttpUri::get_schema()
{
    return schema.data();
}

const char* SrsHttpUri::get_host()
{
    return host.data();
}

int SrsHttpUri::get_port()
{
    return port;
}

const char* SrsHttpUri::get_path()
{
    return path.data();
}

const char* SrsHttpUri::get_query()
{
    return query.data();
}

const char* SrsHttpUri::get_raw_path()
{
    return raw_path.data();
}


string SrsHttpUri::get_uri_field(string uri, http_parser_url* hp_u, http_parser_url_fields field)
{
    if((hp_u->field_set & (1 << field)) == 0){
        return "";
    }

    srs_verbose("uri field matched, off=%d, len=%d, value=%.*s",
                hp_u->field_data[field].off,
                hp_u->field_data[field].len,
                hp_u->field_data[field].len,
                uri.c_str() + hp_u->field_data[field].off);

    int offset = hp_u->field_data[field].off;
    int len = hp_u->field_data[field].len;

    return uri.substr(offset, len);
}


SrsHttpResponseWriter::SrsHttpResponseWriter(SimpleSocketStream* io)
{
    skt = io;
    hdr = new SrsHttpHeader();
    header_wrote = false;
    status = SRS_CONSTS_HTTP_OK;
    content_length = -1;
    written = 0;
    header_sent = false;
    nb_iovss_cache = 0;
    iovss_cache = NULL;
    header_cache[0] = '\0';
}

SrsHttpResponseWriter::~SrsHttpResponseWriter()
{
    srs_freep(hdr);
    srs_freepa(iovss_cache);
}

SimpleSocketStream* SrsHttpResponseWriter::srs_st_socket()
{
    return skt;
}

int SrsHttpResponseWriter::final_request()
{
    // write the header data in memory.
    if (!header_wrote) {
        write_header(SRS_CONSTS_HTTP_OK);
    }

    // complete the chunked encoding.
    if (content_length == -1) {
        std::stringstream ss;
        ss << 0 << SRS_HTTP_CRLF << SRS_HTTP_CRLF;
        std::string ch = ss.str();
        return skt->write((void*)ch.data(), (int)ch.length(), NULL);
    }

    // flush when send with content length
    return write(NULL, 0);
}

SrsHttpHeader* SrsHttpResponseWriter::header()
{
    return hdr;
}

int SrsHttpResponseWriter::write(char* data, int size)
{
    int ret = ERROR_SUCCESS;

    // write the header data in memory.
    if (!header_wrote) {
        write_header(SRS_CONSTS_HTTP_OK);
    }

    // whatever header is wrote, we should try to send header.
    if ((ret = send_header(data, size)) != ERROR_SUCCESS) {
        srs_error("http: send header failed. ret=%d", ret);
        return ret;
    }

    // check the bytes send and content length.
    written += size;
    if (content_length != -1 && written > content_length) {
        ret = ERROR_HTTP_CONTENT_LENGTH;
        srs_error("http: exceed content length. ret=%d", ret);
        return ret;
    }

    // ignore NULL content.
    if (!data) {
        return ret;
    }

    // directly send with content length
    if (content_length != -1) {
        return skt->write((void*)data, size, NULL);
    }

    // send in chunked encoding.
    int nb_size = snprintf(header_cache, SRS_HTTP_HEADER_CACHE_SIZE, "%x", size);

    iovec iovs[4];
    iovs[0].iov_base = (char*)header_cache;
    iovs[0].iov_len = (int)nb_size;
    iovs[1].iov_base = (char*)SRS_HTTP_CRLF;
    iovs[1].iov_len = 2;
    iovs[2].iov_base = (char*)data;
    iovs[2].iov_len = size;
    iovs[3].iov_base = (char*)SRS_HTTP_CRLF;
    iovs[3].iov_len = 2;

    ssize_t nwrite;
    if ((ret = skt->writev(iovs, 4, &nwrite)) != ERROR_SUCCESS) {
        return ret;
    }

    return ret;
}

int SrsHttpResponseWriter::writev(iovec* iov, int iovcnt, ssize_t* pnwrite)
{
    int ret = ERROR_SUCCESS;

    // when header not ready, or not chunked, send one by one.
    if (!header_wrote || content_length != -1) {
        ssize_t nwrite = 0;
        for (int i = 0; i < iovcnt; i++) {
            iovec* piovc = iov + i;
            nwrite += piovc->iov_len;
            if ((ret = write((char*)piovc->iov_base, (int)piovc->iov_len)) != ERROR_SUCCESS) {
                return ret;
            }
        }

        if (pnwrite) {
            *pnwrite = nwrite;
        }

        return ret;
    }

    // ignore NULL content.
    if (iovcnt <= 0) {
        return ret;
    }

    // send in chunked encoding.
    int nb_iovss = 3 + iovcnt;
    iovec* iovss = iovss_cache;
    if (nb_iovss_cache < nb_iovss) {
        srs_freepa(iovss_cache);
        nb_iovss_cache = nb_iovss;
        iovss = iovss_cache = new iovec[nb_iovss];
    }

    // send in chunked encoding.

    // chunk size.
    int size = 0;
    for (int i = 0; i < iovcnt; i++) {
        iovec* data_iov = iov + i;
        size += data_iov->iov_len;
    }
    written += size;

    // chunk header
    int nb_size = snprintf(header_cache, SRS_HTTP_HEADER_CACHE_SIZE, "%x", size);
    iovec* iovs = iovss;
    iovs[0].iov_base = (char*)header_cache;
    iovs[0].iov_len = (int)nb_size;
    iovs++;

    // chunk header eof.
    iovs[0].iov_base = (char*)SRS_HTTP_CRLF;
    iovs[0].iov_len = 2;
    iovs++;

    // chunk body.
    for (int i = 0; i < iovcnt; i++) {
        iovec* data_iov = iov + i;
        iovs[0].iov_base = (char*)data_iov->iov_base;
        iovs[0].iov_len = (int)data_iov->iov_len;
        iovs++;
    }

    // chunk body eof.
    iovs[0].iov_base = (char*)SRS_HTTP_CRLF;
    iovs[0].iov_len = 2;
    iovs++;

    // sendout all ioves.
    ssize_t nwrite;
    if ((ret = srs_write_large_iovs(skt, iovss, nb_iovss, &nwrite)) != ERROR_SUCCESS) {
        return ret;
    }

    if (pnwrite) {
        *pnwrite = nwrite;
    }

    return ret;
}

void SrsHttpResponseWriter::write_header(int code)
{
    if (header_wrote) {
        srs_warn("http: multiple write_header calls, code=%d", code);
        return;
    }

    header_wrote = true;
    status = code;

    // parse the content length from header.
    content_length = hdr->content_length();
}

int SrsHttpResponseWriter::send_header(char* data, int size)
{
    int ret = ERROR_SUCCESS;

    if (header_sent) {
        return ret;
    }
    header_sent = true;

    std::stringstream ss;

    // status_line
    ss << "HTTP/1.1 " << status << " "
        << srs_generate_http_status_text(status) << SRS_HTTP_CRLF;

    // detect content type
    if (srs_go_http_body_allowd(status)) {
        if (hdr->content_type().empty()) {
            hdr->set_content_type(srs_go_http_detect(data, size));
        }
    }

    // set server if not set.
    if (hdr->get("Server").empty()) {
        hdr->set("Server", RTMP_SIG_SRS_SERVER);
    }

    // chunked encoding
    if (content_length == -1) {
        hdr->set("Transfer-Encoding", "chunked");
    }

    // keep alive to make vlc happy.
    hdr->set("Connection", "Keep-Alive");

    // write headers
    hdr->write(ss);

    // header_eof
    ss << SRS_HTTP_CRLF;

    std::string buf = ss.str();
    //hdr->set_header_length(buf.length());
    return skt->write((void*)buf.c_str(), buf.length(), NULL);
}

SrsHttpResponseReader::SrsHttpResponseReader(SrsHttpMessage* msg, SimpleSocketStream* io)
{
    skt = io;
    owner = msg;
    is_eof = false;
    nb_total_read = 0;
    nb_left_chunk = 0;
    buffer = NULL;
    nb_chunk = 0;
}

SrsHttpResponseReader::~SrsHttpResponseReader()
{
}

int SrsHttpResponseReader::initialize(SrsFastBuffer* body)
{
    int ret = ERROR_SUCCESS;

    nb_chunk = 0;
    nb_left_chunk = 0;
    nb_total_read = 0;
    buffer = body;

    return ret;
}

bool SrsHttpResponseReader::eof()
{
    return is_eof;
}

int SrsHttpResponseReader::read_full(char* data, int nb_data, int *nb_read)
{
    int ret = ERROR_SUCCESS;
    int nsize = 0;
    int nleft = nb_data;
    while (true) {
        ret = read(data, nleft, &nsize);
        if (ret != ERROR_SUCCESS) {
            return ret;
        }

        data = data + nsize;
        nleft = nleft - nsize;
        nsize = 0;
        if (nleft < 0) {
            ret = ERROR_SYSTEM_IO_INVALID;
            return ret;
        }

        if (nleft == 0) {
            *nb_read = nb_data;
            break;
        }
    }

    return ret;
}

int SrsHttpResponseReader::read(char* data, int nb_data, int* nb_read)
{

    int ret = ERROR_SUCCESS;

    if (is_eof) {
        ret = ERROR_HTTP_RESPONSE_EOF;
        srs_error("http: response EOF. ret=%d, cont len=%d", ret, (int)owner->content_length());
        return ret;
    }

    // chunked encoding.
    if (owner->is_chunked()) {
        return read_chunked(data, nb_data, nb_read);
    }

    // read by specified content-length
    if (owner->content_length() != -1) {
        int max = (int)owner->content_length() - (int)nb_total_read;
        if (max <= 0) {
            is_eof = true;
            return ret;
        }

        // change the max to read.
        nb_data = srs_min(nb_data, max);
        return read_specified(data, nb_data, nb_read);
    }

    // infinite chunked mode, directly read.
    if (owner->is_infinite_chunked()) {
        srs_assert(!owner->is_chunked() && owner->content_length() == -1);
        return read_specified(data, nb_data, nb_read);
    }

    // infinite chunked mode, but user not set it,
    // we think there is no data left.
    is_eof = true;

    return ret;
}

int SrsHttpResponseReader::read_chunked(char* data, int nb_data, int* nb_read)
{
    int ret = ERROR_SUCCESS;

    // when no bytes left in chunk,
    // parse the chunk length first.
    if (nb_left_chunk <= 0) {
        char* at = NULL;
        int length = 0;
        while (!at) {
            // find the CRLF of chunk header end.
            char* start = buffer->bytes();
            char* end = start + buffer->size();
            for (char* p = start; p < end - 1; p++) {
                if (p[0] == SRS_HTTP_CR && p[1] == SRS_HTTP_LF) {
                    // invalid chunk, ignore.
                    if (p == start) {
                        ret = ERROR_HTTP_INVALID_CHUNK_HEADER;
                        srs_error("chunk header start with CRLF. ret=%d", ret);
                        return ret;
                    }
                    length = (int)(p - start + 2);
                    at = buffer->read_slice(length);
                    break;
                }
            }

            // got at, ok.
            if (at) {
                break;
            }

            // when empty, only grow 1bytes, but the buffer will cache more.
            if ((ret = buffer->grow(skt, buffer->size() + 1)) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret)) {
                    srs_error("read body from server failed. ret=%d", ret);
                }
                return ret;
            }
        }
        srs_assert(length >= 3);

        // it's ok to set the pos and pos+1 to NULL.
        at[length - 1] = 0;
        at[length - 2] = 0;

        // size is the bytes size, excludes the chunk header and end CRLF.
        int ilength = (int)::strtol(at, NULL, 16);
        if (ilength < 0) {
            ret = ERROR_HTTP_INVALID_CHUNK_HEADER;
            srs_error("chunk header negative, length=%d. ret=%d", ilength, ret);
            return ret;
        }

        // all bytes in chunk is left now.
        nb_chunk = nb_left_chunk = ilength;
    }

    if (nb_chunk <= 0) {
        // for the last chunk, eof.
        is_eof = true;
    } else {
        // for not the last chunk, there must always exists bytes.
        // left bytes in chunk, read some.
        srs_assert(nb_left_chunk);

        int nb_bytes = srs_min(nb_left_chunk, nb_data);
        ret = read_specified(data, nb_bytes, &nb_bytes);

        // the nb_bytes used for output already read size of bytes.
        if (nb_read) {
            *nb_read = nb_bytes;
        }
        nb_left_chunk -= nb_bytes;
        srs_info("http: read %d bytes of chunk", nb_bytes);

        // error or still left bytes in chunk, ignore and read in future.
        if (nb_left_chunk > 0 || (ret != ERROR_SUCCESS)) {
            return ret;
        }
        srs_info("http: read total chunk %dB", nb_chunk);
    }

    // for both the last or not, the CRLF of chunk payload end.
    if ((ret = buffer->grow(skt, 2)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("read EOF of chunk from server failed. ret=%d", ret);
        }
        return ret;
    }
    buffer->read_slice(2);

    return ret;
}

int SrsHttpResponseReader::read_specified(char* data, int nb_data, int* nb_read)
{
    int ret = ERROR_SUCCESS;

    if (buffer->size() <= 0) {
        // when empty, only grow 1bytes, but the buffer will cache more.
        if ((ret = buffer->grow(skt, 1)) != ERROR_SUCCESS) {
            if (!srs_is_client_gracefully_close(ret)) {
                srs_error("read body from server failed. ret=%d", ret);
            }
            return ret;
        }
    }

    int nb_bytes = srs_min(nb_data, buffer->size());

    // read data to buffer.
    srs_assert(nb_bytes);
    char* p = buffer->read_slice(nb_bytes);
    memcpy(data, p, nb_bytes);
    if (nb_read) {
        *nb_read = nb_bytes;
    }

    // increase the total read to determine whether EOF.
    nb_total_read += nb_bytes;

    // for not chunked and specified content length.
    if (!owner->is_chunked() && owner->content_length() != -1) {
        // when read completed, eof.
        if (nb_total_read >= (int)owner->content_length()) {
            is_eof = true;
        }
    }

    return ret;
}

SrsHttpParser::SrsHttpParser()
{
    expect_field_name = false;
    header_parsed = 0;
    buffer = new SrsFastBuffer();
}

SrsHttpParser::~SrsHttpParser()
{
    srs_freep(buffer);
}

int SrsHttpParser::initialize(enum http_parser_type type)
{
    int ret = ERROR_SUCCESS;

    memset(&settings, 0, sizeof(settings));
    settings.on_message_begin = on_message_begin;
    settings.on_url = on_url;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;

    http_parser_init(&parser, type);
    // callback object ptr.
    parser.data = (void*)this;

    return ret;
}

int SrsHttpParser::parse_message(SimpleSocketStream* skt, ISrsHttpMessage** ppmsg)
{
    *ppmsg = NULL;

    int ret = ERROR_SUCCESS;

    // reset request data.
    field_name = "";
    field_value = "";
    expect_field_name = true;
    state = SrsHttpParseStateInit;
    header = http_parser();
    url = "";
    headers.clear();
    header_parsed = 0;

    // do parse
    if ((ret = parse_message_imp(skt)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("parse http msg failed. ret=%d", ret);
        }
        return ret;
    }

    // create msg
    SrsHttpMessage* msg = new SrsHttpMessage(skt);

    // initalize http msg, parse url.
    if ((ret = msg->update(url, &header, buffer, headers)) != ERROR_SUCCESS) {
        srs_error("initialize http msg failed. ret=%d", ret);
        srs_freep(msg);
        return ret;
    }

    // parse ok, return the msg.
    *ppmsg = msg;

    return ret;
}

int SrsHttpParser::parse_message_imp(SimpleSocketStream* skt)
{
    int ret = ERROR_SUCCESS;

    while (true) {
        ssize_t nparsed = 0;

        // when got entire http header, parse it.
        // @see https://github.com/ossrs/srs/issues/400
        char* start = buffer->bytes();
        char* end = start + buffer->size();
        for (char* p = start; p <= end - 4; p++) {
            // SRS_HTTP_CRLFCRLF "\r\n\r\n" // 0x0D0A0D0A
            if (p[0] == SRS_CONSTS_CR && p[1] == SRS_CONSTS_LF && p[2] == SRS_CONSTS_CR && p[3] == SRS_CONSTS_LF) {
                nparsed = http_parser_execute(&parser, &settings, buffer->bytes(), buffer->size());
                srs_info("buffer=%d, nparsed=%d, header=%d", buffer->size(), (int)nparsed, header_parsed);
                break;
            }
        }

        // consume the parsed bytes.
        if (nparsed && header_parsed) {
            buffer->read_slice(header_parsed);
            srs_info("temp: size=%d,  %.*s", buffer->size(), buffer->size(), buffer->bytes());
        }

        // ok atleast header completed,
        // never wait for body completed, for maybe chunked.
        if (state == SrsHttpParseStateHeaderComplete || state == SrsHttpParseStateMessageComplete) {
            break;
        }

        // when nothing parsed, read more to parse.
        if (nparsed == 0) {
            // when requires more, only grow 1bytes, but the buffer will cache more.
            if ((ret = buffer->grow(skt, buffer->size() + 1)) != ERROR_SUCCESS) {
                if (!srs_is_client_gracefully_close(ret)) {
                    srs_error("read body from server failed. ret=%d", ret);
                }
                return ret;
            }
        }
    }

    // parse last header.
    if (!field_name.empty() && !field_value.empty()) {
        headers.push_back(std::make_pair(field_name, field_value));
    }

    return ret;
}

int SrsHttpParser::on_message_begin(http_parser* parser)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    srs_assert(obj);

    obj->state = SrsHttpParseStateStart;

    srs_info("***MESSAGE BEGIN***");

    return 0;
}

int SrsHttpParser::on_headers_complete(http_parser* parser)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    srs_assert(obj);

    obj->header = *parser;
    // save the parser when header parse completed.
    obj->state = SrsHttpParseStateHeaderComplete;
    obj->header_parsed = (int)parser->nread;

    srs_info("***HEADERS COMPLETE***");

    // see http_parser.c:1570, return 1 to skip body.
    return 0;
}

int SrsHttpParser::on_message_complete(http_parser* parser)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    srs_assert(obj);

    // save the parser when body parse completed.
    obj->state = SrsHttpParseStateMessageComplete;

    srs_info("***MESSAGE COMPLETE***\n");

    return 0;
}

int SrsHttpParser::on_url(http_parser* parser, const char* at, size_t length)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    srs_assert(obj);

    if (length > 0) {
        obj->url.append(at, (int)length);
    }

    srs_info("Method: %d, Url: %.*s", parser->method, (int)length, at);

    return 0;
}

int SrsHttpParser::on_header_field(http_parser* parser, const char* at, size_t length)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    srs_assert(obj);

    // field value=>name, reap the field.
    if (!obj->expect_field_name) {
        obj->headers.push_back(std::make_pair(obj->field_name, obj->field_value));

        // reset the field name when parsed.
        obj->field_name = "";
        obj->field_value = "";
    }
    obj->expect_field_name = true;

    if (length > 0) {
        obj->field_name.append(at, (int)length);
    }

    srs_info("Header field(%d bytes): %.*s", (int)length, (int)length, at);
    return 0;
}

int SrsHttpParser::on_header_value(http_parser* parser, const char* at, size_t length)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    srs_assert(obj);

    if (length > 0) {
        obj->field_value.append(at, (int)length);
    }
    obj->expect_field_name = false;

    srs_info("Header value(%d bytes): %.*s", (int)length, (int)length, at);
    return 0;
}

int SrsHttpParser::on_body(http_parser* parser, const char* at, size_t length)
{
    SrsHttpParser* obj = (SrsHttpParser*)parser->data;
    srs_assert(obj);

    srs_info("Body:len:%d,  %.*s", (int)length,(int)length, at);

    return 0;
}

SrsHttpMessage::SrsHttpMessage(SimpleSocketStream* io) : ISrsHttpMessage()
{
    chunked = false;
    infinite_chunked = false;
    keep_alive = true;
    _uri = new SrsHttpUri();
    _body = new SrsHttpResponseReader(this, io);
    _http_ts_send_buffer = new char[SRS_HTTP_TS_SEND_BUFFER_SIZE];
    jsonp = false;
}

SrsHttpMessage::~SrsHttpMessage()
{
    srs_freep(_body);
    srs_freep(_uri);
    srs_freepa(_http_ts_send_buffer);
}

int SrsHttpMessage::update(string url, http_parser* header, SrsFastBuffer* body, vector<SrsHttpHeaderField>& headers)
{
    int ret = ERROR_SUCCESS;

    _url = url;
    _header = *header;
    _headers = headers;

    // whether chunked.
    std::string transfer_encoding = get_request_header("Transfer-Encoding");
    chunked = (transfer_encoding == "chunked");

    // whether keep alive.
    keep_alive = http_should_keep_alive(header);

    // set the buffer.
    if ((ret = _body->initialize(body)) != ERROR_SUCCESS) {
        return ret;
    }

    // parse uri from url.
    std::string host = get_request_header("Host");

    // use server public ip when no host specified.
    // to make telnet happy.
    if (host.empty()) {
        host= srs_get_public_internet_address();
    }

    // parse uri to schema/server:port/path?query
    std::string uri = "http://" + host + _url;
    if ((ret = _uri->initialize(uri)) != ERROR_SUCCESS) {
        return ret;
    }

    // must format as key=value&...&keyN=valueN
    std::string q = _uri->get_query();
    size_t pos = string::npos;
    while (!q.empty()) {
        std::string k = q;
        if ((pos = q.find("=")) != string::npos) {
            k = q.substr(0, pos);
            q = q.substr(pos + 1);
        } else {
            q = "";
        }

        std::string v = q;
        if ((pos = q.find("&")) != string::npos) {
            v = q.substr(0, pos);
            q = q.substr(pos + 1);
        } else {
            q = "";
        }

        _query[k] = v;
    }

    // parse ext.
    _ext = _uri->get_path();
    if ((pos = _ext.rfind(".")) != string::npos) {
        _ext = _ext.substr(pos);
    } else {
        _ext = "";
    }

    // parse jsonp request message.
    if (!query_get("callback").empty()) {
        jsonp = true;
    }
    if (jsonp) {
        jsonp_method = query_get("method");
    }
    return ret;
}

SrsHttpResponseReader* SrsHttpMessage::get_http_response_reader()
{
    return _body;
}

u_int8_t SrsHttpMessage::method()
{
    if (jsonp && !jsonp_method.empty()) {
        if (jsonp_method == "GET") {
            return SRS_CONSTS_HTTP_GET;
        } else if (jsonp_method == "PUT") {
            return SRS_CONSTS_HTTP_PUT;
        } else if (jsonp_method == "POST") {
            return SRS_CONSTS_HTTP_POST;
        } else if (jsonp_method == "DELETE") {
            return SRS_CONSTS_HTTP_DELETE;
        }
    }

    return (u_int8_t)_header.method;
}

u_int16_t SrsHttpMessage::status_code()
{
    return (u_int16_t)_header.status_code;
}

string SrsHttpMessage::method_str()
{
    if (jsonp && !jsonp_method.empty()) {
        return jsonp_method;
    }

    if (is_http_get()) {
        return "GET";
    }
    if (is_http_put()) {
        return "PUT";
    }
    if (is_http_post()) {
        return "POST";
    }
    if (is_http_delete()) {
        return "DELETE";
    }
    if (is_http_options()) {
        return "OPTIONS";
    }

    return "OTHER";
}

bool SrsHttpMessage::is_http_get()
{
    return method() == SRS_CONSTS_HTTP_GET;
}

bool SrsHttpMessage::is_http_put()
{
    return method() == SRS_CONSTS_HTTP_PUT;
}

bool SrsHttpMessage::is_http_post()
{
    return method() == SRS_CONSTS_HTTP_POST;
}

bool SrsHttpMessage::is_http_delete()
{
    return method() == SRS_CONSTS_HTTP_DELETE;
}

bool SrsHttpMessage::is_http_options()
{
    return _header.method == SRS_CONSTS_HTTP_OPTIONS;
}

bool SrsHttpMessage::is_chunked()
{
    return chunked;
}

bool SrsHttpMessage::is_keep_alive()
{
    return keep_alive;
}

bool SrsHttpMessage::is_infinite_chunked()
{
    return infinite_chunked;
}

string SrsHttpMessage::uri()
{
    std::string uri = _uri->get_schema();
    if (uri.empty()) {
        uri += "http";
    }
    uri += "://";

    uri += host();
    uri += path();

    return uri;
}

string SrsHttpMessage::url()
{
    return _uri->get_url();
}

string SrsHttpMessage::host()
{
    return _uri->get_host();
}

string SrsHttpMessage::path()
{
    return _uri->get_path();
}

string SrsHttpMessage::query()
{
    return _uri->get_query();
}

string SrsHttpMessage::ext()
{
    return _ext;
}

int SrsHttpMessage::parse_rest_id(string pattern)
{
    string p = _uri->get_path();
    if (p.length() <= pattern.length()) {
        return -1;
    }

    string id = p.substr((int)pattern.length());
    if (!id.empty()) {
        return ::atoi(id.c_str());
    }

    return -1;
}

int SrsHttpMessage::enter_infinite_chunked()
{
    int ret = ERROR_SUCCESS;

    if (infinite_chunked) {
        return ret;
    }

    if (is_chunked() || content_length() != -1) {
        ret = ERROR_HTTP_DATA_INVALID;
        srs_error("infinite chunkted not supported in specified codec. ret=%d", ret);
        return ret;
    }

    infinite_chunked = true;

    return ret;
}

int SrsHttpMessage::body_read_all(string& body)
{
    int ret = ERROR_SUCCESS;

    // cache to read.
    char* buf = new char[SRS_HTTP_READ_CACHE_BYTES];
    SrsAutoFreeA(char, buf);

    // whatever, read util EOF.
    while (!_body->eof()) {
        int nb_read = 0;
        if ((ret = _body->read(buf, SRS_HTTP_READ_CACHE_BYTES, &nb_read)) != ERROR_SUCCESS) {
            return ret;
        }

        if (nb_read > 0) {
            body.append(buf, nb_read);
        }
    }

    return ret;
}

SrsFastBuffer::SrsFastBuffer()
{
#ifdef SRS_PERF_MERGED_READ
    merged_read = false;
    _handler = NULL;
#endif

    nb_buffer = SRS_DEFAULT_RECV_BUFFER_SIZE;
    buffer = (char*)malloc(nb_buffer);
    p = end = buffer;
}

SrsFastBuffer::~SrsFastBuffer()
{
    free(buffer);
    buffer = NULL;
}

int SrsFastBuffer::size()
{
    return (int)(end - p);
}

char* SrsFastBuffer::bytes()
{
    return p;
}

int SrsFastBuffer::update(char* data, int required_size)
{
    int ret = ERROR_SUCCESS;

    // must be positive.
    srs_assert(required_size > 0);

    // the free space of buffer,
    //      buffer = consumed_bytes + exists_bytes + free_space.
    int nb_free_space = (int)(buffer + nb_buffer - end);

    // the bytes already in buffer
    int nb_exists_bytes = (int)(end - p);
    srs_assert(nb_exists_bytes >= 0);

    // resize the space when no left space.
    if (nb_free_space < required_size) {
        srs_verbose("move fast buffer %d bytes", nb_exists_bytes);

        // reset or move to get more space.
        if (!nb_exists_bytes) {
            // reset when buffer is empty.
            p = end = buffer;
            srs_verbose("all consumed, reset fast buffer");
        } else if (nb_exists_bytes < nb_buffer && p > buffer) {
            // move the left bytes to start of buffer.
            // @remark Only move memory when space is enough, or failed at next check.
            // @see https://github.com/ossrs/srs/issues/848
            buffer = (char*)memmove(buffer, p, nb_exists_bytes);
            p = buffer;
            end = p + nb_exists_bytes;
        }

        // check whether enough free space in buffer.
        nb_free_space = (int)(buffer + nb_buffer - end);
        if (nb_free_space < required_size) {
            ret = ERROR_READER_BUFFER_OVERFLOW;
            srs_error("buffer overflow, required=%d, max=%d, left=%d, ret=%d",
                required_size, nb_buffer, nb_free_space, ret);
            return ret;
        }
    }

    memcpy(end, data, required_size);
    end += required_size;
    nb_free_space -= required_size;

    return ret;
}

void SrsFastBuffer::set_buffer(int buffer_size)
{
    // never exceed the max size.
    if (buffer_size > SRS_MAX_SOCKET_BUFFER) {
        srs_warn("limit the user-space buffer from %d to %d",
            buffer_size, SRS_MAX_SOCKET_BUFFER);
    }

    // the user-space buffer size limit to a max value.
    int nb_resize_buf = srs_min(buffer_size, SRS_MAX_SOCKET_BUFFER);

    // only realloc when buffer changed bigger
    if (nb_resize_buf <= nb_buffer) {
        return;
    }

    // realloc for buffer change bigger.
    int start = (int)(p - buffer);
    int nb_bytes = (int)(end - p);

    buffer = (char*)realloc(buffer, nb_resize_buf);
    nb_buffer = nb_resize_buf;
    p = buffer + start;
    end = p + nb_bytes;
}

char SrsFastBuffer::read_1byte()
{
    srs_assert(end - p >= 1);
    return *p++;
}

char* SrsFastBuffer::read_slice(int size)
{
    srs_assert(size >= 0);
    srs_assert(end - p >= size);
    srs_assert(p + size >= buffer);

    char* ptr = p;
    p += size;

    return ptr;
}

void SrsFastBuffer::skip(int size)
{
    srs_assert(end - p >= size);
    srs_assert(p + size >= buffer);
    p += size;
}

int SrsFastBuffer::grow(ISrsHttpResponseReader* reader, int required_size)
{
    int ret = ERROR_SUCCESS;

    // already got required size of bytes.
    if (end - p >= required_size) {
        return ret;
    }

    // must be positive.
    srs_assert(required_size > 0);

    // the free space of buffer,
    //      buffer = consumed_bytes + exists_bytes + free_space.
    int nb_free_space = (int)(buffer + nb_buffer - end);

    // the bytes already in buffer
    int nb_exists_bytes = (int)(end - p);
    srs_assert(nb_exists_bytes >= 0);

    // resize the space when no left space.
    if (nb_free_space < required_size - nb_exists_bytes) {
        srs_verbose("move fast buffer %d bytes", nb_exists_bytes);

        // reset or move to get more space.
        if (!nb_exists_bytes) {
            // reset when buffer is empty.
            p = end = buffer;
            srs_verbose("all consumed, reset fast buffer");
        } else if (nb_exists_bytes < nb_buffer && p > buffer) {
            // move the left bytes to start of buffer.
            // @remark Only move memory when space is enough, or failed at next check.
            // @see https://github.com/ossrs/srs/issues/848
            buffer = (char*)memmove(buffer, p, nb_exists_bytes);
            p = buffer;
            end = p + nb_exists_bytes;
        }

        // check whether enough free space in buffer.
        nb_free_space = (int)(buffer + nb_buffer - end);
        if (nb_free_space < required_size - nb_exists_bytes) {
            ret = ERROR_READER_BUFFER_OVERFLOW;
            srs_error("buffer overflow, required=%d, max=%d, left=%d, ret=%d",
                required_size, nb_buffer, nb_free_space, ret);
            return ret;
        }
    }

    // buffer is ok, read required size of bytes.
    while (end - p < required_size) {
        int nread;
        if ((ret = reader->read(end, nb_free_space, &nread)) != ERROR_SUCCESS) {
            return ret;
        }

#ifdef SRS_PERF_MERGED_READ
        /**
        * to improve read performance, merge some packets then read,
        * when it on and read small bytes, we sleep to wait more data.,
        * that is, we merge some data to read together.
        * @see https://github.com/ossrs/srs/issues/241
        */
        if (merged_read && _handler) {
            _handler->on_read(nread);
        }
#endif
        // we just move the ptr to next.
        srs_assert((int)nread > 0);
        end += nread;
        nb_free_space -= nread;
    }
    return ret;
}

int SrsFastBuffer::grow(ISrsProtocolReaderWriter* reader, int required_size)
{
    int ret = ERROR_SUCCESS;

    // already got required size of bytes.
    if (end - p >= required_size) {
        return ret;
    }

    // must be positive.
    srs_assert(required_size > 0);

    // the free space of buffer,
    //      buffer = consumed_bytes + exists_bytes + free_space.
    int nb_free_space = (int)(buffer + nb_buffer - end);

    // the bytes already in buffer
    int nb_exists_bytes = (int)(end - p);
    srs_assert(nb_exists_bytes >= 0);

    // resize the space when no left space.
    if (nb_free_space < required_size - nb_exists_bytes) {
        srs_verbose("move fast buffer %d bytes", nb_exists_bytes);

        // reset or move to get more space.
        if (!nb_exists_bytes) {
            // reset when buffer is empty.
            p = end = buffer;
            srs_verbose("all consumed, reset fast buffer");
        } else if (nb_exists_bytes < nb_buffer && p > buffer) {
            // move the left bytes to start of buffer.
            // @remark Only move memory when space is enough, or failed at next check.
            // @see https://github.com/ossrs/srs/issues/848
            buffer = (char*)memmove(buffer, p, nb_exists_bytes);
            p = buffer;
            end = p + nb_exists_bytes;
        }

        // check whether enough free space in buffer.
        nb_free_space = (int)(buffer + nb_buffer - end);
        if (nb_free_space < required_size - nb_exists_bytes) {
            ret = ERROR_READER_BUFFER_OVERFLOW;
            srs_error("buffer overflow, required=%d, max=%d, left=%d, ret=%d",
                required_size, nb_buffer, nb_free_space, ret);
            return ret;
        }
    }

    // buffer is ok, read required size of bytes.
    while (end - p < required_size) {
        int64_t nread;
        if ((ret = reader->read((void*)end, nb_free_space, &nread)) != ERROR_SUCCESS) {
            return ret;
        }

#ifdef SRS_PERF_MERGED_READ
        /**
        * to improve read performance, merge some packets then read,
        * when it on and read small bytes, we sleep to wait more data.,
        * that is, we merge some data to read together.
        * @see https://github.com/ossrs/srs/issues/241
        */
        if (merged_read && _handler) {
            _handler->on_read(nread);
        }
#endif
        // we just move the ptr to next.
        srs_assert((int)nread > 0);
        end += nread;
        nb_free_space -= nread;
    }
    return ret;
}


#ifdef SRS_PERF_MERGED_READ
void SrsFastBuffer::set_merge_read(bool v, IMergeReadHandler* handler)
{
    merged_read = v;
    _handler = handler;
}
#endif

ISrsHttpResponseReader* SrsHttpMessage::body_reader()
{
    return _body;
}

int64_t SrsHttpMessage::content_length()
{
    return _header.content_length;
}

string SrsHttpMessage::query_get(string key)
{
    std::string v;

    if (_query.find(key) != _query.end()) {
        v = _query[key];
    }

    return v;
}

int SrsHttpMessage::request_header_count()
{
    return (int)_headers.size();
}

string SrsHttpMessage::request_header_key_at(int index)
{
    srs_assert(index < request_header_count());
    SrsHttpHeaderField item = _headers[index];
    return item.first;
}

string SrsHttpMessage::request_header_value_at(int index)
{
    srs_assert(index < request_header_count());
    SrsHttpHeaderField item = _headers[index];
    return item.second;
}

string SrsHttpMessage::get_request_header(string name)
{
    std::vector<SrsHttpHeaderField>::iterator it;

    for (it = _headers.begin(); it != _headers.end(); ++it) {
        SrsHttpHeaderField& elem = *it;
        std::string key = elem.first;
        std::string value = elem.second;
        if (key == name) {
            return value;
        }
    }

    return "";
}

SrsRequest* SrsHttpMessage::to_request(string vhost)
{
    SrsRequest* req = new SrsRequest();
    // add by chenyun 2018-04-05
    req->x_real_ip = get_request_header("X-Real-IP");
    //req->ip = conn->client_ip();
    req->app = _uri->get_path();
    string _param = _uri->get_query();
    size_t pos = string::npos;
    if ((pos = req->app.rfind("/")) != string::npos) {
        req->stream = req->app.substr(pos + 1);
        req->app = req->app.substr(0, pos);
    }
    if ((pos = req->stream.rfind(".")) != string::npos) {
        req->stream = req->stream.substr(0, pos);
    }

    req->tcUrl = "rtmp://" + vhost + req->app;
    req->pageUrl = get_request_header("Referer");
    req->objectEncoding = 0;

    srs_discovery_tc_url(req->tcUrl,
                         req->schema, req->host, req->vhost, req->app, req->port,
                         req->param);

    if (!_param.empty()) {
        if (req->param.empty()) {
            req->param = "?" + _param;
        } else {
            req->param = req->param + "&" + _param;
        }
    }

    req->strip();

    return req;
}

bool SrsHttpMessage::is_jsonp()
{
    return jsonp;
}

// get the status text of code.
string srs_generate_http_status_text(int status)
{
    static std::map<int, std::string> _status_map;
    if (_status_map.empty()) {
        _status_map[SRS_CONSTS_HTTP_Continue] = SRS_CONSTS_HTTP_Continue_str;
        _status_map[SRS_CONSTS_HTTP_SwitchingProtocols] = SRS_CONSTS_HTTP_SwitchingProtocols_str;
        _status_map[SRS_CONSTS_HTTP_OK] = SRS_CONSTS_HTTP_OK_str;
        _status_map[SRS_CONSTS_HTTP_Created] = SRS_CONSTS_HTTP_Created_str;
        _status_map[SRS_CONSTS_HTTP_Accepted] = SRS_CONSTS_HTTP_Accepted_str;
        _status_map[SRS_CONSTS_HTTP_NonAuthoritativeInformation] = SRS_CONSTS_HTTP_NonAuthoritativeInformation_str;
        _status_map[SRS_CONSTS_HTTP_NoContent] = SRS_CONSTS_HTTP_NoContent_str;
        _status_map[SRS_CONSTS_HTTP_ResetContent] = SRS_CONSTS_HTTP_ResetContent_str;
        _status_map[SRS_CONSTS_HTTP_PartialContent] = SRS_CONSTS_HTTP_PartialContent_str;
        _status_map[SRS_CONSTS_HTTP_MultipleChoices] = SRS_CONSTS_HTTP_MultipleChoices_str;
        _status_map[SRS_CONSTS_HTTP_MovedPermanently] = SRS_CONSTS_HTTP_MovedPermanently_str;
        _status_map[SRS_CONSTS_HTTP_Found] = SRS_CONSTS_HTTP_Found_str;
        _status_map[SRS_CONSTS_HTTP_SeeOther] = SRS_CONSTS_HTTP_SeeOther_str;
        _status_map[SRS_CONSTS_HTTP_NotModified] = SRS_CONSTS_HTTP_NotModified_str;
        _status_map[SRS_CONSTS_HTTP_UseProxy] = SRS_CONSTS_HTTP_UseProxy_str;
        _status_map[SRS_CONSTS_HTTP_TemporaryRedirect] = SRS_CONSTS_HTTP_TemporaryRedirect_str;
        _status_map[SRS_CONSTS_HTTP_BadRequest] = SRS_CONSTS_HTTP_BadRequest_str;
        _status_map[SRS_CONSTS_HTTP_Unauthorized] = SRS_CONSTS_HTTP_Unauthorized_str;
        _status_map[SRS_CONSTS_HTTP_PaymentRequired] = SRS_CONSTS_HTTP_PaymentRequired_str;
        _status_map[SRS_CONSTS_HTTP_Forbidden] = SRS_CONSTS_HTTP_Forbidden_str;
        _status_map[SRS_CONSTS_HTTP_NotFound] = SRS_CONSTS_HTTP_NotFound_str;
        _status_map[SRS_CONSTS_HTTP_MethodNotAllowed] = SRS_CONSTS_HTTP_MethodNotAllowed_str;
        _status_map[SRS_CONSTS_HTTP_NotAcceptable] = SRS_CONSTS_HTTP_NotAcceptable_str;
        _status_map[SRS_CONSTS_HTTP_ProxyAuthenticationRequired] = SRS_CONSTS_HTTP_ProxyAuthenticationRequired_str;
        _status_map[SRS_CONSTS_HTTP_RequestTimeout] = SRS_CONSTS_HTTP_RequestTimeout_str;
        _status_map[SRS_CONSTS_HTTP_Conflict] = SRS_CONSTS_HTTP_Conflict_str;
        _status_map[SRS_CONSTS_HTTP_Gone] = SRS_CONSTS_HTTP_Gone_str;
        _status_map[SRS_CONSTS_HTTP_LengthRequired] = SRS_CONSTS_HTTP_LengthRequired_str;
        _status_map[SRS_CONSTS_HTTP_PreconditionFailed] = SRS_CONSTS_HTTP_PreconditionFailed_str;
        _status_map[SRS_CONSTS_HTTP_RequestEntityTooLarge] = SRS_CONSTS_HTTP_RequestEntityTooLarge_str;
        _status_map[SRS_CONSTS_HTTP_RequestURITooLarge] = SRS_CONSTS_HTTP_RequestURITooLarge_str;
        _status_map[SRS_CONSTS_HTTP_UnsupportedMediaType] = SRS_CONSTS_HTTP_UnsupportedMediaType_str;
        _status_map[SRS_CONSTS_HTTP_RequestedRangeNotSatisfiable] = SRS_CONSTS_HTTP_RequestedRangeNotSatisfiable_str;
        _status_map[SRS_CONSTS_HTTP_ExpectationFailed] = SRS_CONSTS_HTTP_ExpectationFailed_str;
        _status_map[SRS_CONSTS_HTTP_InternalServerError] = SRS_CONSTS_HTTP_InternalServerError_str;
        _status_map[SRS_CONSTS_HTTP_NotImplemented] = SRS_CONSTS_HTTP_NotImplemented_str;
        _status_map[SRS_CONSTS_HTTP_BadGateway] = SRS_CONSTS_HTTP_BadGateway_str;
        _status_map[SRS_CONSTS_HTTP_ServiceUnavailable] = SRS_CONSTS_HTTP_ServiceUnavailable_str;
        _status_map[SRS_CONSTS_HTTP_GatewayTimeout] = SRS_CONSTS_HTTP_GatewayTimeout_str;
        _status_map[SRS_CONSTS_HTTP_HTTPVersionNotSupported] = SRS_CONSTS_HTTP_HTTPVersionNotSupported_str;
    }

    std::string status_text;
    if (_status_map.find(status) == _status_map.end()) {
        status_text = "Status Unknown";
    } else {
        status_text = _status_map[status];
    }

    return status_text;
}

// bodyAllowedForStatus reports whether a given response status code
// permits a body.  See RFC2616, section 4.4.
bool srs_go_http_body_allowd(int status)
{
    if (status >= 100 && status <= 199) {
        return false;
    } else if (status == 204 || status == 304) {
        return false;
    }

    return true;
}

// DetectContentType implements the algorithm described
// at http://mimesniff.spec.whatwg.org/ to determine the
// Content-Type of the given data.  It considers at most the
// first 512 bytes of data.  DetectContentType always returns
// a valid MIME type: if it cannot determine a more specific one, it
// returns "application/octet-stream".
string srs_go_http_detect(char* data, int size)
{
    // detect only when data specified.
    if (data) {
    }
    return "application/octet-stream"; // fallback
}

int srs_go_http_error(ISrsHttpResponseWriter* w, int code)
{
    return srs_go_http_error(w, code, srs_generate_http_status_text(code));
}

int srs_go_http_error(ISrsHttpResponseWriter* w, int code, string error)
{
    int ret = ERROR_SUCCESS;

    w->header()->set_content_type("text/plain; charset=utf-8");
    w->header()->set_content_length(error.length());
    w->write_header(code);
    w->write((char*)error.data(), (int)error.length());

    return ret;
}