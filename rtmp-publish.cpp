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
#include <fcntl.h>
#include <string>
#include "srs_librtmp.h"
using namespace std;

int proxy(srs_flv_t flv, srs_rtmp_t ortmp, int recur);
int do_proxy(srs_flv_t flv, srs_rtmp_t ortmp, int64_t base, int64_t re, int32_t* pstarttime, u_int32_t* ptimestamp);

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
           "   out_rtmp_url    output rtmp url, publish to this url.\n"
           "For example:\n"
           "   %s -i doc/source.200kbps.768x320.flv -y rtmp://127.0.0.1/live/livestream\n"
           "   %s -i ../../doc/source.200kbps.768x320.flv -y rtmp://127.0.0.1/live/livestream\n",
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

int main(int argc, char** argv)
{
    print_header();
    int ret = 0;
    srs_rtmp_t ortmp;
    srs_flv_t flv;
    string input_file, url, concat_txt;
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
    if (!concat_txt.empty()) {
        srs_human_trace("concat_txt: %s", concat_txt.c_str());
    } else {
        srs_human_trace("input:  %s", input_file.c_str());
    }
    srs_human_trace("output: %s", url.c_str());
    srs_human_trace("recur: %d", recur);

    ortmp = srs_rtmp_create(url.c_str());
    // set 4s timeout
    srs_rtmp_set_timeout(ortmp, 4000, 4000);

    if (!concat_txt.empty()) {
        int ret = 0;
        u_int32_t timestamp = 0;
        int32_t starttime = -1;
        char header[13];
        if ((ret = connect_oc(ortmp)) != 0) {
            return ret;
        }

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
                    srs_flv_close(flv);
                    break;
                }

                int64_t re = re_create();

                ret = do_proxy(flv, ortmp, base, re, &starttime, &timestamp);
                if (ret != 0) {
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
            srs_human_trace("open flv file failed. ret=%d", ret);
            return ret;
        }

        ret = proxy(flv, ortmp, recur);
        srs_human_trace("ingest flv to RTMP completed");

        srs_rtmp_destroy(ortmp);
        srs_flv_close(flv);
    }

    return ret;
}

int do_proxy(srs_flv_t flv, srs_rtmp_t ortmp, int64_t base, int64_t re, int32_t* pstarttime, u_int32_t* ptimestamp)
{
    int ret = 0;

    // packet data
    char type;
    int size;
    char* data = NULL;
    int64_t last_timestamp, now;

    last_timestamp = srs_utils_time_ms();
    srs_human_trace("start ingest flv to RTMP stream");
    for (;;) {
        now = srs_utils_time_ms();
        if (now - last_timestamp >= 3000) {
            last_timestamp = now;
            if((ret = inject_profiling_packet_sei(ortmp, now, *ptimestamp+base)) != 0) {
                return ret;
            }
        }
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

        u_int32_t timestamp = *ptimestamp;
        // make sure srs_rtmp_write_packet will delete data in any conditions
        if ((ret = srs_rtmp_write_packet(ortmp, type, *ptimestamp+base, data, size)) != 0) {
            srs_human_trace("irtmp get packet failed. ret=%d", ret);
            goto out;
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
out:
    return ret;
}

int proxy(srs_flv_t flv, srs_rtmp_t ortmp, int recur)
{
    int ret = 0;
    u_int32_t timestamp = 0;
    int32_t starttime = -1;
    char header[13];
    if ((ret = connect_oc(ortmp)) != 0) {
        return ret;
    }

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

        int64_t re = re_create();

        ret = do_proxy(flv, ortmp, base, re, &starttime, &timestamp);


        // for the last pulse, always sleep.
        re_cleanup(re, starttime, timestamp);
        srs_flv_lseek(flv, 0);

        recur--;
        last_time = base + timestamp;
    } while(recur > 0 && ret == 0);

    if ((ret = inject_h264_stream_eof_packet(ortmp, last_time)) != 0)
    {
        return ret;
    }

    if ((ret = srs_rtmp_fmle_unpublish_stream(ortmp)) != 0) {
        return ret;
    }
    return ret;
}

int connect_oc(srs_rtmp_t ortmp)
{
    int ret = 0;

    if ((ret = srs_rtmp_handshake(ortmp)) != 0) {
        srs_human_trace("ortmp simple handshake failed. ret=%d", ret);
        return ret;
    }
    srs_human_trace("ortmp simple handshake success");

    if ((ret = srs_rtmp_connect_app(ortmp)) != 0) {
        srs_human_trace("ortmp connect vhost/app failed. ret=%d", ret);
        return ret;
    }
    srs_human_trace("ortmp connect vhost/app success");

    if ((ret = srs_rtmp_publish_stream(ortmp)) != 0) {
        srs_human_trace("ortmp publish stream failed. ret=%d", ret);
        return ret;
    }
    srs_human_trace("ortmp publish stream success");

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
