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
#g++ -o $OUTPUT/rtmp-ts rtmp-play-timestamp.cpp librtmp/srs_librtmp.cpp -I librtmp
g++ -o $OUTPUT/rtmp-publish rtmp-publish.cpp librtmp/srs_librtmp.cpp -I librtmp


cp $OUTPUT/rtmp-play $BIN
cp $OUTPUT/rtmp-publish $BIN

rm live-rusty-cat.tgz
mkdir live-rusty-cat
cp -rf bin/ live-rusty-cat
cp -rf logs/ live-rusty-cat
cp -rf video/ live-rusty-cat
cp rtmp-ts.sh live-rusty-cat
#tar czvf live-rusty-cat.tgz live-rusty-cat
rm -rf live-rusty-cat
tar czvf live-rusty-cat.tgz ../live-rusty-cat/bin ../live-rusty-cat/logs ../live-rusty-cat/video ../live-rusty-cat/scripts ../live-rusty-cat/resource
