/**
# Example to use srs-librtmp
# see: https://github.com/winlinvip/simple-rtmp-server/wiki/v2_CN_SrsLibrtmp
    gcc main.cpp srs_librtmp.cpp -g -O0 -lstdc++ -o output
*/
//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sstream>
#include <signal.h>
#include "librtmp/srs_librtmp.h"
#define SrsVideoAvcFrameTraitNALU 1
#define SrsVideoAvcFrameTypeKeyFrame 1
#define SrsVideoAvcFrameTypeInterFrame 2
static volatile int keep_running = 1;

void print_usage(int argc, char** argv)
{
    printf("Usage: %s -i <rtmp_url> -r <time_interval> -t <total_time> -p (<print_log>) -m (<parse_metadata>)\n"
        "   rtmp_url     RTMP stream url to play\n"
        "   time_interval   count frames per time_interval (millisecond), default is 1000, must be a positive value.\n"
        "   total_time   total time of record (millisecond), default is 10000, must be a positive value.\n"
        "   print_log   print record log of processing, default is 0.\n"
        "   parse_metadata parse metadata and print. \n"
        "For example:\n"
        "   %s -i rtmp://127.0.0.1:1935/live/livestream -r 1000 -t 10000 -p  -m\n"
        "   %s -i rtmp://ossrs.net:1935/live/livestream \n",
        argv[0], argv[0], argv[0]);
}
static long count_interval = 1000;
static long total_time = 10000;
bool print_log = false;
bool print_sum = false;
char *input_url = NULL;
bool parse_metadata = false;

void parse_configure(int argc, char** argv)
{
    const char *optString = "i:t:r:pms";
    char opt;
    while ((opt = getopt(argc, argv, optString)) != -1) {
        switch(opt) {
            case 'i':
                input_url = (char *)malloc(strlen(optarg));
                strcpy(input_url, optarg);
                break;
            case 't':
                total_time = atoi(optarg);
                break;
            case 'r':
                count_interval = atoi(optarg);
                break;
            case 'p':
                print_log = true;
                break;
            case 'm':
                parse_metadata = true;
                break;
            case 's':
                print_sum = true;
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

void sig_handler( int sig )
{
    if ( sig == SIGINT) {
        keep_running = 0;
    }
}

int main(int argc, char** argv)
{
    signal( SIGINT, sig_handler );
    signal( SIGTERM, sig_handler );
	int frame_count = 0, total_count = 0;
	int is_firstI = 0, nonfluency_count = 0;
	long start_time = 0, first_frame_time = 0;
	int64_t last_time, now_time, interval;
    int64_t e2e = 0, e2relay = 0, e2edge = 0, sei_count = 0;
    float nonfluency_rate = 0;
	now_time = last_time = start_time = srs_utils_time_ms();
    if (argc < 2) {
        if (print_log){
            print_usage(argc,argv);
        }
       	return -1;
    }

    // startup socket for windows.
    parse_configure(argc, argv);
    srs_rtmp_t rtmp = srs_rtmp_create(input_url);
    if (print_log){
        srs_human_trace("suck rtmp stream like rtmpdump");
        srs_human_trace("srs(simple-rtmp-server) client librtmp library.");
        srs_human_trace("version: %d.%d.%d", srs_version_major(), srs_version_minor(), srs_version_revision());
        srs_human_trace("rtmp url: %s", input_url);
	    srs_human_trace("time_interval: %d", count_interval);
        srs_human_trace("total_time: %d", total_time);
    }
    if (srs_rtmp_handshake(rtmp) != 0) {
        if (print_log){
            srs_human_trace("simple handshake failed.");
        }
        goto rtmp_destroy;
    }
    if (srs_rtmp_connect_app(rtmp) != 0) {
        if (print_log){
            srs_human_trace("connect vhost/app failed.");
        }
        goto rtmp_destroy;
    }
    if (srs_rtmp_play_stream(rtmp) != 0) {
        if (print_log){
            srs_human_trace("play stream failed.");
        }
        goto rtmp_destroy;
    }
    if (print_log){
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
            if (print_log)
            {
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

        if (parse_metadata && srs_utils_is_metadata(type, data, size)) {
            std::stringstream ms;
            srs_print_metadata(data, size, ms);
            if (print_log)
            {
                srs_human_trace("metadata is \n %s", ms.str().c_str());
            }
        }

        if (srs_utils_is_sei_profiling(type, data, size)) {
            srs_print_sei_profiling(data, size, ss, e2e, e2edge, e2relay);
            sei_count++;
            if (print_log)
            {
                srs_human_trace("node timestamp \n %s", ss.str().c_str());
            }
        }

        char frame_type = srs_utils_flv_video_frame_type(data, size);
	    char avc_packet_type = srs_utils_flv_video_avc_packet_type(data, size);
	    if((avc_packet_type == SrsVideoAvcFrameTraitNALU) && (frame_type == SrsVideoAvcFrameTypeKeyFrame || frame_type == SrsVideoAvcFrameTypeInterFrame)){
		    frame_count++;
	        if(is_firstI == 0 && frame_type == SrsVideoAvcFrameTypeKeyFrame){
                first_frame_time = now_time;
                if (print_log)
                {
                    srs_human_trace("play stream start and the first I frame arrive at %ld ms.",first_frame_time);
                }
                is_firstI = 1;
                last_time = now_time;
	   	    }
        }

        delete data;
    }

    if(total_count != 0){
         nonfluency_rate = float(nonfluency_count)/total_count;
    }
     if(first_frame_time == 0){
        nonfluency_rate = 100;
    }
    if (print_log)
    {
        srs_human_trace("play stream end");
    }
    printf("address %s, firstItime %ld, total_count %ld, nonfluency_count %ld, nonfluency_rate %.2f, sei_frame_count %d",
           input_url, first_frame_time - start_time, total_count, nonfluency_count, nonfluency_rate, sei_count);
    if (sei_count > 0) {
        printf(", e2e %d, e2relay %d, e2edge %d",e2e/sei_count, e2relay/sei_count, e2edge/sei_count);
    } 
    printf("\n");

rtmp_destroy:
    srs_rtmp_destroy(rtmp);
    return 0;
}

