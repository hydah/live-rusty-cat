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
#include <setjmp.h>
#include "librtmp/srs_librtmp.h"
#include "rtmp-tool.h"
#include "argparse.hpp"
#include <sys/time.h>

using namespace std;

struct LiveRes{
    string addr;
    int total_count;
    int nonfluency_count;
    float nonfluency_rate;
    int sei_count;
    int handshake_time;
    int connection_time;
    int first_frame_time;
    int64_t e2e;
    int64_t e2relay;
    int64_t e2edge;
};

struct Config {
    ArgumentParser parser;
    string addr;
    int total_time;
    int count_interval;
    bool show_json;
    bool only_sum;
    bool debug;
    Config() {
        total_time = 10000;
        count_interval = 1000;
        show_json = only_sum = debug = false;
    };
    void parse(int argc, const char **argv) {
        parser.parse(argc, argv);
        only_sum = parser.valid("s");
        debug = parser.valid("d");
        show_json = parser.valid("l");
        addr = parser.retrieve<string>("i");
        if (parser.valid("t")) {
            total_time = parser.retrieve<int>("t");
        }

        if (parser.valid("r")) {
            count_interval = parser.retrieve<int>("r");
        }
    }
} config;
static jmp_buf buf;

void sig_handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM || sig == SIGALRM) {
        longjmp(buf, 1);
    }
}

// todo:
// use buffer time, not frame rate
void run(LiveRes &live_res)
{
    int64_t start_time, last_time;
    int frame_count = 0;
    bool is_firstI = false;
    int64_t now_time, interval;
    srs_rtmp_t rtmp = NULL;

    start_time = srs_utils_time_ms();
    if (!setjmp(buf)) {
        // startup socket for windows.
        rtmp = srs_rtmp_create(live_res.addr.c_str());
        // set 4s timeout
        srs_rtmp_set_timeout(rtmp, 4000, 4000);

        if (srs_rtmp_handshake(rtmp) != 0) {
            log(ERROR, "simple handshake failed.");
            goto rtmp_destroy;
        }
        log(INFO, "simple handshake success");
        live_res.handshake_time = srs_utils_time_ms() - start_time;

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

        last_time = srs_utils_time_ms();
        for (;;) {
            int size;
            char type;
            char* data = NULL;
            uint32_t timestamp, pts;
            uint32_t stream_id;
            std::stringstream ss;
            int ret = 0;

            now_time = srs_utils_time_ms();
            int64_t interval = now_time - last_time ;
            if (interval >= config.count_interval) {
                float fps = float(frame_count*1000)/interval;
                if(fps < 15.0){
                    live_res.nonfluency_count++;
                }
                log(DEBUG, "play stream past %ld ms, frame count is %d, fps is %.2f.",interval,frame_count,fps);
                live_res.total_count++;
                frame_count = 0;
                last_time = now_time;
            }

            // if this func is non-block, it will be better.
            if ((ret = srs_rtmp_read_packet(rtmp, &type, &timestamp, &data, &size, &stream_id)) != 0) {
                log(ERROR, "read packet error, errno is %d", ret);
                break;
            }

            if (false && srs_utils_is_metadata(type, data, size)) {
                std::stringstream ms;
                srs_print_metadata(data, size, ms);
                log(INFO, "metadata is \n %s", ms.str().c_str());
            }

            if (srs_utils_is_sei_profiling(type, data, size)) {
                srs_print_sei_profiling(data, size, ss, live_res.e2e, live_res.e2edge, live_res.e2relay);
                live_res.sei_count++;
                log(INFO, "node timestamp \n %s", ss.str().c_str());
            }

            char frame_type = srs_utils_flv_video_frame_type(data, size);
            char avc_packet_type = srs_utils_flv_video_avc_packet_type(data, size);
            now_time = srs_utils_time_ms();
            if((avc_packet_type == SrsVideoAvcFrameTraitNALU)
                && (frame_type == SrsVideoAvcFrameTypeKeyFrame || frame_type == SrsVideoAvcFrameTypeInterFrame)){
                frame_count++;
                if(is_firstI == false && frame_type == SrsVideoAvcFrameTypeKeyFrame){
                    live_res.first_frame_time = now_time - start_time;
                    log(DEBUG, "play stream start and the first I frame arrive at %ld ms.", live_res.first_frame_time);
                    is_firstI = true;
                }
            }

            delete data;
        }
    }

    if(live_res.total_count != 0) {
        live_res.nonfluency_rate = float(live_res.nonfluency_count)/live_res.total_count;
    }
    if(live_res.first_frame_time == 0) {
        live_res.nonfluency_rate = 100;
    }
    log(INFO, "play stream end");

rtmp_destroy:
    srs_rtmp_destroy(rtmp);
    return;
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
             << SRS_JFIELD_ORG("handshake_time", live_res.handshake_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("connection_time", live_res.connection_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("first_itime", live_res.first_frame_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("total_count", live_res.total_count) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("nonfluency_count", live_res.nonfluency_count) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("nonfluency_rate", live_res.nonfluency_rate) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("sei_frame_count", live_res.sei_count) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2e", avg_e2e) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2relay", avg_e2relay) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2edge", avg_e2edge)
             << SRS_JOBJECT_END;
        cout << endl;

    } else {
        printf("address %s, handshake_time %d, connection_time %d, first_itime %d, total_count %d, nonfluency_count %d, nonfluency_rate %.2f, sei_frame_count %d",
               live_res.addr.c_str(), live_res.handshake_time, live_res.connection_time, live_res.first_frame_time, live_res.total_count, live_res.nonfluency_count, live_res.nonfluency_rate, live_res.sei_count);
        printf(", e2e %d, e2relay %d, e2edge %d", avg_e2e, avg_e2relay, avg_e2edge);
        printf("\n");
    }
}

int main(int argc, const char** argv)
{
    config.parser.addArgument("-i", "--input_url", 1, false, "rtmp play url. like rtmp://127.0.0.1:1935/live/livestream");
    config.parser.addArgument("-t", "--total_time", 1, true, "the duration that this program will run, 10s if not specified");
    config.parser.addArgument("-r", "--interval", 1, true, "the interval when checking frame rate, default is 1s");
    config.parser.addArgument("-d", "--debug", 0, true, "\tprint all the debug info");
    config.parser.addArgument("-s", "--only_summary", 0, true, "only print summary");
    config.parser.addArgument("-j", "--json", 0, true, "\tprint summary with json fomat");
    config.parser.addFinalArgument("null", 0);
    config.parse(argc, argv);

    LiveRes live_res;
    live_res.addr = config.addr;
    if (config.only_sum) {
        init_loglevel(SUM);
    } else if (config.debug) {
        init_loglevel(DEBUG);
    }

    log(INFO, "suck rtmp stream like rtmpdump");
    log(INFO, "srs(simple-rtmp-server) client librtmp library.");
    log(INFO, "version: %d.%d.%d", srs_version_major(), srs_version_minor(), srs_version_revision());
    log(INFO, "rtmp url: %s", live_res.addr.c_str());
    log(INFO, "time_interval: %d", config.count_interval);
    log(INFO, "total_time: %d", config.total_time);

    signal(SIGALRM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    alarm(config.total_time/1000);

    run(live_res);

    print_result(live_res, config.show_json);
    return 0;
}
