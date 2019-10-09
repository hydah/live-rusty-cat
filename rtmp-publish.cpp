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

#include "argparse.hpp"
#include "lib-livestream.hpp"

using namespace std;

int proxy(srs_flv_t flv, srs_rtmp_t ortmp, int recur);
int do_proxy(srs_flv_t flv, srs_rtmp_t ortmp, int64_t base, int64_t re, int32_t* pstarttime, u_int32_t* ptimestamp);
int connect_oc(srs_rtmp_t ortmp);
int64_t tools_main_entrance_startup_time;
struct Config {
    ArgumentParser parser;
    string addr;
    string input_file;
    string concat_file;
    int recur_times;
    Config() {
        recur_times = 1;
    };

    void parse(int argc, const char **argv) {
        parser.parse(argc, argv);
        input_file = parser.retrieve<string>("i");
        addr = parser.retrieve<string>("y");
        concat_file = parser.retrieve<string>("f");
        if (parser.valid("r")) {
            recur_times = parser.get<int>("r");
        }
    }
} config;

void init(int argc, const char** argv) {
    config.parser.addArgument("-i", "--input_file", 1, false, "\trtmp play url. like rtmp://127.0.0.1:1935/live/livestream");
    config.parser.addArgument("-y", "--tcURL", 1, false, "\tthe duration that this program will run, 10s if not specified");
    config.parser.addArgument("-f", "--concat_file", 1, true, "print all the debug info");
    config.parser.addArgument("-r", "--recur_times", 1, true, "only print summary");
    config.parser.addFinalArgument("null", 0);
    config.parse(argc, argv);
    tools_main_entrance_startup_time = srs_utils_time_ms();
}

int publish_concat(srs_rtmp_t ortmp, string concat_file, int recur)
{
    int ret = 0;
    srs_flv_t flv;
    u_int32_t timestamp = 0;
    int32_t starttime = -1;
    char header[13];
    if ((ret = connect_oc(ortmp)) != 0) {
        return ret;
    }

    int64_t base_time = srs_utils_time_ms();
    int64_t base = 0;
    do {
        ifstream fin(concat_file.c_str(), ios::in);
        string f;
        while(getline(fin, f)) {
            if ((flv = srs_flv_open_read(f.c_str())) == NULL) {
                ret = 2;
                log(INFO, "open flv file %s failed. ret=%d", f.c_str(), ret);
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

            int64_t re = re_create(tools_main_entrance_startup_time);

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

    return ret;
}

int publish(srs_rtmp_t ortmp, string input_file, int recur)
{
    int ret = 0;
    srs_flv_t flv;
    if ((flv = srs_flv_open_read(input_file.c_str())) == NULL) {
        ret = 2;
        log(INFO, "open flv file failed. ret=%d", ret);
        return ret;
    }

    ret = proxy(flv, ortmp, recur);
    log(INFO, "ingest flv to RTMP completed");

    srs_rtmp_destroy(ortmp);
    srs_flv_close(flv);

    return ret;
}

int main(int argc, const char** argv)
{
    int ret = 0;
    string input_file, url, concat_file;
    int recur;

    init(argc, argv);
    input_file = config.input_file;
    concat_file = config.concat_file;
    url = config.addr;
    recur = config.recur_times;

    if (input_file.empty() && concat_file.empty()) {
        srs_human_trace("input invalid, use -i <input> or -f <concat_txt>");
        return -1;
    }
    if (url.empty()) {
        srs_human_trace("output invalid, use -y <output>");
        return -1;
    }
    if (!concat_file.empty()) {
        srs_human_trace("concat_txt: %s", concat_file.c_str());
    } else {
        srs_human_trace("input: %s", input_file.c_str());
    }
    log(INFO, "output: %s", url.c_str());
    log(INFO, "recur: %d", recur);

    srs_rtmp_t ortmp = srs_rtmp_create(url.c_str());
    // set 4s timeout
    srs_rtmp_set_timeout(ortmp, 4000, 4000);
    if (!concat_file.empty()) {
        ret = publish_concat(ortmp, concat_file, recur);
    } else {
        ret = publish(ortmp, input_file, recur);
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
    log(INFO, "start ingest flv to RTMP stream");
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
                log(INFO, "parse completed.");
                return 0;
            }
            log(INFO, "flv get packet failed. ret=%d", ret);
            return ret;
        }

        if (size <= 0) {
            log(INFO, "invalid size=%d", size);
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
            log(INFO, "irtmp get packet failed. ret=%d", ret);
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

        int64_t re = re_create(tools_main_entrance_startup_time);

        ret = do_proxy(flv, ortmp, base, re, &starttime, &timestamp);
        // for the last pulse, always sleep.
        re_cleanup(re, starttime, timestamp);
        srs_flv_lseek(flv, 0);

        recur--;
        last_time = base + timestamp;
    } while(recur > 0 && ret == 0);

    if ((ret = inject_h264_stream_eof_packet(ortmp, last_time)) != 0) {
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
        log(INFO, "ortmp simple handshake failed. ret=%d", ret);
        return ret;
    }
    log(INFO, "ortmp simple handshake success");

    if ((ret = srs_rtmp_connect_app(ortmp)) != 0) {
        log(INFO, "ortmp connect vhost/app failed. ret=%d", ret);
        return ret;
    }
    log(INFO, "ortmp connect vhost/app success");

    if ((ret = srs_rtmp_publish_stream(ortmp)) != 0) {
        log(INFO, "ortmp publish stream failed. ret=%d", ret);
        return ret;
    }
    log(INFO, "ortmp publish stream success");

    return ret;
}

