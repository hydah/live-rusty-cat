cmake_minimum_required (VERSION 2.8)

project (RustyCat)

include_directories(libs cli)

set(
    LIB_SOURCES

    libs/http_parser.c
    libs/rtmp_tool.cpp

    libs/logs/log.cpp

    libs/app/hdl.cpp

    libs/basic/io.cpp
    libs/basic/file.cpp
    libs/basic/buffer.cpp

    libs/container/flv.cpp

    libs/protocol/http.cpp
    libs/protocol/rtmp.cpp
)

add_library (rtmp ${LIB_SOURCES})

add_executable(output/rtmp-play rtmp-play.cpp )
target_link_libraries (output/rtmp-play  rtmp)
add_executable(output/rtmp-publish rtmp-publish.cpp)
target_link_libraries (output/rtmp-publish  rtmp)