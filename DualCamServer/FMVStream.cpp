#include "FMVStream.h"

#include <stdexcept>
#include <iostream>

FMVStream::FMVStream(const char *zFilename, const int nWidth, const int nHeight)
    : format(0),
    format_ctx(0),
    stream(),
    io_ctx(0),
    streamFilename(zFilename),
    frameWidth(nWidth),
    frameHeight(nHeight)
{
    // Register all formats and codecs
    av_register_all();
    avcodec_register_all();

    // Init networking
    avformat_network_init();

    // Find format
    format = av_guess_format("mpegts", NULL, NULL);

    // allocate the AVFormatContext
    format_ctx = avformat_alloc_context();

    if (!format_ctx)
    {
        throw std::runtime_error("avformat_alloc_context failed");
    }

    format_ctx->oformat = format;
    //sprintf_s(format_ctx->filename, sizeof(format_ctx->filename), "%s", streamFilename.c_str());

    stream.st = initVideoStream(format_ctx);

    initFrame();

    // Allocate AVIOContext
    int ret = avio_open(&io_ctx, streamFilename.c_str(), AVIO_FLAG_WRITE);

    if (ret != 0)
    {
        throw std::runtime_error("avio_open failed");
    }

    format_ctx->pb = io_ctx;

    // Print some debug info about the format
    av_dump_format(format_ctx, 0, NULL, 1);

    // Begin the output by writing the container header
    avformat_write_header(format_ctx, NULL);

//    AVFormatContext* ac[] = { format_ctx };
//    print_sdp(ac, 1);
}

FMVStream::~FMVStream()
{
    AVCodecContext* c = stream.st->codec;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    for (;;)
    {
        avcodec_send_frame(c, NULL);
        if (avcodec_receive_packet(c, &pkt) == 0)
        {
            av_interleaved_write_frame(format_ctx, &pkt);
            av_packet_unref(&pkt);
        }
        else
        {
            break;
        }
    }

    av_write_trailer(format_ctx);
    avcodec_close(stream.st->codec);

    avio_close(io_ctx);

    avformat_free_context(format_ctx);

    if (stream.frame)
    {
        av_frame_free(&stream.frame);
    }
  //  av_free(format);
}

AVFrame* FMVStream::AllocPicture(AVPixelFormat pix_fmt, int width, int height)
{
    // Allocate a frame
    AVFrame* frame = av_frame_alloc();

    if (frame == nullptr)
    {
        throw std::runtime_error("avcodec_alloc_frame failed");
    }

    if (av_image_alloc(frame->data, frame->linesize, width, height, pix_fmt, 1) < 0)
    {
        throw std::runtime_error("av_image_alloc failed");
    }

    frame->width = width;
    frame->height = height;
    frame->format = pix_fmt;

    return frame;
}

void FMVStream::print_sdp(AVFormatContext** avc, int n)
{
    char sdp[2048];
    av_sdp_create(avc, n, sdp, sizeof(sdp));
    printf("SDP:\n%s\n", sdp);
    fflush(stdout);
}

AVStream* FMVStream::initVideoStream(AVFormatContext* oc)
{
    AVStream* st = avformat_new_stream(oc, NULL);

    if (st == nullptr)
    {
        std::runtime_error("Could not alloc stream");
    }

    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);

    if (codec == nullptr)
    {
        throw std::runtime_error("couldn't find mpeg2 encoder");
    }

    st->codec = avcodec_alloc_context3(codec);

    st->codec->codec_id = AV_CODEC_ID_H264;
    st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codec->bit_rate = 400000;
    st->codec->time_base.num = 1;
    st->codec->time_base.den = 30;

    st->codec->width = frameWidth;
    st->codec->height = frameHeight;

    st->time_base.num = 1;
    st->time_base.den = 30;

    st->codec->framerate.num = 1;
    st->codec->framerate.den = 30;

    st->codec->max_b_frames = 2;
    st->codec->gop_size = 12;
    st->codec->pix_fmt = AV_PIX_FMT_YUV420P;

    st->id = oc->nb_streams - 1;

    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
    {
        st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // option setup for the codec
    av_opt_set(st->codec->priv_data, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);

    if (avcodec_open2(st->codec, codec, NULL) < 0)
    {
        throw std::runtime_error("avcodec_open failed");
    }

    return st;
}

void FMVStream::initFrame()
{
    // Allocate a main frame
    stream.frame = AllocPicture(AV_PIX_FMT_YUV420P, frameWidth, frameHeight);
}

void FMVStream::CopyFrameData(const uint8_t* data)
{
    // convert the RGB frame to a YUV frame using the sws Context
    if (!stream.sws_ctx)
    {
        stream.sws_ctx = sws_getContext(stream.st->codec->width, stream.st->codec->height, AV_PIX_FMT_RGB32,
            stream.st->codec->width, stream.st->codec->height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    }

    int inLinesize[1] = { 4 * stream.st->codec->width };
    // use the scale function to transcode this raw frame to the correct type
    sws_scale(stream.sws_ctx, &data, inLinesize, 0, stream.st->codec->height, stream.frame->data, stream.frame->linesize);
}

int FMVStream::EncodeFrame(const uint8_t* data, const timeval& tv)
{
    AVCodecContext* c = stream.st->codec;

    AVRational one;
    one.den = one.num = 1;
#if 0
    // check to see if we want to keep writing frames we can probably change this to a toggle switch
    if (av_compare_ts(stream.next_pts, stream.st->codec->time_base, 10, one) >= 0)
    {
        stream.frame = nullptr;
    }
    else
    {
        // Convert and load the frame data into the AVFrame struct
    }
#endif
    CopyFrameData(data);

#if 0
    // setup the timestamp stepping
    AVPacket pkt = { 0 };
    av_init_packet(&pkt);
    stream.frame->pts = int64_t(90000 * stream.next_pts++) / stream.st->codec->framerate.den;

    int gotPacket, out_size, ret;

    out_size = avcodec_encode_video2(c, &pkt, stream.frame, &gotPacket);


    if (gotPacket == 1)
    {
        ret = write_frame(format_ctx, &c->time_base, stream.st, &pkt);
    }
    else
    {
        ret = 0;
    }

    if (ret < 0)
    {
        std::cerr << "Error writing video frame" << std::endl;
    }

    av_free_packet(&pkt);
#else
    int err;

    stream.frame->pts = int64_t(90000 * stream.next_pts++) / stream.st->codec->framerate.den;
    if ((err = avcodec_send_frame(c, stream.frame)) < 0)
    {
        std::cout << "Failed to send frame" << err << std::endl;
        return 0;
    }
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    pkt.flags |= AV_PKT_FLAG_KEY;
    if (avcodec_receive_packet(c, &pkt) == 0)
    {
        err = write_frame(format_ctx, &c->time_base, stream.st, &pkt);
        av_packet_unref(&pkt);
    }
#endif
    return 1;
}

int FMVStream::write_frame(AVFormatContext* fmt_ctx, const AVRational* time_base, AVStream* st, AVPacket* pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    return av_interleaved_write_frame(fmt_ctx, pkt);
}

void FMVStream::setFrameWidth(const int width)
{
    frameWidth = width;
}

int FMVStream::getFrameWidth() const
{
    return frameWidth;
}

void FMVStream::setFrameHeight(const int height)
{
    frameHeight = height;
}

int FMVStream::getFrameHeight() const
{
    return frameHeight;
}

void FMVStream::setStreamAddress(const std::string& address)
{
    streamFilename = address;
}

std::string FMVStream::getStreamAddress() const
{
    return streamFilename;
}
