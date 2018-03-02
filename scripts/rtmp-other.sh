#!/bin/bash

### 会通过hostname知道自己是哪个节点和哪个运营商
### 从edge_vip ip 库中找到同个运营商的其它节点vip，然后同时播放同一路流
### 每个节点的探测结果输出到自己的日志文件中，那么对于一个节点，运行一段时间后，
### 会有通过其他节点播放的结果日志，假如名字为AH-CM-rtmp-other-2018-02-28-17为在该节点通过AH-CM
### 播放流在2018-02-28-17时的日志文件。

### 在日志收集汇总时，会将所有的2018-02-28-17的日志简单处理（只拿summary）收集下来。

WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/..; pwd`
NAME=`basename $0`
mkdir -p $WORK_DIR/logs/rtmp-other

function rand() {
    min=$1
    max=$(($2-$min+1))
    num=$(($RANDOM+1000001))
    echo $(($num%$max+$min))
}

function run_log() {
    file=${NAME%.*}.log.$(date -d "today" +"%Y-%m-%d-%H")
    log=$WORK_DIR/logs/$file
    echo $log
}

function run_vip_log() {
    vip_name=$1
    file=${NAME%.*}-${vip_name}.log.$(date -d "today" +"%Y-%m-%d-%H")
    log=$WORK_DIR/logs/rtmp-other/$file
    echo $log
}

function get_cids() {
    cids=''
    temp=''
    for old in $@; do
        temp=`ps -ef | awk '{if('$old' == $3){print $2}}'`
        cids=`echo $cids $temp`
    done
    echo $cids
}

function init() {
    cur_time=$(date -d "today" +"%Y-%m-%d %H:%M:%S")
    cur_pid=$$
    cur_log=$(run_log)

    old_pids=`ps -ef | grep 'rtmp-other.sh' | grep -v 'grep' | awk '{print $2}'| uniq | grep -v $cur_pid`

    cids=$(get_cids "$old_pids")

    if [ "x$old_pids" != "x" ]; then
        for old in $old_pids; do
            kill -9 $old > /dev/null 2>&1
            echo -e "[$cur_time][$cur_pid] kill old shell: $old" >> $cur_log
        done
    fi

    if [ "x$cids" != "x" ]; then
        echo -e "killing all child pids..." >> $cur_log
        for pid in $cids; do
            if [ "x$pid" == "x$cur_pid" ]; then
                echo -e "['$cur_time][$cur_pid] not kill self" >> $cur_log
            else
                kill -9 $pid > /dev/null 2>&1
                echo -e "[$cur_time][$cur_pid] kill old child shell: $pid" >> $cur_log
            fi
        done
    fi


    if [ "x$1" == "xkill" ]; then
        echo -e "[$cur_time][$cur_pid] only check and kill old task, then exit" >> $cur_log
        exit
    fi

    rand_sleep=$(rand 0 10)
    echo -e "[$cur_time][$cur_pid] sleep $rand_sleep" >> $cur_log
    sleep $rand_sleep
    cur_time=$(date -d "today" +"%Y-%m-%d %H:%M:%S")
    echo -e "[$cur_time][$cur_pid] after sleep " >> $cur_log
}

init $1
sleep 2

function get_ops() {
    HOSTNAME=`hostname`
    OP=`echo $HOSTNAME | cut -d '-' -f 2`
    OPs=`cat $WORK_DIR/resource/edge_vip.txt | grep $OP`
    echo ${OPs}
}


OPs=($(get_ops))

function do_sniff() {
    ##$1: vip name
    ##$2: vip ip
    for ((;;)); do
        cur_log=$(run_vip_log $1)
        ${WORK_DIR}/bin/rtmp-play -t 120000 -i rtmp://$2/profiling?vhost=play.jcloud.com/shwj4 >> ${cur_log} 2>&1
        sleep 10
    done

}

for ((i=0; i<${#OPs[@]}; i=i+2)); do
    next=$(($i+1))
    do_sniff ${OPs[$i]} ${OPs[$next]} &
done

for ((;;)); do
    sleep 2
done
