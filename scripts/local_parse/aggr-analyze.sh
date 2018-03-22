#!/bin/sh
# $1 每小时聚合的rtmp-other数据 tgz
# $2 分析结果存放目录

WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/../..; pwd`
RES_DIR=$2
SUM_RES_FILE=$RES_DIR/all.log
NEAR_RES_FILE=$RES_DIR/nearest.log
FAST_RES_FILE=$RES_DIR/fastest.log

HOSTNAME=`hostname`

NAME=`basename $1`
TMP_DIR=/tmp/${NAME%.*}

function clr() {
    mkdir -p $RES_DIR
    rm $SUM_RES_FILE
    rm $NEAR_RES_FILE
    rm $FAST_RES_FILE
    rm $RES_DIR/*VIP.log
}

function get_vips() {
    VIPs=`cat $WORK_DIR/resource/edge_vip.json | grep VIP | cut -d '"' -f 2`
    echo ${VIPs}
}

function cac() {
    file=$1
    resfile=$2
    name=$3
    tmpfile="/tmp/live-rusty-cat-result-`date +%s`"
    fail_times=`grep 'first_itime 0' $1 | wc -l`
    grep 'address' $1  | grep -v 'first_itime 0' > $tmpfile
    suc_times=`cat $tmpfile | wc -l`

    avg_handshake=0
    avg_connection=0
    avg_iframe=0
    avg_e2e=0
    run_time=0
    total_time=0
    wait_time=0

    if [ $suc_times -gt 0 ]; then
        # grep -Po 'connection_time.*?(,|$)' | grep -Eo  '[0-9]+'
        # address rtmp://play.jcloud.com/profiling/avatar, handshake_time 63, connection_time 334, firstItime 399, total_count 6, nonfluency_count 0, nonfluency_rate 0.00, sei_frame_count 4, e2e 202, e2relay 168, e2edge 42
        avg_handshake=$(cat $tmpfile | grep -Po 'handshake_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum/NR}')
        avg_connection=$(cat $tmpfile | grep -Po 'connection_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum/NR}')
        avg_iframe=$(cat $tmpfile | grep -Po '(first_itime|firstItime).*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum/NR}')
        avg_e2e=$(cat $tmpfile | grep -Po 'e2e.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum/NR}')
        run_time=$(cat $tmpfile | grep -Po 'run_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum/NR}')
        total_time=$(cat $tmpfile | grep -Po 'total_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum/NR}')
        wait_time=$(cat $tmpfile | grep -Po 'waiting_time.*?(,|$)' | grep -Eo '[0-9]+' | awk '{sum+=$1} END {if (NR==0) print 0; else print sum/NR}')
    fi
    wait_rate=$(echo "$run_time $wait_time" | awk '{if ($1==0) print 100; else print $2/$1}')

    echo -n `hostname`:
    echo -n ' try_times '$(($fail_times+$suc_times))
    echo -n '; fail_times '$fail_times
    echo -n '; avg_handshake '$avg_handshake
    echo -n '; avg_connection '$avg_connection
    echo -n '; avg_iframe '$avg_iframe
    echo -n '; avg_e2e '$avg_e2e
    echo -n '; total_time '$total_time
    echo -n '; run_time '$run_time
    echo -n '; waiting_time '$wait_time
    echo    '; waiting_rate '$wait_rate
}

function get_nearest(){
    op=$1
    name=$2
    nearest=`python2 $WORK_DIR/scripts/nearest.py $WORK_DIR/resource/edge_vip.json $name $op | cut -d '-' -f 1`
    echo $nearest
}

clr
rm -rf $TMP_DIR
tar xf $1 -C /tmp

pushd $TMP_DIR
for zf in `ls $TMP_DIR/*.tgz`; do
    tar xf $zf
done
VIPs=($(get_vips))

for ((i=0; i<${#VIPs[@]}; i=i+1)); do
    vip_name=${VIPs[$i]}
    echo $vip_name
    name=`echo $vip_name | cut -d '-' -f 1`
    op=`echo $vip_name | cut -d '-' -f 2`
    nearest=($(get_nearest $op $name))

    tmp="/tmp/live-rusty-cat-res-$vip_name-other.log"
    cat `find $TMP_DIR -name "*$vip_name*"` > $tmp
    resfile=$RES_DIR/$vip_name.log
    $(cac $tmp $resfile "$vip_name"_All)
    echo "=====" >> $resfile

    #split
    nearest_file="/tmp/live-rusty-cat-res-$vip_name-nearest.log"
    echo "" > $nearest_file
    for f in `find $TMP_DIR -name "*$vip_name*"`; do
        tmp_name=`dirname $f`
        tmp_name=`basename $tmp_name`
        oname=`echo $tmp_name | cut -d '-' -f 2`

        if echo "${nearest[@]}" | grep -w "$oname" &> /dev/null; then
            cat $f >> $nearest_file
        fi
        $(cac $f $resfile $tmp_name)
    done
    echo "====" >> $resfile
    echo "nearest:" ${nearest[@]} >> $resfile
    $(cac $nearest_file $resfile "$vip_name"_Nearest)

    echo "" > $nearest_file
    fastest=(`grep 'aggr' $resfile | tr -s [:space:] | sort -n -k 7 -t ' ' | head -4 | cut -d ':' -f 1`)
    for f in `find $TMP_DIR -name "*$vip_name*"`; do
        tmp_name=`dirname $f`
        tmp_name=`basename $tmp_name`
        if echo "${fastest[@]}" | grep -w "$tmp_name" &> /dev/null; then
            cat $f >> $nearest_file
        fi
    done
    echo "====" >> $resfile
    echo "fastest:" ${fastest[@]} >> $resfile
    $(cac $nearest_file $resfile "$vip_name"_Fastest)


    # summary
    grep $vip_name $resfile | sed s/.*All/$vip_name/ >> $SUM_RES_FILE
    grep "Nearest" $resfile | sed s/.*Nearest/$vip_name/ >> $NEAR_RES_FILE
    grep "Fastest" $resfile | sed s/.*Fastest/$vip_name/ >> $FAST_RES_FILE
done
