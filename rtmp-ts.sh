#!/bin/bash


function rand() {
    min=$1
    max=$(($2-$min+1))
    num=$(($RANDOM+1000001))
    echo $(($num%$max+$min))
}

run_log=./logs/run.log.$(date -d "today" +"%Y-%m-%d-%H")

cur_time=$(date -d "today" +"%Y-%m-%d %H:%M:%S")

cur_pid=$$

old_pids=`ps -ef | grep 'rtmp-ts.sh' | grep -v grep | awk '{print $2}'`
if [ "x$old_pids" != "x" ]; then
    for old in $old_pids
    do
        if [ $old -eq $cur_pid ]; then
            echo -e "[$cur_time][$cur_pid] do not kill cur: old[$old] cur[$cur_pid]" >> $run_log
        else
            kill -9 $old > /dev/null 2>&1
            echo -e "[$cur_time][$cur_pid] kill old shell: $old" >> $run_log
        fi

    done
fi

old_pids=`ps -ef | grep rtmp-publish | grep -v grep | awk '{print $2}'`
if [ "x$old_pids" != "x" ]; then
    for old in $old_pids
    do
        kill -9 $old > /dev/null 2>&1
        echo -e "[$cur_time][$cur_pid] kill old task: $old" >> $run_log
    done
fi

old_pids=`ps -ef | grep rtmp-play | grep -v grep | awk '{print $2}'`
if [ "x$old_pids" != "x" ]; then
    for old in $old_pids
    do
        kill -9 $old > /dev/null 2>&1
        echo -e "[$cur_time][$cur_pid] kill old task: $old" >> $run_log
    done
fi

if [ "x$1" == "xkill" ]; then
    echo -e "[$cur_time][$cur_pid] only check and kill old task, then exit" >> $run_log
    exit
fi

rand_sleep=$(rand 0 10)
echo -e "[$cur_time][$cur_pid] sleep $rand_sleep" >> $run_log
sleep $rand_sleep
cur_time=$(date -d "today" +"%Y-%m-%d %H:%M:%S")
echo -e "[$cur_time][$cur_pid] after sleep " >> $run_log

grep 'play.jcloud.com' /etc/hosts > /dev/null
if [ $? -eq 0 ]; then
    sed -i 's/.*play.jcloud.com/127.0.0.1 play.jcloud.com/' /etc/hosts
else
    echo -e "[$cur_time][$cur_pid] 127.0.0.1 play.jcloud.com" >> /etc/hosts
fi


echo -e "\n[$cur_time][$cur_pid] set local /etc/hosts for test" >> $run_log
cat /etc/hosts >> $run_log

NAME=`hostname`
function publish() {
    for ((;;)); do
     ./bin/rtmp-publish -i ./video/avatar.flv -y rtmp://origin.jcloud.com/profiling?vhost=push.jcloud.com/avatar-${NAME} &>/dev/null
    done
}

publish &


sleep 5

for ((;;)); do
    ./bin/rtmp-play rtmp://play.jcloud.com/profiling/avatar-${NAME} >> $run_log 2>&1
    sleep 10
done


sed -i '/.*play.jcloud.com/d' /etc/hosts

echo -e "\n[$cur_time][$cur_pid] clear local /etc/hosts after test" >> $run_log
cat /etc/hosts >> $run_log
