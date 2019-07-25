/**
# rtmp-play is a rtmp profiling tool based on srs_librtmp
# see: https://github.com/winlinvip/simple-rtmp-server/wiki/v2_CN_SrsLibrtmp
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sstream>
#include <signal.h>
#include "librtmp/srs_librtmp.h"
#include "rtmp-tool.h"
#include "argparse.hpp"
#include <sys/time.h>

using namespace std;

#define CUR_TIME srs_utils_time_ms()

struct LiveRes{
    string addr;
    int64_t e2e;
    int64_t e2relay;
    int64_t e2edge;
    int total_time;
    int runtime;
    int waittime;
    int waitcnt;
    int sei_count;
    int dns_resolve_time;
    int connect_server_time;
    int handshake_time;
    int connection_time;
    int first_frame_time;
    LiveRes() {
        addr = "";
        e2e = e2relay = e2edge = 0;
        runtime = total_time = waittime = waitcnt = 0;
        handshake_time = connection_time = first_frame_time = 0;
        sei_count = 0;
    };
};

struct Config {
    ArgumentParser parser;
    string addr;
    int total_time;
    bool show_json;
    bool only_sum;
    bool debug;
    bool complex_hs;
    bool fast_hs;
    Config() {
        total_time = 10000;
        show_json = only_sum = debug = false;
        complex_hs = false;
        fast_hs = false;
    };
    void parse(int argc, const char **argv) {
        parser.parse(argc, argv);
        only_sum = parser.valid("s");
        debug = parser.valid("d");
        show_json = parser.valid("j");
        fast_hs = parser.valid("F");
        addr = parser.retrieve<string>("i");
        if (parser.valid("t")) {
            total_time = parser.get<int>("t");
        }
        if (parser.valid("H")) {
            string hs_type = parser.retrieve<string>("H");
            if (hs_type == "complex") {
                complex_hs = true;
            }
        }
    }
} config;
bool stop_playing = false;

void sig_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM || sig == SIGALRM) {
        log(WARNING, "got signal %d", sig);
        stop_playing = true;
    }
}

void do_rtmp(LiveRes &live_res)
{
    int64_t start_time, last_time;
    int frame_count = 0;
    bool is_firstI = false;
    int64_t now_time, interval;
    uint32_t last_ts = 0, timestamp = 0;;
    srs_rtmp_t rtmp = NULL;

    start_time = srs_utils_time_ms();

    // startup socket for windows.
    rtmp = srs_rtmp_create(live_res.addr.c_str());
    // set 4s timeout
    srs_rtmp_set_timeout(rtmp, 4000, 4000);


    if (srs_rtmp_dns_resolve(rtmp) != ERROR_SUCCESS) {
        log(ERROR, "dns resolve failed.");
        goto rtmp_destroy;
    }
    live_res.dns_resolve_time = CUR_TIME - start_time;

    if (srs_rtmp_connect_server(rtmp) != ERROR_SUCCESS) {
        log(ERROR, "connect to server faild.");
        goto rtmp_destroy;
    }
    live_res.connect_server_time = CUR_TIME - start_time;

    if (config.fast_hs) {
        if (srs_rtmp_fast_handshake_play(rtmp)!= ERROR_SUCCESS) {
            log(ERROR, "fast handshake failed.");
            goto rtmp_destroy;
        }
    } else {
        if (config.complex_hs) {
            if (srs_rtmp_do_complex_handshake(rtmp) != ERROR_SUCCESS) {
                log(ERROR, "simple handshake failed.");
                goto rtmp_destroy;
            }
        } else {
            if (srs_rtmp_do_simple_handshake(rtmp) != ERROR_SUCCESS) {
                log(ERROR, "complex handshake failed.");
                goto rtmp_destroy;
            }
        }
        live_res.handshake_time = CUR_TIME - start_time;
        log(INFO, "handshake success");

        if (srs_rtmp_connect_app(rtmp) != 0) {
            log(ERROR, "connect vhost/app failed.");
            goto rtmp_destroy;
        }
        log(INFO, "connect vhost/app success");
        live_res.connection_time = srs_utils_time_ms() - start_time;

        if (srs_rtmp_play_stream(rtmp) != 0) {
            log(ERROR, "play stream failed.");
            goto rtmp_destroy;
        }
        log(INFO, "play stream success");
    }

    last_time = srs_utils_time_ms();
    char buffer[500];
    while(!stop_playing) {
        int size;
        char type;
        char* data = NULL;
        uint32_t _timestamp, pts;
        uint32_t stream_id;
        std::stringstream ss;
        int ret = 0;

        now_time = srs_utils_time_ms();

        // if this func is non-block, it will be better.
        if ((ret = srs_rtmp_read_packet(rtmp, &type, &_timestamp, &data, &size, &stream_id)) != 0) {
            log(ERROR, "read packet error, errno is %d", ret);
            break;
        }
        timestamp = _timestamp;

        char frame_type = srs_utils_flv_video_frame_type(data, size);
        char avc_packet_type = srs_utils_flv_video_avc_packet_type(data, size);
        now_time = srs_utils_time_ms();
        if((avc_packet_type == SrsVideoAvcFrameTraitNALU)
            && (frame_type == SrsVideoAvcFrameTypeKeyFrame || frame_type == SrsVideoAvcFrameTypeInterFrame)){
            frame_count++;
            int64_t tmp_time = srs_utils_time_ms();
            int _wt = int(tmp_time - last_time) - int(timestamp - last_ts);
            if (is_firstI && _wt > 100) {
                live_res.waittime += _wt;
                last_time = tmp_time;
                last_ts = timestamp;
                live_res.waitcnt ++;
            }
            if(is_firstI == false && frame_type == SrsVideoAvcFrameTypeKeyFrame){
                live_res.first_frame_time = now_time - start_time;
                log(DEBUG, "play stream start and the first I frame arrive at %ld ms.", live_res.first_frame_time);
                is_firstI = true;
                last_time = tmp_time;
                last_ts = timestamp;
            }
        }

        if (srs_utils_is_metadata(type, data, size)) {
            std::stringstream ms;
            srs_print_metadata(data, size, ms);
            log(INFO, "metadata is \n %s", ms.str().c_str());
        }

        if (srs_utils_is_sei_profiling(type, data, size)) {
            srs_print_sei_profiling(data, size, ss, live_res.e2e, live_res.e2edge, live_res.e2relay);
            live_res.sei_count++;
            log(INFO, "node timestamp \n %s", ss.str().c_str());
        }
        if (config.debug) {
            memset(buffer, 0, sizeof(char)*500);
            srs_human_format_rtmp_packet(buffer, 100, type, timestamp, data, size);
            log(INFO, buffer);
        }
        delete data;
    }

    log(INFO, "play stream end");

rtmp_destroy:
    int64_t tmp_time = srs_utils_time_ms();
    int _wt = int(tmp_time - last_time) - int(timestamp - last_ts);
    if (is_firstI && _wt > 0) {
        live_res.waittime += _wt;
        last_time = tmp_time;
        last_ts = timestamp;
    }
    live_res.runtime = tmp_time - start_time;
    srs_rtmp_destroy(rtmp);
    return;
}

void do_file(LiveRes &live_res)
{
    int64_t start_time, last_time;
    int frame_count = 0;
    bool is_firstI = false;
    int ret;
    int64_t now_time, interval;
    uint32_t last_ts = 0, timestamp = 0;;
    srs_rtmp_t rtmp = NULL;

    start_time = srs_utils_time_ms();
    last_time = srs_utils_time_ms();
    char buffer[600];
    char header[13];
    char *data = NULL;
    srs_flv_t flv;
    log(INFO, "play local file");

    if ((flv = srs_flv_open_read(live_res.addr.c_str())) == NULL) {
        log(ERROR, "open flv file failed");
        return;
    }

    if ((ret = srs_flv_read_header(flv, header)) != 0) {
        srs_flv_close(flv);
        return;
    }
    while(!stop_playing) {
       int size;
       char type;
       uint32_t timestamp;

       // tag header
       if ((ret = srs_flv_read_tag_header(flv, &type, &size, &timestamp)) != 0) {
           if (srs_flv_is_eof(ret)) {
               log(INFO, "parse completed.");
               break;
           }
           log(ERROR, "flv get packet failed. ");
           break;
       }

       if (size <= 0) {
           log(INFO, "invalid size=%d", size);
           break;
       }

      // TODO: FIXME: mem leak when error.
      data = new char[size];
      if ((ret = srs_flv_read_tag_data(flv, data, size)) != 0) {
          goto free_data;
      }
       if (config.debug) {
           memset(buffer, 0, sizeof(char)*600);
           srs_human_format_rtmp_packet(buffer, 200, type, timestamp, data, size);
           log(INFO, buffer);
       }
       delete [] data;
   }

free_data:
    if (data) {
        delete []data;
    }
    srs_flv_close(flv);
    return;
}

void do_http(LiveRes &live_res)
{
}
// todo:
// use buffer time, not frame rate
void run(LiveRes &live_res)
{
    log(INFO, "addr is %s", live_res.addr.c_str());
    if (live_res.addr.find("http://") == 0) {
        do_http(live_res);
    } else if (live_res.addr.find("rtmp://") == 0) {
        do_rtmp(live_res);
    } else {
        do_file(live_res);
    }
}

void print_result(LiveRes &live_res, bool print_json) {
    int avg_e2e = 0, avg_e2relay = 0, avg_e2edge = 0;
    if (live_res.sei_count > 0) {
        avg_e2e = live_res.e2e/live_res.sei_count;
        avg_e2relay = live_res.e2relay/live_res.sei_count;
        avg_e2edge = live_res.e2edge/live_res.sei_count;
    }

    if (print_json == true) {
        cout << SRS_JOBJECT_START
             << SRS_JFIELD_STR("address", live_res.addr) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("total_time", live_res.total_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("run_time", live_res.runtime) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("dns_resolve_time", live_res.dns_resolve_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("connect_servier_time", live_res.connect_server_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("handshake_time", live_res.handshake_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("connection_time", live_res.connection_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("first_itime", live_res.first_frame_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("waiting_time", live_res.waittime) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("waiting_count", live_res.waitcnt) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("sei_frame_count", live_res.sei_count) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2e", avg_e2e) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2relay", avg_e2relay) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2edge", avg_e2edge)
             << SRS_JOBJECT_END;
        cout << endl;

    } else {
        printf("address %s, total_time %d, run_time %d, dns_resolve_time %d, connect_server_time %d, handshake_time %d, "
               "connection_time %d, first_itime %d, waiting_time %d, waiting_count %d, sei_frame_count %d",
               live_res.addr.c_str(), live_res.total_time, live_res.runtime, live_res.dns_resolve_time,
               live_res.connect_server_time, live_res.handshake_time, live_res.connection_time, live_res.first_frame_time,
               live_res.waittime, live_res.waitcnt, live_res.sei_count);
        printf(", e2e %d, e2relay %d, e2edge %d", avg_e2e, avg_e2relay, avg_e2edge);
        printf("\n");
    }
}

int main(int argc, const char** argv)
{
    config.parser.addArgument("-i", "--input_url", 1, false, "rtmp play url. like rtmp://127.0.0.1:1935/live/livestream");
    config.parser.addArgument("-t", "--total_time", 1, true, "the duration that this program will run, 10s if not specified");
    config.parser.addArgument("-d", "--debug", 0, true, "\tprint all the debug info");
    config.parser.addArgument("-s", "--only_summary", 0, true, "only print summary");
    config.parser.addArgument("-j", "--json", 0, true, "\tprint summary with json fomat");
    config.parser.addArgument("-F", "--fast_hs", 0, true, "\tuse fast handshake");
    config.parser.addArgument("-H", "--handshake", 1, true, "handshake type, will use complex handshake only if specified 'complex'");
    config.parser.addFinalArgument("null", 0);
    config.parse(argc, argv);

    LiveRes live_res;
    live_res.addr = config.addr;
    live_res.total_time = config.total_time;
    if (config.only_sum) {
        init_loglevel(SUM);
    } else if (config.debug) {
        init_loglevel(DEBUG);
    }

    log(INFO, "suck rtmp stream like rtmpdump");
    log(INFO, "srs(simple-rtmp-server) client librtmp library.");
    log(INFO, "version: %d.%d.%d", srs_version_major(), srs_version_minor(), srs_version_revision());
    log(INFO, "url: %s", live_res.addr.c_str());
    log(INFO, "total_time: %d", config.total_time);

    signal(SIGALRM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    alarm(config.total_time/1000);

    run(live_res);

    print_result(live_res, config.show_json);
    return 0;
}
