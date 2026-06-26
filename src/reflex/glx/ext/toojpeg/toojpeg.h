#pragma once

//this is a modified version of TooJpeg

namespace TooJPEG
{
    typedef unsigned char UInt8;
    typedef unsigned short UInt16;
    typedef unsigned int UInt32;

    typedef void(*Output)(void * client, const UInt8 * data, UInt32 n);

    struct Options
    {
        UInt8 quality = 90;
        const char * comment = 0;
        UInt32 comment_length = 0;
    };

    bool EncodeBitmap(const UInt8 * pixels, UInt32 width, UInt32 height, UInt8 nchannel, Options options, void * client, Output output);
}
