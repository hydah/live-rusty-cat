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
    printf("Usage: %s -i <rtmp_url> -r <time_interval> -t <total_time> -p (<print_log>)\n"
        "   rtmp_url     RTMP stream url to play\n"
        "   time_interval   count frames per time_interval (millisecond), default is 1000, must be a positive value.\n"
        "   total_time   total time of record (millisecond), default is 10000, must be a positive value.\n"
        "   print_log   print record log of processing, default is 0.\n"
        "For example:\n"
        "   %s -i rtmp://127.0.0.1:1935/live/livestream -r 1000 -t 10000 -p \n"
        "   %s -i rtmp://ossrs.net:1935/live/livestream \n",
        argv[0], argv[0], argv[0]);
}

void parse_configure(int argc, char** argv, char **input_url, long &count_interval, long &total_time, int &print_log)
{
    count_interval = 1000, total_time = 10000, print_log = 0;
    const char *optString = "i:t:r:p?";
    char opt;
    while ((opt = getopt(argc, argv, optString)) != -1) {
        switch(opt) {
            case 'i':
                *input_url = optarg;
                break;
            case 't':
                total_time = atoi(optarg);
                break;
            case 'r':
                count_interval = atoi(optarg);
                break;
            case 'p':
                print_log = 1;
                break;
            case '?':
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
    if ( sig == SIGINT)
    {
        keep_running = 0;
    }
}
int main(int argc, char** argv)
{
    signal( SIGINT, sig_handler );
    signal( SIGTERM, sig_handler );
	int frame_count = 0, total_count = 0;
	int is_firstI = 0, nonfluency_count = 0, print_log = 0;
	long interval = 0, count_interval = 0, total_time = 0, start_time = 0, first_frame_time = 0;
	int64_t last_time, now_time;
    int64_t e2e = 0, e2relay = 0, e2edge = 0, sei_count = 0;
    float nonfluency_rate = 0;
    char *input_url;
	now_time = last_time = start_time = srs_utils_time_ms();
    printf("suck rtmp stream like rtmpdump\n");
    printf("srs(simple-rtmp-server) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());

    if (argc < 2) {
        print_usage(argc,argv);
       	return -1;
    }

    // startup socket for windows.
    parse_configure(argc, argv,&input_url,count_interval,total_time,print_log);
    srs_human_trace("rtmp url: %s", input_url);
    srs_rtmp_t rtmp = srs_rtmp_create(input_url);
	srs_human_trace("time_interval: %d", count_interval);
    srs_human_trace("total_time: %d", total_time);
    if (srs_rtmp_handshake(rtmp) != 0) {
        srs_human_trace("simple handshake failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("simple handshake success");

    if (srs_rtmp_connect_app(rtmp) != 0) {
        srs_human_trace("connect vhost/app failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("connect vhost/app success");

    if (srs_rtmp_play_stream(rtmp) != 0) {
        srs_human_trace("play stream failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("play stream success");
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
    srs_human_trace("play stream end");
    printf("--------------\n");
    printf("total count %d, non count %d\n", total_count, nonfluency_count);
    if (sei_count > 0) {
        printf("first arrival of I frame spent %ld ms, nonfluency rate %.2f, e2e %d ms, e2relay %d ms, e2edge %dms\n",
                first_frame_time-start_time, nonfluency_rate, e2e/sei_count, e2relay/sei_count, e2edge/sei_count);
    } else {
        printf("first arrival of I frame spent %ld ms, nonfluency rate %.2f\n",
                first_frame_time-start_time, nonfluency_rate);
    }

rtmp_destroy:
    srs_rtmp_destroy(rtmp);
    return 0;
}

