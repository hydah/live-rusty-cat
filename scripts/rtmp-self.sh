#!/bin/bash
WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/..; pwd`
NAME=`basename $0`
HOSTNAME=`hostname`

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

    old_pids=`ps -ef | grep 'rtmp-self.sh' | grep -v 'grep' | awk '{print $2}'| uniq | grep -v $cur_pid`

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

function publish() {
    for ((;;)); do
     ${WORK_DIR}/bin/rtmp-publish -i ${WORK_DIR}/video/avatar.flv -y rtmp://origin.jcloud.com/profiling?vhost=push.jcloud.com/avatar-${HOSTNAME} &>/dev/null
    done
}

publish &


sleep 5

for ((;;)); do
    cur_log=$(run_log)
    # run 2 mins
    ${WORK_DIR}/bin/rtmp-play -t 120000 -i rtmp://127.0.0.1/profiling?vhost=play.jcloud.com/avatar-${HOSTNAME} >> ${cur_log} 2>&1
    sleep 10
done
