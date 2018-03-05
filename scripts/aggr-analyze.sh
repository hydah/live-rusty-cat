#!/bin/sh
WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/..; pwd`
RES_DIR=$WORK_DIR/rtmp-other-$(date -d "today" +"%Y-%m-%d-%H")
rm -rf $RES_DIR
mkdir -p $RES_DIR
SUM_RES_FILE=$RES_DIR/all.log
NEAR_RES_FILE=$RES_DIR/nearest.log
#$1 is the tar file

HOSTNAME=`hostname`

NAME=`basename $1`
DEST_DIR=/tmp/${NAME%.*}


function get_vips() {
    VIPs=`cat $WORK_DIR/resource/edge_vip.json | grep VIP | cut -d '"' -f 2`
    echo ${VIPs}
}

function cac() {
    file=$1
    resfile=$2
    name=$3
    tmpfile="/tmp/live-rusty-cat-result-`date +%s`"
    fail_times=`grep 'total_count 0' $1 | wc -l`
    grep 'address' $1 | grep -v 'total_count 0' > $tmpfile
    suc_times=`cat $tmpfile | wc -l`

    avg_e2e=0
    avg_iframe=0
    total_cnt=0
    nonf_cnt=0
    if [ $suc_times -gt 0 ]; then
        avg_e2e=$(cat $tmpfile | cut -d ',' -f 7  | grep -v '^$' | cut -d ' ' -f 3 | awk '{sum+=$1} END {print sum/NR}')
        avg_iframe=$(cat $tmpfile | cut -d ',' -f 2  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum/NR}')
        total_cnt=$(cat $tmpfile | cut -d ',' -f 3  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum}')
        nonf_cnt=$(cat $tmpfile | cut -d ',' -f 4  | grep -v '^$' |cut -d ' '  -f 3 | awk '{sum+=$1} END {print sum}')
    fi
    if [ -z "$total_cnt" ]; then
        total_cnt=0
    fi
    if [ -z "$nonf_cnt" ]; then
        nonf_cnt=0
    fi
    if [ -z "$avg_e2e" ]; then
        avg_e2e=0
    fi
    if [ -z "$avg_iframe" ]; then
        avg_iframe=0
    fi
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

function get_nearest(){
    op=$1
    name=$2
    nearest=`python2 $WORK_DIR/scripts/nearest.py $WORK_DIR/resource/edge_vip.json $name $op | cut -d '-' -f 1`
    echo $nearest
}

rm -rf $DEST_DIR
tar xf $1 -C /tmp
pushd $DEST_DIR

for zf in `ls $DEST_DIR/*.tgz`; do
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
    cat `find . -name "*$vip_name*"` > $tmp
    resfile=$RES_DIR/$vip_name.log
    $(cac $tmp $resfile $vip_name)
    echo "=====" >> $resfile

    #split
    nearest_file="/tmp/live-rusty-cat-res-$vip_name-nearest.log"
    echo "" > $nearest_file
    for f in `find . -name "*$vip_name*"`; do
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
    $(cac $nearest_file $resfile "$name-$op-Nearest")

    # summary
    grep $vip_name $resfile >> $SUM_RES_FILE
    grep "Nearest" $resfile >> $NEAR_RES_FILE
done
