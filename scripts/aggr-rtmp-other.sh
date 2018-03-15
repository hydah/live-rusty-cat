#简单处理打包

WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/..; pwd`
NAME=`basename $0`
HOSTNAME=`hostname`

LOG_DIR=$WORK_DIR/logs/rtmp-other
CUR_TIME=$(date -d "today" +"%Y-%m-%d-%H")
CUR_TIME=$1

DEST_DIR=${NAME%.*}_${HOSTNAME}_${CUR_TIME}
rm -rf /tmp/$DEST_DIR
mkdir -p /tmp/$DEST_DIR


pushd $LOG_DIR
for file in `ls *.$CUR_TIME`; do
    name=`basename $file`
    grep 'address' $file > /tmp/$DEST_DIR/$name
done
popd

if ls $LOG_DIR/*.$CUR_TIME; then
    pushd /tmp
    tar czvf $WORK_DIR/result/$DEST_DIR.tgz $DEST_DIR
    popd
fi
