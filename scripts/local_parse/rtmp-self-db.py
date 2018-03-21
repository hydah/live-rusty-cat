#!/bin/env python3
import pymongo
import sys
import os
import re

xs=[("avg_handshake", float), ("avg_connection", float), ("avg_iframe", float), ("avg_e2e", float), ("duration", int), ("try_times", int), ("fail_times", int), ("waiting_time", int), ("waiting_rate", float)]
logf=sys.argv[1]
ts=sys.argv[2]

host="10.12.132.250"
client = pymongo.MongoClient(host=host, port=27017)
live_db = client.live_cdn
cperf_self = live_db.c_perf_self

def parse_each():

    with open(logf) as f:
        name = os.path.basename(logf)[:-4]

        for line in f.readlines():
            doc = {"name": "", "source": "", "time": ts, "try_times": 0, "fail_times": 0, "avg_iframe": 0, "avg_e2e": 0.0, "duration": 0, "waiting_time": 0, "waiting_rate": 0.0}
            name = line.split(':')[0]
            doc["name"]= name.strip()
            if not 'RTMP' in name:
                continue

            for k in xs:
                pattern = r".*{} (.*?)[;|\n$]".format(k[0])
                tmp_obj = re.match(pattern, line)
                if tmp_obj:
                    doc[k[0]] = k[1](tmp_obj.group(1))
                else:
                    print("no {}".format(k[0]))

            print(doc)
            cperf_self.insert_one(doc)
            cperf_self.ensure_index([('name', pymongo.ASCENDING), ("time", pymongo.DESCENDING)], unique=True)

parse_each()
