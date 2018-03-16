#!/bin/env python3
import sys
import os
import re
from influxdb import InfluxDBClient
import time

xs=[("avg_handshake", float), ("avg_connection", float), ("avg_iframe", float), ("avg_e2e", float), ("total_cnt", int), ("try_times", int), ("fail_times", int), ("nonf_cnt", int), ("avg_nf", float)]
logf=sys.argv[1]
ts=sys.argv[2]
ts_trans = time.strftime("%Y-%m-%d %H:%M:%S", time.strptime(ts, "%Y-%m-%d-%H"))
print(ts_trans)

host="10.12.132.250"
client = InfluxDBClient('localhost', 8086, 'root', '', '') # 初始化
client.create_database("live_cdn")
client.switch_database("live_cdn")

def parse_each():

    with open(logf) as f:
        name = os.path.basename(logf)[:-4]

        for line in f.readlines():
            doc = {
                    "measurement": "cperf_self",
                    "time": ts_trans,
                    "tags": {
                        "name": "",
                        },
                    "fields": {
                        "try_times": 0,
                        "fail_times": 0,
                        "avg_iframe": 0,
                        "avg_e2e": 0.0,
                        "total_cnt": 0,
                        "nonf_cnt": 0,
                        "avg_nf": 0.0
                        }
                    }
            name = line.split(':')[0]
            doc["tags"]["name"]= name.strip()
            if not 'RTMP' in name:
                continue

            for k in xs:
                pattern = r".*{} (.*?)[;|\n$]".format(k[0])
                tmp_obj = re.match(pattern, line)
                if tmp_obj:
                    doc["fields"][k[0]] = k[1](tmp_obj.group(1))
                else:
                    print("no {}".format(k[0]))

            print(doc)
            client.write_points([doc])

parse_each()
