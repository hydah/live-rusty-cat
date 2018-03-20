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


g++ -o $OUTPUT/rtmp-play rtmp-play.cpp rtmp-tool.cpp librtmp/srs_librtmp.cpp -I librtmp
#g++ -o $OUTPUT/rtmp-ts rtmp-play-timestamp.cpp librtmp/srs_librtmp.cpp -I librtmp
g++ -o $OUTPUT/rtmp-publish rtmp-publish.cpp librtmp/srs_librtmp.cpp -I librtmp

chmod +x $OUTPUT/rtmp-play
chmod +x $OUTPUT/rtmp-publish
cp $OUTPUT/rtmp-play $BIN
cp $OUTPUT/rtmp-publish $BIN

rm live-rusty-cat.tgz
mkdir live-rusty-cat
cp -rf bin/ live-rusty-cat
cp -rf logs/ live-rusty-cat
cp -rf video/ live-rusty-cat
cp -rf resource/ live-rusty-cat
cp -rf scripts/ live-rusty-cat
mkdir -p live-rusty-cat/result
rm -rf live-rusty-cat/logs/*
tar czvf live-rusty-cat.tgz live-rusty-cat
rm -rf live-rusty-cat
#tar czvf live-rusty-cat.tgz ../live-rusty-cat/bin ../live-rusty-cat/logs ../live-rusty-cat/video ../live-rusty-cat/scripts --exclude ../live-rusty-cat/logs/* ../live-rusty-cat/resource
