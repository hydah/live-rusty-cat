/**
# Example to use srs-librtmp
# see: https://github.com/winlinvip/simple-rtmp-server/wiki/v2_CN_SrsLibrtmp
    gcc main.cpp srs_librtmp.cpp -g -O0 -lstdc++ -o output
*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sstream>
#include <signal.h>
#include "librtmp/srs_librtmp.h"
using namespace std;

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
static volatile int keep_running = 1;

void print_usage(int argc, char** argv)
{
    printf("Usage: %s -i <rtmp_url> -r <time_interval> -t <total_time> -s (<print_summary>) -d (<print_detail>) -m (<parse_metadata>)\n"
        "   rtmp_url     RTMP stream url to play\n"
        "   time_interval   count frames per time_interval (millisecond), default is 1000, must be a positive value.\n"
        "   total_time   total time of record (millisecond), default is 10000, must be a positive value.\n"
        "   print_summary   only print summary, default is false.\n"
        "   print_detail    print detailed infomation."
        "   parse_metadata parse metadata and print. \n"
        "For example:\n"
        "   %s -i rtmp://127.0.0.1:1935/live/livestream -r 1000 -t 10000 -d\n"
        "   %s -i rtmp://127.0.0.1:1935/live/livestream -r 1000 -t 10000 -m\n"
        "   %s -i rtmp://127.0.0.1:1935/live/livestream -s\n",
        argv[0], argv[0], argv[0], argv[0]);
}
static long count_interval = 1000;
static long total_time = 10000;
bool print_sum = false;
bool print_json = false;
bool print_detail = false;
char *input_url = NULL;
bool parse_metadata = false;

void parse_configure(int argc, char** argv)
{
    const char *optString = "i:t:r:dmsj";
    char opt;
    while ((opt = getopt(argc, argv, optString)) != -1) {
        switch(opt) {
            case 'i':
                input_url = (char *)malloc(strlen(optarg)+1);
                strcpy(input_url, optarg);
                break;
            case 't':
                total_time = atoi(optarg);
                break;
            case 'r':
                count_interval = atoi(optarg);
                break;
            case 'd':
                print_detail = true;
                break;
            case 'm':
                parse_metadata = true;
                break;
            case 's':
                print_sum = true;
                break;
            case 'j':
                print_json = true;
                break;
            case '?':
                print_usage(argc, argv);
                exit(-1);
            default:
                print_usage(argc, argv);
                exit(-1);
        }
    }
    if((total_time <= 0) || (count_interval <= 0)){
        print_usage(argc, argv);
        exit(-1);
    }
    return;
}

void sig_handler(int sig)
{
    if (sig == SIGINT) {
        keep_running = 0;
    }
}

int main(int argc, char** argv)
{
    signal(SIGINT, sig_handler );
    signal(SIGTERM, sig_handler );
	int frame_count = 0, total_count = 0;
	int is_firstI = 0, nonfluency_count = 0;
	long start_time = 0, first_frame_time = 0;
    long handshake_time = 0, connection_time = 0;
	int64_t last_time, now_time, interval;
    int64_t e2e = 0, e2relay = 0, e2edge = 0, sei_count = 0;
    float nonfluency_rate = 0;
	now_time = last_time = start_time = srs_utils_time_ms();
    if (argc < 2) {
        print_usage(argc,argv);
       	return -1;
    }

    // startup socket for windows.
    parse_configure(argc, argv);
    srs_rtmp_t rtmp = srs_rtmp_create(input_url);
    if (!print_sum) {
        srs_human_trace("suck rtmp stream like rtmpdump");
        srs_human_trace("srs(simple-rtmp-server) client librtmp library.");
        srs_human_trace("version: %d.%d.%d", srs_version_major(), srs_version_minor(), srs_version_revision());
        srs_human_trace("rtmp url: %s", input_url);
	    srs_human_trace("time_interval: %d", count_interval);
        srs_human_trace("total_time: %d", total_time);
    }
    if (srs_rtmp_handshake(rtmp) != 0) {
        srs_human_trace("simple handshake failed.");
        goto rtmp_destroy;
    }
    handshake_time = srs_utils_time_ms();
    if (srs_rtmp_connect_app(rtmp) != 0) {
        srs_human_trace("connect vhost/app failed.");
        goto rtmp_destroy;
    }
    connection_time = srs_utils_time_ms();
    if (srs_rtmp_play_stream(rtmp) != 0) {
        srs_human_trace("play stream failed.");
        goto rtmp_destroy;
    }
    if (!print_sum) {
        srs_human_trace("simple handshake success");
        srs_human_trace("connect vhost/app success");
        srs_human_trace("play stream success");
    }
    for (;keep_running!=0;) {
        int size;
        char type;
        char* data = NULL;
        uint32_t timestamp, pts;
        uint32_t stream_id;
        std::stringstream ss;

		now_time = srs_utils_time_ms();
        if((now_time - start_time) >= total_time){
            break;
        }

	    interval = now_time - last_time ;
		if(interval >= count_interval && is_firstI == 1){
            float fps = float(frame_count*1000)/interval;
            if(fps < 15.0){
                nonfluency_count++;
            }
            if (!print_sum && print_detail) {
              	srs_human_trace("play stream past %ld ms, frame count is %d, fps is %.2f.",interval,frame_count,fps);
            }
            frame_count = 0;
            total_count++;
            last_time = now_time;
	  	}

        // if this func is non-block, it will be better.
        if (srs_rtmp_read_packet(rtmp, &type, &timestamp, &data, &size, &stream_id) != 0) {
            break;
        }

        if (!print_sum && parse_metadata && srs_utils_is_metadata(type, data, size)) {
            std::stringstream ms;
            srs_print_metadata(data, size, ms);
            srs_human_trace("metadata is \n %s", ms.str().c_str());
        }

        if (srs_utils_is_sei_profiling(type, data, size)) {
            srs_print_sei_profiling(data, size, ss, e2e, e2edge, e2relay);
            sei_count++;
            if (!print_sum && print_detail) {
                srs_human_trace("node timestamp \n %s", ss.str().c_str());
            }
        }

        char frame_type = srs_utils_flv_video_frame_type(data, size);
	    char avc_packet_type = srs_utils_flv_video_avc_packet_type(data, size);
	    if((avc_packet_type == SrsVideoAvcFrameTraitNALU) && (frame_type == SrsVideoAvcFrameTypeKeyFrame || frame_type == SrsVideoAvcFrameTypeInterFrame)){
		    frame_count++;
	        if(is_firstI == 0 && frame_type == SrsVideoAvcFrameTypeKeyFrame){
                first_frame_time = now_time;
                if (!print_sum && print_detail) {
                    srs_human_trace("play stream start and the first I frame arrive at %ld ms.",first_frame_time);
                }
                is_firstI = 1;
                last_time = now_time;
	   	    }
        }

        delete data;
    }

    if(total_count != 0) {
        nonfluency_rate = float(nonfluency_count)/total_count;
    }
    if(first_frame_time == 0) {
        nonfluency_rate = 100;
    }
    if (!print_sum) {
        srs_human_trace("play stream end");
    }


rtmp_destroy:
    int avg_e2e = 0, avg_e2relay = 0, avg_e2edge = 0;
    if (sei_count > 0) {
        avg_e2e = e2e/sei_count;
        avg_e2relay = e2relay/sei_count;
        avg_e2edge = e2edge/sei_count;
    }

    if (print_json == true) {
        cout << SRS_JOBJECT_START
             << SRS_JFIELD_STR("address", input_url) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("handshake_time", handshake_time-start_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("connection_time", connection_time-start_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("firstItime", first_frame_time-start_time) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("total_count", total_count) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("nonfluency_count", nonfluency_count) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("nonfluency_rate", nonfluency_rate) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("sei_frame_count", sei_count) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2e", avg_e2e) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2relay", avg_e2relay) << SRS_JFIELD_CONT
             << SRS_JFIELD_ORG("e2edge", avg_e2edge)
             << SRS_JOBJECT_END;
        cout << endl;

    } else {
        printf("address %s, firstItime %ld, total_count %ld, nonfluency_count %ld, nonfluency_rate %.2f, sei_frame_count %d",
               input_url, first_frame_time - start_time, total_count, nonfluency_count, nonfluency_rate, sei_count);
        printf(", e2e %d, e2relay %d, e2edge %d", avg_e2e, avg_e2relay, avg_e2edge);
        printf("\n");
    }
    srs_rtmp_destroy(rtmp);
    return 0;
}
