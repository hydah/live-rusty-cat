#ifndef FLV_HPP
#define FLV_HPP

#define SRS_FLV_TAG_HEADER_SIZE 11
#define SRS_FLV_PREVIOUS_TAG_SIZE 4
enum SrsCodecFlvTag
{
    // set to the zero to reserved, for array map.
    SrsCodecFlvTagReserved = 0,

    // 8 = audio
    SrsCodecFlvTagAudio = 8,
    // 9 = video
    SrsCodecFlvTagVideo = 9,
    // 18 = script data
    SrsCodecFlvTagScript = 18,
};

#endif