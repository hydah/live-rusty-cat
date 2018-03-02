#简单处理打包

WORK_DIR=`dirname $0`
WORK_DIR=`cd ${WORK_DIR}/..; pwd`
NAME=`basename $0`
HOSTNAME=`hostname`

LOG_DIR=$WORK_DIR/logs/rtmp-other
CUR_TIME=$(date -d "today" +"%Y-%m-%d-%H")
CUR_TIME=$1

DEST_DIR=${NAME%.*}-$HOSTNAME-$CUR_TIME
rm -rf /tmp/$DEST_DIR
mkdir -p /tmp/$DEST_DIR


pushd $LOG_DIR
for file in `ls *.$CUR_TIME`; do
    name=`basename $file`
    grep 'address' $file > /tmp/$DEST_DIR/$name
done
popd

pushd /tmp
tar czvf $WORK_DIR/$DEST_DIR.tgz $DEST_DIR
popd
