#include <sstream>
#include "logs/log.hpp"
#include "hdl.hpp"
#include "basic/buffer.hpp"
#include "rtmp_tool.h"

SrsHDLProtocol::SrsHDLProtocol(ISrsProtocolReaderWriter *io)
{
    skt = io;
    stream_id = 0;
    msg = NULL;
    _body = NULL;
    tag_stream = new SrsBuffer();
    protocol = new SrsProtocol(io);
    in_buffer = new SrsFastBuffer();
}

SrsHDLProtocol::~SrsHDLProtocol()
{
    srs_freep(msg);
    srs_freep(in_buffer);
    srs_freep(protocol);
    srs_freep(tag_stream);
}

void SrsHDLProtocol::set_msg(SrsHttpMessage * _msg)
{
    srs_freep(msg);
    msg = _msg;
    _body = msg->get_http_response_reader();
}

void SrsHDLProtocol::set_recv_timeout(int64_t timeout_us)
{
    return skt->set_recv_timeout(timeout_us);
}

int64_t SrsHDLProtocol::get_recv_timeout()
{
    return skt->get_recv_timeout();
}

void SrsHDLProtocol::set_send_timeout(int64_t timeout_us)
{
    return skt->set_send_timeout(timeout_us);
}

int64_t SrsHDLProtocol::get_send_timeout()
{
    return skt->get_send_timeout();
}

int64_t SrsHDLProtocol::get_recv_bytes()
{
    return skt->get_recv_bytes();
}

int64_t SrsHDLProtocol::get_send_bytes()
{
    return skt->get_send_bytes();
}
int SrsHDLProtocol::write_metadata_to_cache(char type, char* data, int size, char* cache)
{
    int ret = ERROR_SUCCESS;

    srs_assert(data);

    // 11 bytes tag header
    /*char tag_header[] = {
     (char)type, // TagType UB [5], 18 = script data
     (char)0x00, (char)0x00, (char)0x00, // DataSize UI24 Length of the message.
     (char)0x00, (char)0x00, (char)0x00, // Timestamp UI24 Time in milliseconds at which the data in this tag applies.
     (char)0x00, // TimestampExtended UI8
     (char)0x00, (char)0x00, (char)0x00, // StreamID UI24 Always 0.
     };*/

    // write data size.
    if ((ret = tag_stream->initialize(cache, 11)) != ERROR_SUCCESS) {
        return ret;
    }
    tag_stream->write_1bytes(type);
    tag_stream->write_3bytes(size);
    tag_stream->write_3bytes(0x00);
    tag_stream->write_1bytes(0x00);
    tag_stream->write_3bytes(0x00);

    return ret;
}

int SrsHDLProtocol::write_audio_to_cache(int64_t timestamp, char* data, int size, char* cache)
{
    int ret = ERROR_SUCCESS;

    srs_assert(data);

    timestamp &= 0x7fffffff;

    // 11bytes tag header
    /*char tag_header[] = {
     (char)SrsFrameTypeAudio, // TagType UB [5], 8 = audio
     (char)0x00, (char)0x00, (char)0x00, // DataSize UI24 Length of the message.
     (char)0x00, (char)0x00, (char)0x00, // Timestamp UI24 Time in milliseconds at which the data in this tag applies.
     (char)0x00, // TimestampExtended UI8
     (char)0x00, (char)0x00, (char)0x00, // StreamID UI24 Always 0.
     };*/

    // write data size.
    if ((ret = tag_stream->initialize(cache, 11)) != ERROR_SUCCESS) {
        return ret;
    }
    tag_stream->write_1bytes(SrsFrameTypeAudio);
    tag_stream->write_3bytes(size);
    tag_stream->write_3bytes((int32_t)timestamp);
    // default to little-endian
    tag_stream->write_1bytes((timestamp >> 24) & 0xFF);
    tag_stream->write_3bytes(0x00);

    return ret;
}

int SrsHDLProtocol::write_video_to_cache(int64_t timestamp, char* data, int size, char* cache)
{
    int ret = ERROR_SUCCESS;

    srs_assert(data);

    timestamp &= 0x7fffffff;

    // 11bytes tag header
    /*char tag_header[] = {
     (char)SrsFrameTypeVideo, // TagType UB [5], 9 = video
     (char)0x00, (char)0x00, (char)0x00, // DataSize UI24 Length of the message.
     (char)0x00, (char)0x00, (char)0x00, // Timestamp UI24 Time in milliseconds at which the data in this tag applies.
     (char)0x00, // TimestampExtended UI8
     (char)0x00, (char)0x00, (char)0x00, // StreamID UI24 Always 0.
     };*/

    // write data size.
    if ((ret = tag_stream->initialize(cache, 11)) != ERROR_SUCCESS) {
        return ret;
    }
    tag_stream->write_1bytes(SrsFrameTypeVideo);
    tag_stream->write_3bytes(size);
    tag_stream->write_3bytes((int32_t)timestamp);
    // default to little-endian
    tag_stream->write_1bytes((timestamp >> 24) & 0xFF);
    tag_stream->write_3bytes(0x00);

    return ret;
}

int SrsHDLProtocol::write_pts_to_cache(int size, char* cache)
{
    int ret = ERROR_SUCCESS;

    if ((ret = tag_stream->initialize(cache, SRS_FLV_PREVIOUS_TAG_SIZE)) != ERROR_SUCCESS) {
        return ret;
    }
    tag_stream->write_4bytes(size);

    return ret;
}

int SrsHDLProtocol::write_metadata(char type, char* data, int size)
{
    int ret = ERROR_SUCCESS;

    srs_assert(data);

    if ((ret = write_metadata_to_cache(type, data, size, tag_header)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = write_tag(tag_header, sizeof(tag_header), data, size)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("write flv data tag failed. ret=%d", ret);
        }
        return ret;
    }

    return ret;
}

int SrsHDLProtocol::write_audio(int64_t timestamp, char* data, int size)
{
    int ret = ERROR_SUCCESS;

    srs_assert(data);

    if ((ret = write_audio_to_cache(timestamp, data, size, tag_header)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = write_tag(tag_header, sizeof(tag_header), data, size)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("write flv audio tag failed. ret=%d", ret);
        }
        return ret;
    }

    return ret;
}

int SrsHDLProtocol::write_video(int64_t timestamp, char* data, int size)
{
    int ret = ERROR_SUCCESS;

    srs_assert(data);

    if ((ret = write_video_to_cache(timestamp, data, size, tag_header)) != ERROR_SUCCESS) {
        return ret;
    }

    if ((ret = write_tag(tag_header, sizeof(tag_header), data, size)) != ERROR_SUCCESS) {
        srs_error("write flv video tag failed. ret=%d", ret);
        return ret;
    }

    return ret;
}

int SrsHDLProtocol::write_tag(char* header, int header_size, char* tag, int tag_size)
{
    int ret = ERROR_SUCCESS;

    // PreviousTagSizeN UI32 Size of last tag, including its header, in bytes.
    char pre_size[SRS_FLV_PREVIOUS_TAG_SIZE];
    if ((ret = write_pts_to_cache(tag_size + header_size, pre_size)) != ERROR_SUCCESS) {
        return ret;
    }

    iovec iovs[3];
    iovs[0].iov_base = header;
    iovs[0].iov_len = header_size;
    iovs[1].iov_base = tag;
    iovs[1].iov_len = tag_size;
    iovs[2].iov_base = pre_size;
    iovs[2].iov_len = SRS_FLV_PREVIOUS_TAG_SIZE;

    if ((ret = skt->writev(iovs, 3, NULL)) != ERROR_SUCCESS) {
        if (!srs_is_client_gracefully_close(ret)) {
            srs_error("write flv tag failed. ret=%d", ret);
        }
        return ret;
    }

    return ret;
}

int SrsHDLProtocol::write_flv_tag(char type, int32_t time, char* data, int size)
{
    int ret = ERROR_SUCCESS;


    if (type == SRS_RTMP_TYPE_AUDIO) {
        return write_audio(time, data, size);
    } else if (type == SRS_RTMP_TYPE_VIDEO) {
        return write_video(time, data, size);
    } else {
        return write_metadata(type, data, size);
    }

    return ret;
}

int SrsHDLProtocol::write_flv_header(char flv_header[9]) {
    int ret = ERROR_SUCCESS;
    if ((ret = skt->write(flv_header, 9, NULL)) != ERROR_SUCCESS) {
        srs_error("write flv header failed. ret=%d", ret);
        return ret;
    }

    // previous tag size.
    char pts[] = { (char)0x00, (char)0x00, (char)0x00, (char)0x00 };
    if ((ret = skt->write(pts, 4, NULL)) != ERROR_SUCCESS) {
        return ret;
    }
    return ret;
}

int SrsHDLProtocol::recv_flv_header() {
    int ret = ERROR_SUCCESS;
    char header[9];
    uint32_t tsize;
    if ((ret = read_header(header)) != ERROR_SUCCESS) {
        return ret;
    };
    return read_previous_tag_size(&tsize);
}

int SrsHDLProtocol::read_header(char header[9])
{
    int ret = ERROR_SUCCESS;
    int nsize = 0;

    srs_assert(header);
    if ((ret = _body->read_full(header, 9, &nsize)) != ERROR_SUCCESS || nsize != 9) {
        if (ret != ERROR_SOCKET_TIMEOUT && !srs_is_client_gracefully_close(ret)) {
            if (ret == ERROR_SUCCESS) {
                ret = ERROR_SYSTEM_IO_INVALID;
            }
            srs_error("read 9bytes flv header failed. required_size=%d, return size=%d, ret=%d", 9, nsize, ret);
        }
        return ret;
    }

    char* h = header;
    if (h[0] != 'F' || h[1] != 'L' || h[2] != 'V') {
        ret = ERROR_KERNEL_FLV_HEADER;
        srs_warn("flv header must start with FLV. ret=%d", ret);
        return ret;
    }

    return ret;
}

int SrsHDLProtocol::read_tag_header(char* ptype, int32_t* pdata_size, u_int32_t* ptime)
{
    int ret = ERROR_SUCCESS;
    int nsize = 0;

    srs_assert(ptype);
    srs_assert(pdata_size);
    srs_assert(ptime);

    char th[11]; // tag header

    // read tag header
    if ((ret = _body->read_full(th, 11, &nsize)) != ERROR_SUCCESS || nsize != 11) {
        if (ret != ERROR_SOCKET_TIMEOUT && !srs_is_client_gracefully_close(ret)) {
            if (ret == ERROR_SUCCESS) {
                ret = ERROR_SYSTEM_IO_INVALID;
            }
            srs_error("read 11bytes tag header failed. required_size=%d, return size=%d, ret=%d", 9, nsize, ret);
        }
        return ret;
    }

    // Reserved UB [2]
    // Filter UB [1]
    // TagType UB [5]
    *ptype = (th[0] & 0x1F);

    // DataSize UI24
    char* pp = (char*)pdata_size;
    pp[3] = 0;
    pp[2] = th[1];
    pp[1] = th[2];
    pp[0] = th[3];

    // Timestamp UI24
    pp = (char*)ptime;
    pp[2] = th[4];
    pp[1] = th[5];
    pp[0] = th[6];

    // TimestampExtended UI8
    pp[3] = th[7];
    return ret;
}

int SrsHDLProtocol::read_tag_data(char* data, int32_t size)
{
    int ret = ERROR_SUCCESS;
    int nsize = 0;

    srs_assert(data);
    if ((ret = _body->read_full(data, size, &nsize)) != ERROR_SUCCESS || nsize != size) {
        if (ret != ERROR_SOCKET_TIMEOUT && !srs_is_client_gracefully_close(ret)) {
            if (ret == ERROR_SUCCESS) {
                ret = ERROR_SYSTEM_IO_INVALID;
            }
            srs_error("read %dbytes tag data failed. required_size=%d, return size=%d, ret=%d", size, nsize, ret);
        }
        return ret;
    }

    return ret;
}

int SrsHDLProtocol::read_previous_tag_size(uint32_t *tag_size)
{
    int ret = ERROR_SUCCESS;
    int nsize = 0;

    srs_assert(tag_size);
    char previous_tag_size[4];
    if ((ret = _body->read_full(previous_tag_size, 4, &nsize)) != ERROR_SUCCESS || nsize != 4) {
        if (ret != ERROR_SOCKET_TIMEOUT && !srs_is_client_gracefully_close(ret)) {
            if (ret == ERROR_SUCCESS) {
                ret = ERROR_SYSTEM_IO_INVALID;
            }
            srs_error("read 4bytes previous tag size failed. required_size=%d, return size=%d, ret=%d", 4, nsize, ret);
        }
        return ret;
    }

    char *pp = (char *)tag_size;
    pp[3] = previous_tag_size[0];
    pp[2] = previous_tag_size[1];
    pp[1] = previous_tag_size[2];
    pp[0] = previous_tag_size[3];

    return ret;
}

int SrsHDLProtocol::recv_message(SrsCommonMessage** pmsg)
{
    *pmsg = NULL;

    int ret = ERROR_SUCCESS;

    while (true) {
        SrsCommonMessage* msg = NULL;
        if ((ret = do_recv_message(&msg)) != ERROR_SUCCESS) {
            if (ret != ERROR_SOCKET_TIMEOUT && !srs_is_client_gracefully_close(ret)) {
                srs_error("recv interlaced message failed. ret=%d", ret);
            }
            srs_freep(msg);
            return ret;
        }
        srs_verbose("entire msg received");

        if (msg->size <= 0 || msg->header.payload_length <= 0) {
            srs_trace("ignore empty message(type=%d, size=%d, time=%"PRId64", sid=%d).",
                msg->header.message_type, msg->header.payload_length,
                msg->header.timestamp, msg->header.stream_id);
            srs_freep(msg);
            continue;
        }

        srs_verbose("got a msg, cid=%d, type=%d, size=%d, time=%"PRId64,
            msg->header.perfer_cid, msg->header.message_type, msg->header.payload_length,
            msg->header.timestamp);
        *pmsg = msg;
        break;
    }

    return ret;
}

int SrsHDLProtocol::do_recv_message(SrsCommonMessage** pmsg)
{
    int ret = ERROR_SUCCESS;
    char type;
    int32_t size;
    uint32_t ts;
    char *data = NULL;

    if ((ret = read_tag_header(&type, &size, &ts)) != ERROR_SUCCESS) {
        srs_error("read tag header failed. ret=%d", ret);
        return ret;
    }
    data = new char[size];
    if ((ret = read_tag_data(data, size)) != ERROR_SUCCESS) {
        srs_error("read tag data failed. ret=%d", ret);
        return ret;
    }

    // char tag_size[4]; // tag size
    uint32_t tag_size;
    if ((ret = read_previous_tag_size(&tag_size)) != ERROR_SUCCESS) {
        srs_error("read previous tag size failed. ret=%d", ret);
        return ret;
    }

    if (tag_size != (uint32_t)size + 11) { // if not equal, output error message, but ignore
        srs_error("size %d not equal previous tag size %d", size+11, tag_size);
    }


    SrsCommonMessage* _msg = new SrsCommonMessage();
    if (type == SrsCodecFlvTagAudio) {
        _msg->header.initialize_audio(size, ts, stream_id);
    } else if (type == SrsCodecFlvTagVideo) {
        _msg->header.initialize_video(size, ts, stream_id);
    } else if (type == SrsCodecFlvTagScript) {
        _msg->header.initialize_amf0_script(size, stream_id);
    } else {
        srs_warn("invalid type %d.", type);
        ret = ERROR_STREAM_CASTER_FLV_TAG;
        return ret;
    }
    _msg->payload = data;
    _msg->size = size;

    *pmsg = _msg;
    return ret;
}

int SrsHDLProtocol::decode_message(SrsCommonMessage* msg, SrsPacket** ppacket)
{
    return protocol->decode_message(msg, ppacket);
}

SrsHDLClient::SrsHDLClient(ISrsProtocolReaderWriter *rw)
{
    io = rw;
    stream_id = 0;

    protocol = new SrsHDLProtocol(io);
    parser = new SrsHttpParser();
    parser->initialize(HTTP_RESPONSE);
}

SrsHDLClient::~SrsHDLClient()
{
    srs_freep(protocol);
    srs_freep(parser);
    srs_freep(io);
}

int SrsHDLClient::connect_server(std::string server, int port)
{
    int ret = ERROR_SUCCESS;

    SimpleSocketStream* skt = new SimpleSocketStream();
    ret = skt->create_socket(NULL);
    if (ret != ERROR_SUCCESS) {
        return ret;
    }
    ret = skt->connect(server.c_str(), port);
    if (ret != ERROR_SUCCESS) {
        return ret;
    }
    io = skt;
    return ret;
}

void SrsHDLClient::set_msg(SrsHttpMessage* msg)
{
    protocol->set_msg(msg);
}
void SrsHDLClient::set_recv_timeout(int64_t timeout_us)
{
    protocol->set_recv_timeout(timeout_us);
}

void SrsHDLClient::set_send_timeout(int64_t timeout_us)
{
    protocol->set_send_timeout(timeout_us);
}

int64_t SrsHDLClient::get_recv_bytes()
{
    return protocol->get_recv_bytes();
}

int64_t SrsHDLClient::get_send_bytes()
{
    return protocol->get_send_bytes();
}

int SrsHDLClient::recv_flv_header()
{
    return protocol->recv_flv_header();
}

int SrsHDLClient::send_flv_header()
{
    //return protocol->write_flv_header();
    return -1;
}
int SrsHDLClient::recv_message(SrsCommonMessage** pmsg)
{
    // need convert flv msg to srs common message
    return protocol->recv_message(pmsg);
}

int SrsHDLClient::decode_message(SrsCommonMessage* msg, SrsPacket** ppacket)
{
    return protocol->decode_message(msg, ppacket);
}

int SrsHDLClient::write_flv_tag(char type, int32_t time, char* data, int size)
{
    return protocol->write_flv_tag(type, time, data, size);
}

int SrsHDLClient::write_flv_header(char flv_header[9])
{
    return protocol->write_flv_header(flv_header);
}

int SrsHDLClient::play(string url)
{
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("do_post_and_get_data. uri initialize failed. url=%s, ret=%d", url.c_str(), ret);
        return ret;
    }

    std::stringstream ss;
    ss << "GET " << uri.get_raw_path() << " "
        << "HTTP/1.1" << SRS_HTTP_CRLF
        << "Host: " << uri.get_host() << SRS_HTTP_CRLF
        << "Connection: Keep-Alive" << SRS_HTTP_CRLF
        << "User-Agent: " << "JDlive" << SRS_HTTP_CRLF
        << SRS_HTTP_CRLF;

    std::string data = ss.str();
    if ((ret = io->write((void*)data.c_str(), data.length(), NULL)) != ERROR_SUCCESS) {
        // disconnect when error.
        srs_error("write http get failed. ret=%d", ret);
        return ret;
    }

    ISrsHttpMessage* msg = NULL;
    SimpleSocketStream* sio = static_cast<SimpleSocketStream*>(io);
    srs_assert(sio);
    if ((ret = parser->parse_message(sio, &msg)) != ERROR_SUCCESS) {
        srs_error("parse http post response failed. ret=%d", ret);
        return ret;
    }
    srs_assert(msg);
    SrsHttpMessage* _msg = static_cast<SrsHttpMessage *>(msg);
    srs_assert(_msg);
    _msg->enter_infinite_chunked();
    set_msg(_msg);
    srs_info("parse http get response success.");

    int code = _msg->status_code();
    if (code != ERROR_HTTP_OK) {
        srs_error("hdl get err, code is %d", code);
        return ERROR_HTTP_SERVICE_NOT_AVAILABLE;
    }

    return ret;
}

int SrsHDLClient::publish(string url)
{
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_error("do_post_and_get_data. uri initialize failed. url=%s, ret=%d", url.c_str(), ret);
        return ret;
    }

    std::stringstream ss;
    ss << "POST " << uri.get_raw_path() << " "
        << "HTTP/1.1" << SRS_HTTP_CRLF
        << "Host: " << uri.get_host() << SRS_HTTP_CRLF
        << "Query: " << uri.get_query() << SRS_HTTP_CRLF
        << "Connection: Keep-Alive" << SRS_HTTP_CRLF
        << "User-Agent: " << "JDlive" << SRS_HTTP_CRLF
        << SRS_HTTP_CRLF;

    std::string data = ss.str();
    if ((ret = io->write((void*)data.c_str(), data.length(), NULL)) != ERROR_SUCCESS) {
        // disconnect when error.
        srs_error("write http post failed. ret=%d", ret);
        return ret;
    }

    ISrsHttpMessage* msg = NULL;
    SimpleSocketStream* sio = static_cast<SimpleSocketStream*>(io);
    srs_assert(sio);
    if ((ret = parser->parse_message(sio, &msg)) != ERROR_SUCCESS) {
        srs_error("parse http post response failed. ret=%d", ret);
        return ret;
    }
    srs_assert(msg);
    SrsHttpMessage* _msg = static_cast<SrsHttpMessage *>(msg);
    srs_assert(_msg);
    _msg->enter_infinite_chunked();
    set_msg(_msg);
    srs_info("parse http post response success.");

    int code = _msg->status_code();
    if (code != ERROR_HTTP_OK) {
        srs_error("hdl post err, code is %d", code);
        return ERROR_HTTP_SERVICE_NOT_AVAILABLE;
    }

    return ret;
}

int SrsHDLClient::srs_connect(std::string server, int port)
{
    int ret = ERROR_SUCCESS;

    SimpleSocketStream* skt = static_cast<SimpleSocketStream*>(io);
    srs_assert(skt);
    ret = skt->connect(server.c_str(), port);
    return ret;
}


void srs_hdl_destroy(void* io)
{
    if (!io) {
       return;
    }

    SrsHDLClient* client = static_cast<SrsHDLClient*>(io);
    srs_freep(client);
    return;
}

void* srs_hdl_create()
{
    SimpleSocketStream* skt = new SimpleSocketStream();
    skt->create_socket(NULL);
    skt->set_recv_timeout(5000);
    skt->set_send_timeout(5000);
    SrsHDLClient* client = new SrsHDLClient(skt);
    return (void*)client;
}


int srs_hdl_connect(void* io, std::string server, int port)
{
    int ret = ERROR_SUCCESS;
    SrsHDLClient* client = static_cast<SrsHDLClient*>(io);
    srs_assert(client);
    ret = client->srs_connect(server, port);
    return ret;
}

int srs_hdl_publish(void* io, std::string url)
{
    int ret = ERROR_SUCCESS;
    SrsHDLClient* client = static_cast<SrsHDLClient*>(io);
    srs_assert(client);
    ret = client->publish(url);
    return ret;
}

int srs_hdl_write_header(void* io, char flv_header[9])
{
    int ret = ERROR_SUCCESS;
    SrsHDLClient* client = static_cast<SrsHDLClient*>(io);
    srs_assert(client);
    ret = client->write_flv_header(flv_header);
    return ret;
}

int srs_hdl_write_tag(void* io, char type, int32_t time, char* data, int size)
{
    int ret = ERROR_SUCCESS;
    SrsHDLClient* client = static_cast<SrsHDLClient*>(io);
    srs_assert(client);
    ret = client->write_flv_tag(type, time, data, size);
    return ret;
}