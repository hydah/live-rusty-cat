/**
# Example to use srs-librtmp
# see: https://github.com/winlinvip/simple-rtmp-server/wiki/v2_CN_SrsLibrtmp
    gcc main.cpp srs_librtmp.cpp -g -O0 -lstdc++ -o output
*/
//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "srs_librtmp.h"

int main(int argc, char** argv)
{

	int frame_count = 0;
	int is_firstI = 0;
	long interval = 0, count_interval = 0;
	int64_t last_time, now_time;
	
	now_time = last_time = srs_utils_time_ms();
    printf("suck rtmp stream like rtmpdump\n");
    printf("srs(simple-rtmp-server) client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());

    if (argc <= 2) {
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
	count_interval = atoi(argv[2]);
	srs_human_trace("time_interval: %d", count_interval);

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

        if (srs_rtmp_read_packet(rtmp, &type, &timestamp, &data, &size, &stream_id) != 0) {
            goto rtmp_destroy;
        }
        if (srs_utils_parse_timestamp(timestamp, type, data, size, &pts) != 0) {
            goto rtmp_destroy;
        }
	    char frame_type = srs_utils_flv_video_frame_type(data, size);
	    char avc_packet_type = srs_utils_flv_video_avc_packet_type(data, size);
		now_time = srs_utils_time_ms();
	    interval = now_time - last_time ;
		last_time = now_time;
	    if((avc_packet_type == 1) && (frame_type == 1 || frame_type == 2)){
	        if(is_firstI == 0 && frame_type == 1){
	   			srs_human_trace("---------play stream start from the first I frame at %ld ms.",now_time);
				is_firstI = 1;
	   	    }
		    frame_count++;
	    }
		if(interval >= count_interval){
		   	srs_human_trace("---------------------------------------play stream past %ld ms, frame count is %d, fps is %d .",interval,frame_count,frame_count*1000/interval);
		   	frame_count = 0;	
	  	}

        delete data;
    }

rtmp_destroy:
    srs_rtmp_destroy(rtmp);

    return 0;
}

