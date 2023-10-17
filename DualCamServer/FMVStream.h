#pragma once

#ifndef _FULL_MOTION_VIDEO_STREAM_H_
#define _FULL_MOTION_VIDEO_STREAM_H_

#include <memory>
#include <string>

#ifndef INT64_C
# define INT64_C(c) (c ## LL)
# define UINT64_C(c) (c ## ULL)
#endif

extern "C"
{
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavformat/avformat.h"
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>

#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class FMVStream
{
public:
    struct OutputStream
    {
        OutputStream() :
            st(0),
            next_pts(0),
            samples_count(0),
            frame(0),
            sws_ctx(0)
        {
        }

        AVStream* st;

        /* pts of the next frame that will be generated */
        int64_t next_pts;
        int samples_count;

        AVFrame* frame;

        struct SwsContext* sws_ctx;
    };

    ///
    /// Constructor
    ///
    FMVStream(const char* zFilename, const int nWidth, const int nHeight);

    ///
    /// Destructor
    ///
    ~FMVStream();

    ///
    /// Frame encoder helper function
    ///
    /// Encodes a raw RGB frame into the transport stream
    ///
    int EncodeFrame(const uint8_t* frame, const timeval &tv);

    ///
    /// Frame width setter
    ///
    void setFrameWidth(int width);

    ///
    /// Frame width getter
    ///
    int getFrameWidth() const;

    ///
    /// Frame height setter
    ///
    void setFrameHeight(int height);

    ///
    /// Frame height getter
    ///
    int getFrameHeight() const;

    ///
    /// Stream address setter
    ///
    void setStreamAddress(const std::string& address);

    ///
    /// Stream address getter
    ///
    std::string getStreamAddress() const;

private:

    ///
    /// Video Stream creation
    ///
    AVStream* initVideoStream(AVFormatContext* oc);

    ///
    /// Raw frame transcoder
    ///
    /// This will convert the raw RGB frame to a raw YUV frame necessary for h.264 encoding
    ///
    void CopyFrameData(const uint8_t* src_frame);

    ///
    /// Video frame allocator
    ///
    AVFrame* AllocPicture(AVPixelFormat pix_fmt, int width, int height);

    ///
    /// Debug print helper function
    ///
    void print_sdp(AVFormatContext** avc, int n);

    ///
    /// Write the frame to the stream
    ///
    int write_frame(AVFormatContext* fmt_ctx, const AVRational* time_base, AVStream* st, AVPacket* pkt);

    ///
    /// initialize the frame data
    ///
    void initFrame();

    // formatting data needed for output streaming and the output container (MPEG 2 TS)
    AVOutputFormat* format;
    AVFormatContext* format_ctx;

    // structure container for our video stream
    OutputStream stream;

    AVIOContext* io_ctx;

    std::string streamFilename;

    int frameWidth;
    int frameHeight;
};

#endif

