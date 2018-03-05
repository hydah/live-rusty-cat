#!/bin/env python
import sys
import json



def main():
    res = sys.argv[1]
    name = sys.argv[2]
    op = sys.argv[3]
    edge_vip = {}
    with open(res) as f:
        content = f.read()
        edge_vip = json.loads(content)

    self = {}
    nerb = {}
    for k, v in edge_vip[op].items():
        loc = k.find("-")
        tmp_name = k[:loc]
        if  tmp_name == name:
            self = {"loc": edge_vip["LOC"][tmp_name], "name": k}
        else:
            nerb[tmp_name] = {"loc": edge_vip["LOC"][tmp_name], "name": k}

    def sortf(item):
        dx = item[1]["loc"][0]
        dy = item[1]["loc"][1]
        dist = pow(dx-self["loc"][0], 2) + pow(dy-self["loc"][1], 2)
        return dist

    a = sorted(nerb.items(), key = sortf)
    l = min(len(a), 3)
    i = 0
    while(i < l):
        name = a[i][1]["name"]
        print("%s %s" %(name, edge_vip[op][name]["ip"]))
        i += 1
    print("%s %s" %(self["name"], edge_vip[op][self["name"]]["ip"]))


if __name__ == "__main__":
    main()


