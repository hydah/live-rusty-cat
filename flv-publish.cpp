/*
The MIT License (MIT)

Copyright (c) 2013-2015 SRS(ossrs)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/**
gcc srs_ingest_flv.c ../../objs/lib/srs_librtmp.a -g -O0 -lstdc++ -o srs_ingest_flv
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string>
#include "librtmp/srs_librtmp.h"


using namespace std;



int proxy(srs_flv_t flv, void* client, int recur);
int do_proxy(srs_flv_t flv, void* client, int64_t base, int64_t re, int32_t* pstarttime, u_int32_t* ptimestamp);

int connect_oc(srs_rtmp_t ortmp);

#define RE_PULSE_MS 300
#define RE_PULSE_JITTER_MS 3000
int64_t re_create();
void re_update(int64_t re, int32_t starttime, u_int32_t time);
void re_cleanup(int64_t re, int32_t starttime, u_int32_t time);

int64_t tools_main_entrance_startup_time;

static const char *optString = "i:y:f:r:";

void print_header()
{
    stringstream header;
    header << "ingest flv file and publish to RTMP server like FFMPEG."<< endl;
    header << "srs(ossrs) client librtmp library." << endl;
    header << "version: " << srs_version_major() << "."  << srs_version_minor() << ".";
    header << srs_version_revision() << "." << endl;
    header << "______________________";
    cout << header.str() << endl;
}

void print_usage(char *name)
{
    if (name == NULL) {
        char b[] = "null";
        name = b;
    }
    printf("ingest flv file and publish to RTMP server\n"
           "Usage: %s <-i in_flv_file>|<-f input_file_list> <-y out_rtmp_url> <-r>\n"
           "   in_flv_file     input flv file, ingest from this file.\n"
           "   out_flv_url    output flv url, publish to this url.\n"
           "For example:\n"
           "   %s -i doc/source.200kbps.768x320.flv -y http://127.0.0.1/live/livestream.flv\n"
           "   %s -i ../../doc/source.200kbps.768x320.flv -y http://127.0.0.1/live/livestream.flv\n",
           name, name, name);
}

void parse_flag(int argc, char** argv, string &input_file, string &concat_txt, string &tc_url, int &recur)
{
    if (argc <= 2) {
        print_usage(argv[0]);
        exit(-1);
    }

    int opt;
    recur = 1;
    while ((opt = getopt(argc, argv, optString)) != -1) {
        switch(opt) {
            case 'i':
                input_file = optarg;
                break;
            case 'y':
                tc_url = optarg;
                break;
            case 'f':
                concat_txt = optarg;
                break;
            case 'r':
                recur = atoi(optarg);
                break;
            case '?':
                print_usage(argv[0]);
                exit(-1);
        }
    }

    return;
}
/*
int conect_server(string server, int port, int64_t timeout)
{
    int stfd = NULL;
    int ret = 0;
    sockaddr_in addr;
    stfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(stfd == -1){
          ret = ERROR_SOCKET_CREATE;
          srs_error("[FATAL_SOCKET_CREATE]create socket error. ret=%d", ret);
          return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr=inet_addr(server);        

    if(connect(stfd, (void *)&addr,sizeof(addr)) == -1){
        ret = ERROR_ST_OPEN_SOCKET;
        srs_error("[FATAL_SOCKET_CONNECT]connect socket failed. ret=%d", ret);
        ::close(stfd);
        return -1;
    }
    //srs_socket_connect(server, port, timeout, &stfd, do_bind, is_https)
    return stfd;
}*/
int  parse_url(string tc_url, string& server, int& port)
{
    int ret = 0;
    port = 80;
    size_t pos = tc_url.find("http");
    if (pos == string::npos) {
        srs_human_trace("output url should only be http start.");
        ret = -1;
        return ret;
    }
    pos = tc_url.find("://");
    if (pos != string::npos) {
        tc_url = tc_url.substr(pos+3);
        pos = tc_url.find("/");
        if (pos != string::npos) {
            tc_url = tc_url.substr(0, pos);
            server = tc_url;
            pos = tc_url.find(":");
            if (pos != string::npos) {
                server = tc_url.substr(0, pos);
                string ps = tc_url.substr(pos+1);
                port = atoi(ps.c_str());
            }
            return ret;
        } 
    }
    ret = -1;
    return ret;
}
/*
int publish(SimpleSocketStream* skt, string url)
{
    int ret = ERROR_SUCCESS;

    SrsHttpUri uri;
    if ((ret = uri.initialize(url)) != ERROR_SUCCESS) {
        srs_human_trace("do_post_and_get_data. uri initialize failed. url=%s, ret=%d", url.c_str(), ret);
        return ret;
    }

    std::stringstream ss;
    ss << "POST " << uri.get_raw_path() << " "
        << "HTTP/1.1" << SRS_HTTP_CRLF
        << "Host: " << uri.get_host() << SRS_HTTP_CRLF
        << "Connection: Keep-Alive" << SRS_HTTP_CRLF
        << "User-Agent: " << "JDlive" << SRS_HTTP_CRLF
        << SRS_HTTP_CRLF;

    std::string data = ss.str();
    if ((ret = skt->write((void*)data.c_str(), data.length(), NULL)) != ERROR_SUCCESS) {
        // disconnect when error.
        srs_human_trace("write http post failed. ret=%d", ret);
        return ret;
    }

    ISrsHttpMessage* msg = NULL;
   // skt->read_fully(void * buf, size_t size, ssize_t * nread);
    if ((ret = parser->parse_message(skt, NULL, &msg)) != ERROR_SUCCESS) {
        srs_human_trace("parse http post response failed. ret=%d", ret);
        return ret;
    }
    srs_assert(msg);
    SrsHttpMessage* _msg = static_cast<SrsHttpMessage *>(msg);
    srs_assert(_msg);
    _msg->enter_infinite_chunked();
    set_msg(_msg);
    srs_human_trace("parse http post response success.");

    int code = _msg->status_code();
    if (code != ERROR_HTTP_OK) {
        srs_human_trace("hdl post err, code is %d", code);
        return ERROR_HTTP_SERVICE_NOT_AVAILABLE;
    }

    return ret;
}*/
int main(int argc, char** argv)
{
    print_header();
    int ret = 0;
    srs_flv_t flv;
    string input_file, url, concat_txt, server = "";
    int port = 80;
    int recur;

    tools_main_entrance_startup_time = srs_utils_time_ms();
    parse_flag(argc, argv, input_file, concat_txt, url, recur);
    if (input_file.empty() && concat_txt.empty()) {
        srs_human_trace("input invalid, use -i <input> or -f <concat_txt>");
        return -1;
    }
    if (url.empty()) {
        srs_human_trace("output invalid, use -y <output>");
        return -1;
    }
    ret = parse_url(url, server, port);
    if (ret != 0) {
        srs_human_trace("parse url: %s error.", url.c_str());
        return ret;
    }
    if (!concat_txt.empty()) {
        srs_human_trace("concat_txt: %s", concat_txt.c_str());
    } else {
        srs_human_trace("input:  %s", input_file.c_str());
    }
    srs_human_trace("output: %s", url.c_str());
    srs_human_trace("recur: %d", recur);
    void* client = srs_hdl_create();
    string ip = srs_dns_resolve(server);
    ret = srs_hdl_connect(client, ip, port);
    if (ret != 0) {
        srs_human_trace("client connect error, ret = %d.", ret);
        srs_hdl_destroy(client);
        return ret;
    }
    ret = srs_hdl_publish(client, url);
    if (ret != 0) {
        srs_human_trace("client publish error, ret = %d.", ret);
        srs_hdl_destroy(client);
        return ret;
    }

    if (!concat_txt.empty()) {
        int ret = 0;
        u_int32_t timestamp = 0;
        int32_t starttime = -1;
        char header[13];

        int64_t base_time = srs_utils_time_ms();
        int64_t base = 0;
        do {
            ifstream fin(concat_txt.c_str(), ios::in);
            string f;
            while(getline(fin, f)) {
                if ((flv = srs_flv_open_read(f.c_str())) == NULL) {
                    ret = 2;
                    srs_human_trace("open flv file %s failed. ret=%d", f.c_str(), ret);
                    break;
                }

                timestamp = 0;
                starttime = -1;
                base = srs_utils_time_ms() - base_time;
                tools_main_entrance_startup_time = srs_utils_time_ms();
                if ((ret = srs_flv_read_header(flv, header)) != 0) {
                    srs_human_trace("main read flv header failed. ret = %d.", ret);
                    srs_flv_close(flv);
                    break;
                }
                ret = srs_hdl_write_header(client, header);
                if (ret != 0) {
                    srs_human_trace("main write flv header failed. ret = %d.", ret);
                    srs_flv_close(flv);
                    break;
                }
                int64_t re = re_create();
                ret = do_proxy(flv, client, base, re, &starttime, &timestamp);
                if (ret != 0) {
                    srs_human_trace("do proxy failed. ret = %d.", ret);
                    srs_flv_close(flv);
                    break;
                }

                // for the last pulse, always sleep.
                re_cleanup(re, starttime, timestamp);
                srs_flv_close(flv);
           }
           recur--;
           fin.close();
         } while(recur > 0 && ret == 0);
    } else {

        if ((flv = srs_flv_open_read(input_file.c_str())) == NULL) {
            ret = 2;
            srs_hdl_destroy(client);
            srs_human_trace("open flv file failed. ret=%d", ret);
            return ret;
        }

        ret = proxy(flv, client, recur);
        srs_human_trace("ingest flv to flv completed");
        srs_flv_close(flv);   
    }
    
    srs_hdl_destroy(client);
    return ret;
}

int do_proxy(srs_flv_t flv, void* client , int64_t base, int64_t re, int32_t* pstarttime, u_int32_t* ptimestamp)
{
    int ret = 0;

    // packet data
    char type;
    int size;
    char* data = NULL;
    int64_t last_timestamp, now;

    last_timestamp = srs_utils_time_ms();
    srs_human_trace("start ingest flv to flv stream");
    for (;;) {
        now = srs_utils_time_ms();
/*        if (now - last_timestamp >= 3000) {
            last_timestamp = now;
            if((ret = inject_profiling_packet_sei(ortmp, now, *ptimestamp+base)) != 0) {
                return ret;
            }
        }*/
        // tag header
        if ((ret = srs_flv_read_tag_header(flv, &type, &size, ptimestamp)) != 0) {
            if (srs_flv_is_eof(ret)) {
                srs_human_trace("parse completed.");
                return 0;
            }
            srs_human_trace("flv get packet failed. ret=%d", ret);
            return ret;
        }

        if (size <= 0) {
            srs_human_trace("invalid size=%d", size);
            break;
        }
        

        // TODO: FIXME: mem leak when error.
        data = new char [size];
        if ((ret = srs_flv_read_tag_data(flv, data, size)) != 0) {
            goto free_data;
        }

        // make sure srs_rtmp_write_packet will delete data in any conditions
        ret = srs_hdl_write_tag(client, type, *ptimestamp+base, data, size);
        if (ret != 0) {
            srs_human_trace("do proxy write flv tag failed. ret = %d.", ret);
            goto free_data;
        }
        if (*pstarttime < 0 && srs_utils_flv_tag_is_av(type)) {
            *pstarttime = *ptimestamp;
        }

        re_update(re, *pstarttime, *ptimestamp);
    }

free_data:
    if (data) {
        delete [] data;
    }
    
    return ret;
}

int proxy(srs_flv_t flv, void* client, int recur)
{
    int ret = 0;
    u_int32_t timestamp = 0;
    int32_t starttime = -1;
    char header[13];
    int64_t base_time = srs_utils_time_ms();
    int64_t base = 0;
    int64_t last_time = base;
    do {
        timestamp = 0;
        starttime = -1;
        base = srs_utils_time_ms() - base_time;
        tools_main_entrance_startup_time = srs_utils_time_ms();
        if ((ret = srs_flv_read_header(flv, header)) != 0) {
            return ret;
        }
        ret = srs_hdl_write_header(client, header);
        if (ret != 0) {
            srs_human_trace("proxy write flv header failed. ret = %d.", ret);
            break;
        }

        int64_t re = re_create();

        ret = do_proxy(flv, client, base, re, &starttime, &timestamp);


        // for the last pulse, always sleep.
        re_cleanup(re, starttime, timestamp);
        srs_flv_lseek(flv, 0);

        recur--;
        last_time = base + timestamp;
    } while(recur > 0 && ret == 0);

    return ret;
}

int64_t re_create()
{
    // if not very precise, we can directly use this as re.
    int64_t re = srs_utils_time_ms();

    // use the starttime to get the deviation
    int64_t deviation = re - tools_main_entrance_startup_time;
    srs_human_trace("deviation is %d ms, pulse is %d ms", (int)(deviation), (int)(RE_PULSE_MS));

    // so, we adjust time to max(0, deviation)
    // because the last pulse, we already sleeped
    int adjust = (int)(deviation);
    if (adjust > 0) {
        srs_human_trace("adjust re time for %d ms", adjust);
        re -= adjust;
    } else {
        srs_human_trace("no need to adjust re time");
    }

    return re;
}
void re_update(int64_t re, int32_t starttime, u_int32_t time)
{
    // send by pulse algorithm.
    int64_t now = srs_utils_time_ms();
    int64_t diff = time - starttime - (now -re);
    if (diff > RE_PULSE_MS && diff < RE_PULSE_JITTER_MS) {
        usleep((useconds_t)(diff * 1000));
    }
}
void re_cleanup(int64_t re, int32_t starttime, u_int32_t time)
{
    // for the last pulse, always sleep.
    // for the virtual live encoder long time publishing.
    int64_t now = srs_utils_time_ms();
    int64_t diff = time - starttime - (now -re);
    if (diff > 0) {
        srs_human_trace("re_cleanup, diff=%d, start=%d, last=%d ms",
            (int)diff, starttime, time);
        usleep((useconds_t)(diff * 1000));
    }
}
