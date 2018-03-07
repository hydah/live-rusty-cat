#!/bin/sh
WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/..; pwd`

lasthour=$((`date +%s`-3600))
lasthour=`date -d@$lasthour +"%Y-%m-%d-%H"`
echo $lasthour

if [ ! -z $1 ]; then
    lasthour=$1
fi
# for rtmp-self.sh
logfile=$WORK_DIR/logs/rtmp-self.log.$lasthour
RESFILE=$WORK_DIR/result/rtmp-self.txt

echo $lasthour >> $RESFILE
sh $WORK_DIR/scripts/analyse_res.sh $logfile >> $RESFILE
if [ $? -eq 0 ]; then
    rm $logfile
fi
# for rtmp-other.sh

sh $WORK_DIR/scripts/aggr.sh $lasthour
if [ $? -eq 0 ]; then
    for file in `ls $WORK_DIR/logs/rtmp-other/*.$lasthour`; do
        rm $file
    done
fi
