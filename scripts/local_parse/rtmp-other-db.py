#!/usr/bin/env python3

import pymongo
import sys
import os
import re

#doc = {"time": "", "name": "", "avg_iframe": "", "avg_e2e": "", "total_cnt": "", "try_times": "", "fail_times": "", "nonf_cnt": "", "avg_nf": ""}
xs=[("avg_handshake", float), ("avg_connection", float), ("avg_iframe", float), ("avg_e2e", float), ("duration", int), ("try_times", int), ("fail_times", int), ("waiting_time", int), ("waiting_rate", float)]
directory=sys.argv[1]
ts=sys.argv[2]

def get_name(line):
    tmp_obj = re.match(r'(.*?): .*', line)
    if tmp_obj:
        return tmp_obj.group(1)
    else:
        return "unkown"

def parse_all():
    cperf_all = live_db.c_perf_all
    with open("{}/all.log".format(directory)) as f:
        for line in f.readlines():
            name = get_name(line)

            doc = {"time": "2018-03-05-17", "name": "", "avg_iframe": 0, "avg_e2e": 0.0, "duration": 0, "try_times": 0, "fail_times": 0, "waiting_time": 0, "waiting_rate": 0.0}
            for k in xs:
                pattern = r".*{} (.*?)[;|\n$]".format(k[0])
                tmp_obj = re.match(pattern, line)
                if tmp_obj:
                    doc[k[0]] = k[1](tmp_obj.group(1))
                else:
                    print("no {}".format(k[0]))
            doc["name"] = name
            print(doc)
            cperf_all.insert_one(doc)


def parse_fastest():
    cperf_fast = live_db.c_perf_fast
    with open("{}/fastest.log".format(directory)) as f:
        for line in f.readlines():
            name = get_name(line)

            doc = {"time": "2018-03-05-17", "name": "", "avg_iframe": 0, "avg_e2e": 0.0, "total_cnt": 0, "try_times": 0, "fail_times": 0, "nonf_cnt": 0, "avg_nf": 0.0}
            for k in xs:
                pattern = r".*{} (.*?)[;|\n$]".format(k[0])
                tmp_obj = re.match(pattern, line)
                if tmp_obj:
                    doc[k[0]] = k[1](tmp_obj.group(1))
                else:
                    print("no {}".format(k[0]))
            doc["name"] = name
            print(doc)
            cperf_fast.inser_one(doc)


host="10.12.132.250"
client = pymongo.MongoClient(host=host, port=27017)
live_db = client.live_cdn
cperf_all = live_db.c_perf_all
cperf_fast = live_db.c_perf_fast
cperf_split = live_db.c_perf_split

def parse_each():

    for sub in os.listdir(directory):
        path = os.path.join(directory, sub)
        if os.path.isfile(path) and "VIP" in path:
            with open(path) as f:
                name = os.path.basename(path)[:-4]

                for line in f.readlines():
                    doc = {"name": "", "source": "", "time": ts, "try_times": 0, "fail_times": 0, "avg_iframe": 0, "avg_e2e": 0.0, "duration": 0, "waiting_time": 0, "waiting_rate": 0.0}
                    first = line.split(':')[0]
                    if not 'RTMP' in first:
                        continue

                    for k in xs:
                        pattern = r".*{} (.*?)[;|\n$]".format(k[0])
                        tmp_obj = re.match(pattern, line)
                        if tmp_obj:
                            doc[k[0]] = k[1](tmp_obj.group(1))
                        else:
                            print("no {}".format(k[0]))


                    if "All" in line:
                        doc["name"] = name
                        tmp_doc = doc.copy()
                        tmp_doc.pop("source", None)
                        print("all", tmp_doc)

                        cperf_all.insert_one(tmp_doc)
                        cperf_all.ensure_index([('name', pymongo.ASCENDING), ("time", pymongo.DESCENDING)], unique=True)
                    elif "Fastest" in line:
                        doc["name"] = name
                        tmp_doc = doc.copy()
                        tmp_doc.pop("source", None)
                        print("fastest", tmp_doc)
                        cperf_fast.insert_one(tmp_doc)
                        cperf_fast.ensure_index([('name', pymongo.ASCENDING), ("time", pymongo.DESCENDING)], unique=True)
                    elif "aggr" in line:
                        doc["name"] = name
                        doc["source"] = line.split(":")[0][5:-14]
                        print("split", doc)
                        cperf_split.insert_one(doc)
                        cperf_split.ensure_index([('name', pymongo.ASCENDING), ('source', pymongo.ASCENDING), ("time", pymongo.DESCENDING)], unique=True)

parse_each()
