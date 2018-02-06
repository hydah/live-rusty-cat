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

#include "librtmp/srs_librtmp.h"
#define SrsVideoAvcFrameTraitNALU 1
#define SrsVideoAvcFrameTypeKeyFrame 1
#define SrsVideoAvcFrameTypeInterFrame 2

int main(int argc, char** argv)
{

	int frame_count = 0, total_count = 0;
	int is_firstI = 0, nonfluency_count = 0;
	long interval = 0, count_interval = 1000, total_time = 10000, start_time = 0, first_frame_time = 0;
	int64_t last_time, now_time;
    int64_t e2e = 0, e2relay = 0, e2edge = 0, sei_count = 0;

	now_time = last_time = start_time = srs_utils_time_ms();
    printf("suck rtmp stream like rtmpdump\n");
    printf("srs(simple-rtmp-server) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());

    if (argc <= 1) {
        printf("Usage: %s <rtmp_url> <time_interval>\n"
            "   rtmp_url     RTMP stream url to play\n"
            "   time_interval   count frames per time_interval millisecond\n"
            "For example:\n"
            "   %s rtmp://127.0.0.1:1935/live/livestream 1000\n"
            "   %s rtmp://ossrs.net:1935/live/livestream 1000\n",
            argv[0], argv[0], argv[0]);
       	return -1;
    }

    // startup socket for windows.

    srs_human_trace("rtmp url: %s", argv[1]);
    srs_rtmp_t rtmp = srs_rtmp_create(argv[1]);
    if(argc > 2){
        count_interval = atoi(argv[2]);
    }
	srs_human_trace("time_interval: %d", count_interval);
    if(argc > 3){
        total_time = atoi(argv[3]);
    }
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
    for (;;) {
        int size;
        char type;
        char* data;
        uint32_t timestamp, pts;
        uint32_t stream_id;
        std::stringstream ss;

        if (srs_rtmp_read_packet(rtmp, &type, &timestamp, &data, &size, &stream_id) != 0) {
            goto rtmp_destroy;
        }

        if (srs_utils_is_sei_profiling(type, data, size)) {
            srs_print_sei_profiling(data, size, ss, e2e, e2edge, e2relay);
            sei_count++;
            srs_human_trace("node timestamp \n %s", ss.str().c_str());
        }

	    char frame_type = srs_utils_flv_video_frame_type(data, size);
	    char avc_packet_type = srs_utils_flv_video_avc_packet_type(data, size);
		now_time = srs_utils_time_ms();
	    interval = now_time - last_time ;
	    if((avc_packet_type == SrsVideoAvcFrameTraitNALU) && (frame_type == SrsVideoAvcFrameTypeKeyFrame || frame_type == SrsVideoAvcFrameTypeInterFrame)){
	        if(is_firstI == 0 && frame_type == SrsVideoAvcFrameTypeKeyFrame){
                first_frame_time = now_time;
	   			srs_human_trace("play stream start and the first I frame arrive at %ld ms.",first_frame_time);
				is_firstI = 1;
	   	    }
		    frame_count++;
	    }
		if(interval >= count_interval){
            float fps = frame_count*1000/interval;
            if(fps < 15){
                nonfluency_count++;
            }
		   	srs_human_trace("play stream past %ld ms, frame count is %d, fps is %.2f.",interval,frame_count,fps);
		   	frame_count = 0;
            total_count++;
            last_time = now_time;
	  	}
        delete data;
        if((now_time - start_time) >= total_time){
            goto rtmp_destroy;
        }
    }

rtmp_destroy:
    float nonfluency_rate = 0;
    if(total_count != 0){
         nonfluency_rate = nonfluency_count/total_count;
    }
     if(first_frame_time == 0){
        nonfluency_rate = 100;
    }
    srs_human_trace("play stream end");
    printf("--------------\n");
    printf("first arival of I frame spent %ld ms, nonfluency rate %.2f, e2e %d, e2relay %d, e2edge %d\n",
            first_frame_time-start_time, nonfluency_rate, e2e/sei_count, e2relay/sei_count, e2edge/sei_count);
    srs_rtmp_destroy(rtmp);
    return 0;
}

