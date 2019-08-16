#include <string>
#include "basic/file.hpp"
#include "container/flv.hpp"
#include "protocol/rtmp.hpp"
#include "protocol/http.hpp"
using namespace std;

class SrsProtocol;
class SrsCommonMessage;
class SrsPacket;
class SrsBuffer;


class SrsHDLProtocol {
private:
    SrsHttpMessage *msg;
    SrsHttpResponseReader* _body;
    int stream_id;
    ISrsProtocolReaderWriter *skt;
    SrsFastBuffer* in_buffer;
    SrsProtocol *protocol;
    SrsBuffer* tag_stream;
    char tag_header[SRS_FLV_TAG_HEADER_SIZE];
public:
    SrsHDLProtocol(ISrsProtocolReaderWriter *io);
    virtual ~SrsHDLProtocol();
public:
    virtual void set_msg(SrsHttpMessage * msg);
    /**
    * set/get the recv timeout in us.
    * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
    */
    virtual void set_recv_timeout(int64_t timeout_us);
    virtual int64_t get_recv_timeout();
    /**
    * set/get the send timeout in us.
    * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
    */
    virtual void set_send_timeout(int64_t timeout_us);
    virtual int64_t get_send_timeout();
    /**
    * get recv/send bytes.
    */
    virtual int64_t get_recv_bytes();
    virtual int64_t get_send_bytes();
    virtual int recv_flv_header();
    virtual int write_flv_header(char* header);
    virtual int read_header(char* header);
    virtual int read_tag_header(char* ptype, int32_t* pdata_size, u_int32_t* ptime);
    virtual int read_tag_data(char* data, int32_t size);
    virtual int read_previous_tag_size(uint32_t* tag_size);
    virtual int decode_message(SrsCommonMessage* msg, SrsPacket** ppacket);
    virtual int write_metadata_to_cache(char type, char* data, int size, char* cache);
    virtual int write_audio_to_cache(int64_t timestamp, char* data, int size, char* cache);
    virtual int write_video_to_cache(int64_t timestamp, char* data, int size, char* cache);
    virtual int write_pts_to_cache(int size, char* cache);
    virtual int write_metadata(char type, char* data, int size);
    virtual int write_audio(int64_t timestamp, char* data, int size);
    virtual int write_video(int64_t timestamp, char* data, int size);
    virtual int write_tag(char* header, int header_size, char* tag, int tag_size);
    virtual int write_flv_tag(char type, int32_t time, char* data, int size);
public:
    /**
    * recv a RTMP message, which is bytes oriented.
    * user can use decode_message to get the decoded RTMP packet.
    * @param pmsg, set the received message,
    *       always NULL if error,
    *       NULL for unknown packet but return success.
    *       never NULL if decode success.
    * @remark, drop message when msg is empty or payload length is empty.
    */
    virtual int recv_message(SrsCommonMessage** pmsg);
    virtual int do_recv_message(SrsCommonMessage** pmsg);
};

class SrsHDLClient
{
private:
    // st_netfd_t stfd;
    ISrsProtocolReaderWriter* io;
    int64_t timeout_us;
    // host name or ip.
    std::string host;
    int port;
    int stream_id;
public:
    SrsHttpParser* parser;
protected:
    SrsHDLProtocol* protocol;
public:
    SrsHDLClient(ISrsProtocolReaderWriter *io);
    virtual ~SrsHDLClient();
public:
    virtual int connect_server(std::string server, int port);
    virtual void set_msg(SrsHttpMessage * msg);
    /**
     * set the recv timeout in us.
     * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
     */
    virtual void set_recv_timeout(int64_t timeout_us);
    /**
     * set the send timeout in us.
     * if timeout, recv/send message return ERROR_SOCKET_TIMEOUT.
     */
    virtual void set_send_timeout(int64_t timeout_us);
    /**
     * get recv/send bytes.
     */
    virtual int64_t get_recv_bytes();
    virtual int64_t get_send_bytes();
    /**
     * recv a RTMP message, which is bytes oriented.
     * user can use decode_message to get the decoded RTMP packet.
     * @param pmsg, set the received message,
     *       always NULL if error,
     *       NULL for unknown packet but return success.
     *       never NULL if decode success.
     * @remark, drop message when msg is empty or payload length is empty.
     */
    virtual int recv_message(SrsCommonMessage** pmsg);
    virtual int recv_flv_header();
    virtual int send_flv_header();
    virtual int decode_message(SrsCommonMessage* msg, SrsPacket** ppacket);
    virtual int play(std::string url);
    virtual int publish(std::string url);
    virtual int write_flv_tag(char type, int32_t time, char* data, int size);
    virtual int write_flv_header(char* flv) ;
    virtual int srs_connect(std::string server, int port);
};

void srs_hdl_destroy(void* io);
void* srs_hdl_create();
int srs_hdl_connect(void* io, string server, int port);
int srs_hdl_publish(void* io, string url);
int srs_hdl_write_header(void* io, char flv_header[9]);
int srs_hdl_write_tag(void* io, char type, int32_t time, char* data, int size);