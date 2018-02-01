PWD=`pwd`
OUTPUT="$PWD/output"
BIN="$PWD/bin"

if [ -d $OUTPUT ]; then
    rm -rf $OUTPUT
fi
mkdir $OUTPUT

if [ ! -d $BIN ]; then
    mkdir $BIN
fi


g++ -o $OUTPUT/rtmp-play rtmp-play.cpp librtmp/srs_librtmp.cpp -I librtmp
g++ -o $OUTPUT/rtmp-publish rtmp-publish.cpp librtmp/srs_librtmp.cpp -I librtmp


cp $OUTPUT/rtmp-play $BIN
