#!/bin/sh
WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/..; pwd`
RES_FILE=$WORK_DIR/rtmp-other-res.log-$(date -d "today" +"%Y-%m-%d-%H")
#$1 is the tar file

HOSTNAME=`hostname`

NAME=`basename $1`
DEST_DIR=/tmp/$NAME

tar xzvf $1 -C /tmp

pushd $DEST_DIR

for zf in `ls $DEST_DIR/*.tgz`; do
    tar xf $zf
done

function get_ops() {
    HOSTNAME=`hostname`
    OP=`echo $HOSTNAME | cut -d '-' -f 2`
    OPs=`cat $WORK_DIR/resource/edge_vip.txt | grep $OP`
    echo ${OPs}
}
OPs=($(get_ops))

function cac() {
    file=$1
    resfile=$2
    name=$3
    tmpfile="/tmp/live-rusty-cat-result-`date +%s`"
    fail_times=`grep 'total_count 0' $1 | wc -l`
    grep 'address' $1 | grep -v 'total_count 0' > $tmpfile
    suc_times=`cat $tmpfile | wc -l`

    avg_e2e=$(cat $tmpfile | cut -d ',' -f 7  | grep -v '^$' | cut -d ' ' -f 3 | awk '{sum+=$1} END {print sum/NR}')
    avg_iframe=$(cat $tmpfile | cut -d ',' -f 2  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum/NR}')
    total_cnt=$(cat $tmpfile | cut -d ',' -f 3  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum}')
    nonf_cnt=$(cat $tmpfile | cut -d ',' -f 4  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum}')
    avg_nf=0.0
    if [ $total_cnt -gt 0 ]; then
        avg_nf=$(echo "$total_cnt $nonf_cnt" | awk '{print $2/$1}')
    fi
    echo -n $name: >> $resfile
    echo -n ' try_times ' $(($fail_times+$suc_times)) >>$resfile
    echo -n '; fail_times ' $fail_times >>$resfile
    echo -n '; avg_iframe ' $avg_iframe >>$resfile
    echo -n '; avg_e2e ' $avg_e2e >>$resfile
    echo -n '; total_cnt ' $total_cnt >>$resfile
    echo -n '; nonf_cnt ' $nonf_cnt >>$resfile
    echo    '; avg_nf ' $avg_nf >>$resfile
}

for ((i=0; i<${#OPs[@]}; i=i+2)); do
    next=$(($i+1))
    name=${OPs[$i]}
    ip=${OPs[$next]}
    tmpfile="/tmp/live-rusty-cat-res-$name-other.log"
    cat `find . -name "*$name*"` > $tmpfile
    cac $tmpfile $RES_FILE $name
done


