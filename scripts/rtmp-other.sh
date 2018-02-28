#!/bin/bash
WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/..; pwd`
NAME=`basename $0`

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

function init() {
    cur_time=$(date -d "today" +"%Y-%m-%d %H:%M:%S")
    cur_pid=$$
    cur_log=$(run_log)

    old_pids=`ps -ef | grep 'rtmp-self.sh' | grep -v grep | awk '{print $2}'`
    if [ "x$old_pids" != "x" ]; then
        for old in $old_pids
        do
            if [ $old -eq $cur_pid ]; then
                echo -e "[$cur_time][$cur_pid] do not kill cur: old[$old] cur[$cur_pid]" >> $cur_log
            else
                kill -9 $old > /dev/null 2>&1
                echo -e "[$cur_time][$cur_pid] kill old shell: $old" >> $cur_log
            fi

        done
    fi

    old_pids=`ps -ef | grep rtmp-play| grep 'profiling?vhost=play.jcloud.com/shwj4' |  grep -v grep | awk '{print $2}'`
    if [ "x$old_pids" != "x" ]; then
        for old in $old_pids
        do
            kill -9 $old > /dev/null 2>&1
            echo -e "[$cur_time][$cur_pid] kill old task: $old" >> $cur_log
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

function get_ips() {
    HOSTNAME=`hostname`
    OP=`cat $HOSTNAME | cut -d '-' -f 2`
    IPs=`cat $WORK_DIR/resource/edge_vip.txt | grep $OP | awk '{print $2}'`
    echo $IPs
}


IPs=$(get_ips)
for ((;;)); do
    cur_log=$(run_log)
    for ip in $IPs; do
        # run 2 mins
        ${WORK_DIR}/bin/rtmp-play -t 120000 -i rtmp://$ip/profiling?vhost=play.jcloud.com/shwj4 >> ${cur_log} 2>&1
        sleep 1
    done
done
