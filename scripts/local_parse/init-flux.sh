#!/bin/sh
WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/../..; pwd`
DATE=$1
LOGFILE=$WORK_DIR/result/live-rusty-cat-res-$DATE.tgz

if [ ! -f $LOGFILE ]; then
    echo "no tgz file"
    exit 1
fi

LOGDIR=$WORK_DIR/result/live-rusty-cat-res-$DATE

tar xf $LOGFILE

function parse_rtmp_other() {
    for f in `ls $LOGDIR/*.tgz`; do
        ts=`echo $f | awk -F 'other-' '{print $2}' | awk -F '.' '{print $1}'`
        resdir=$LOGDIR/rtmp-other-parse-$ts
        sh $WORK_DIR/scripts/local_parse/aggr-analyze.sh $f $resdir
        python3 $WORK_DIR/scripts/local_parse/rtmp-other-influx.py $resdir $ts
    done
}

function parse_rtmp_self() {
    for f in `ls $LOGDIR/rtmp-self*`; do
        ts=`echo $f | awk -F 'rtmp-self-' '{print $2}'`
        echo $ts
        python3 $WORK_DIR/scripts/local_parse/rtmp-self-influx.py $f $ts
    done
}

parse_rtmp_other
parse_rtmp_self



