#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <memory.h>
#include <ctime>
#include <cairo/cairo.h>
#include <sys/time.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}


constexpr int Width = 320;
constexpr int Height = 240;
constexpr int BytePerPixel = 4;
constexpr int StreamDuration = 10;
constexpr int NbFrames = 1000;
uint8_t Buffer[Width * Height * BytePerPixel];

AVFrame* videoFrame = nullptr;
AVCodecContext* cctx = nullptr;
SwsContext* swsCtx = nullptr;
int frameCounter = 0;
AVFormatContext* ofctx = nullptr;
AVOutputFormat* oformat = nullptr;
constexpr int Fps = 30;
constexpr int Bitrate = 2000;

static void pushFrame(const uint8_t* data, const timeval &tv)
{
	int err;
	if (!videoFrame)
	{
		videoFrame = av_frame_alloc();
		videoFrame->format = AV_PIX_FMT_YUV420P;
		videoFrame->width = cctx->width;
		videoFrame->height = cctx->height;
		if ((err = av_frame_get_buffer(videoFrame, 32)) < 0)
		{
			std::cout << "Failed to allocate picture" << err << std::endl;
			return;
		}
	}
	if (!swsCtx)
	{
		swsCtx = sws_getContext(cctx->width, cctx->height, AV_PIX_FMT_RGB32, cctx->width,
			cctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
	}
	int inLinesize[1] = { BytePerPixel * cctx->width };
	// From RGB to YUV
	sws_scale(swsCtx, &data, inLinesize, 0, cctx->height,
		videoFrame->data, videoFrame->linesize);
	videoFrame->pts = frameCounter++; // tv.tv_sec * 1000 + tv.tv_usec / 1000;

	std::cout << videoFrame->pts << " " << cctx->time_base.num << " " <<
		cctx->time_base.den << " " << frameCounter << std::endl;
	if ((err = avcodec_send_frame(cctx, videoFrame)) < 0)
	{
		std::cout << "Failed to send frame" << err << std::endl;
		return;
	}
	AV_TIME_BASE;
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;
	pkt.flags |= AV_PKT_FLAG_KEY;
	if (avcodec_receive_packet(cctx, &pkt) == 0)
	{
		static int counter = 0;
		if (counter == 0)
		{
			FILE* fp = fopen("dump_first_frame1.dat", "wb");
			fwrite(pkt.data, pkt.size, 1, fp);
			fclose(fp);
		}
	/*	AVRational s{
			1, 1
		};
		av_packet_rescale_ts(&pkt, s, cctx->time_base);*/

		std::cout << "pkt key: " << (pkt.flags & AV_PKT_FLAG_KEY) << " " <<
			pkt.size << " " << (counter++) << std::endl;
		std::cout << "pkt pts: " << pkt.pts << "  dts: " << pkt.dts << std::endl;
		uint8_t* size = ((uint8_t*)pkt.data);
		std::cout << "first: " << (int)size[0] << " " << (int)size[1] <<
			" " << (int)size[2] << " " << (int)size[3] << " " << (int)size[4] <<
			" " << (int)size[5] << " " << (int)size[6] << " " << (int)size[7] <<
			std::endl;

		av_interleaved_write_frame(ofctx, &pkt);
		av_packet_unref(&pkt);
	}
}

static int start(const char* zFilename)
{
	oformat = av_guess_format(nullptr, zFilename, nullptr);
	if (!oformat)
	{
		std::cout << "can't create output format" << std::endl;
		return -1;
	}
//	oformat->video_codec = AV_CODEC_ID_H264;

	int err = avformat_alloc_output_context2(&ofctx, oformat, nullptr, zFilename);

	if (err)
	{
		std::cout << "can't create output context" << std::endl;
		return -1;
	}

	AVCodec* codec = nullptr;

	codec = avcodec_find_encoder(oformat->video_codec);
	if (!codec)
	{
		std::cout << "can't create codec" << std::endl;
		return -1;
	}

	AVStream* stream = avformat_new_stream(ofctx, codec);

	if (!stream)
	{
		std::cout << "can't find format" << std::endl;
		return -1;
	}

#if 0
	AVCodecContext* codecCtx = stream->codec;
	if (!codecCtx)
	{
		std::cout << "codecCtx is null" << std::endl;
		return -1;
	}
avcodec_get_context_defaults3(codecCtx, codec);

	/* put sample parameters */
	codecCtx->bit_rate = Height * Width * BytePerPixel * Fps * 8;
	/* resolution must be a multiple of two */
	codecCtx->width = Width;
	codecCtx->height = Height;
	/* frames per second */
	codecCtx->time_base.num = 1;
	codecCtx->time_base.den = Fps;
	codecCtx->gop_size = 10; /* emit one intra frame every ten frames */
//	codecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	std::cout << "Here" << std::endl;

#else
	cctx = avcodec_alloc_context3(codec);

	if (!cctx)
	{
		std::cout << "can't create codec context" << std::endl;
		return -1;
	}
	cctx->bit_rate = Height * Width * BytePerPixel * Fps * 8;
	/* resolution must be a multiple of two */
	cctx->width = Width;
	cctx->height = Height;
	/* frames per second */
	cctx->time_base.num = 1;
	cctx->time_base.den = Fps;
	cctx->gop_size = 10; /* emit one intra frame every ten frames */
//	cctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
	cctx->pix_fmt = AV_PIX_FMT_YUV420P;
/*
	stream->codecpar->codec_id = oformat->video_codec;
	stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	stream->codecpar->width = Width;
	stream->codecpar->height = Height;
	stream->codecpar->format = AV_PIX_FMT_YUV420P;
	stream->codecpar->bit_rate = Height * Width * BytePerPixel * Fps * 8;
	avcodec_parameters_to_context(cctx, stream->codecpar);
	stream->time_base= (AVRational){ 1, 1 };
	cctx->time_base = (AVRational){ 1, Fps };
	cctx->max_b_frames = 2;
	cctx->gop_size = 12;
	cctx->framerate = (AVRational){ Fps, 1 };
	*/
	//must remove the following
	cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	if (stream->codecpar->codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(cctx, "preset", "ultrafast", 0);
	}
	else if (stream->codecpar->codec_id == AV_CODEC_ID_H265)
	{
		av_opt_set(cctx, "preset", "ultrafast", 0);
	}

	avcodec_parameters_from_context(stream->codecpar, cctx);
#endif

	if ((err = avcodec_open2(cctx, codec, NULL)) < 0)
	{
		std::cout << "Failed to open codec" << err << std::endl;
		return -1;
	}

	if (!(oformat->flags & AVFMT_NOFILE))
	{
		if ((err = avio_open(&ofctx->pb, zFilename, AVIO_FLAG_WRITE)) < 0)
		{
			std::cout << "Failed to open file" << err << std::endl;
			return -1;
		}
	}

	if ((err = avformat_write_header(ofctx, NULL)) < 0)
	{
		std::cout << "Failed to write header" << err << std::endl;
		return -1;
	}

	av_dump_format(ofctx, 0, zFilename, 1);

	return 0;
}

static void finish()
{
	//DELAYED FRAMES
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	for (;;)
	{
		avcodec_send_frame(cctx, NULL);
		if (avcodec_receive_packet(cctx, &pkt) == 0)
		{
			av_interleaved_write_frame(ofctx, &pkt);
			av_packet_unref(&pkt);
		}
		else
		{
			break;
		}
	}

	av_write_trailer(ofctx);
	if (!(oformat->flags & AVFMT_NOFILE))
	{
		int err = avio_close(ofctx->pb);
		if (err < 0)
		{
			std::cout << "Failed to close file" << err << std::endl;
		}
	}
}

static void free()
{
	if (videoFrame)
	{
		av_frame_free(&videoFrame);
		videoFrame = nullptr;
	}
	if (cctx)
	{
		avcodec_free_context(&cctx);
		cctx = nullptr;
	}

	if (ofctx)
	{
		avformat_free_context(ofctx);
		ofctx = nullptr;
	}
	if (swsCtx)
	{
		sws_freeContext(swsCtx);
		swsCtx = nullptr;
	}
}



static void DrawTime(cairo_surface_t* surfaceLocal, const tm* tm, const int nSequenceNumber)
{
	char text[128];

	memset(Buffer, 0xff, sizeof(Buffer));
	strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", tm);

	cairo_t* cr = cairo_create(surfaceLocal);

#if 0
	cairo_set_source_rgba(cr, 0.8, 0.2, 0.2, 1.);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

	cairo_move_to(cr, 50, 50);
	cairo_line_to(cr, 280, 200);

	cairo_stroke(cr);
#else
	/* text */
	cairo_select_font_face(cr, "Georgia",
		CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_move_to(cr, 10, 10);
	cairo_show_text(cr, text);

	sprintf(text, "sequence %d", nSequenceNumber);
	cairo_move_to(cr, 10, 100);
	cairo_show_text(cr, text);
#endif

	cairo_destroy(cr);
}

void BuildHLS()
{
	cairo_surface_t* surfaceLocal = cairo_image_surface_create_for_data(Buffer, CAIRO_FORMAT_ARGB32, Width, Height, Width * BytePerPixel);//

	int nSequenceNumber = 1;

	if (start("sequence_1.ts") < 0)
	{
		return;
	}

	time_t transition_time = time(nullptr) + StreamDuration;

	for (int i = 0; i < NbFrames; ++i)
	{
		timeval tv;
		gettimeofday(&tv, nullptr);

		if (tv.tv_sec > transition_time)
		{
			transition_time = tv.tv_sec + StreamDuration;
			finish();
			free();

			char filename[128];
			snprintf(filename, 128, "sequence_%d.ts", ++nSequenceNumber);
			if (start(filename) < 0)
			{
				return;
			}
		}

		tm* tm = localtime(&tv.tv_sec);
		DrawTime(surfaceLocal, tm, nSequenceNumber);
		pushFrame(Buffer, tv);

		usleep(1000000 / Fps);
	}

#if 0
	std::cout << "Writing png file\n";
	cairo_surface_write_to_png(surfaceLocal, "surface.png");
#endif

	finish();

	free();

	cairo_surface_destroy(surfaceLocal);


}


int main()
{
	av_register_all();
	avcodec_register_all();

	BuildHLS();

	return 0;
}
