#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <unistd.h>
#include <memory.h>
#include <ctime>
#include <cairo/cairo.h>
#include <sys/time.h>
#include "FMVStream.h"

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
constexpr int Fps = 30;
constexpr int Bitrate = 2000;
uint8_t Buffer[Width * Height * BytePerPixel];

#if 0
AVFrame* videoFrame = nullptr;
AVCodecContext* cctx = nullptr;
SwsContext* swsCtx = nullptr;
int frameCounter = 0;
AVFormatContext* ofctx = nullptr;
AVOutputFormat* oformat = nullptr;

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
#endif

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

	FMVStream *pStream = new FMVStream("sequence_1.ts", Width, Height);

	time_t transition_time = time(nullptr) + StreamDuration;

	for (int i = 0; i < NbFrames; ++i)
	{
		timeval tv;
		gettimeofday(&tv, nullptr);

		if (tv.tv_sec > transition_time)
		{
			transition_time = tv.tv_sec + StreamDuration;

			delete pStream;
			pStream = nullptr;

			char filename[128];
			snprintf(filename, 128, "sequence_%d.ts", ++nSequenceNumber);
			pStream = new FMVStream(filename, Width, Height);
		}

		tm* tm = localtime(&tv.tv_sec);
		DrawTime(surfaceLocal, tm, nSequenceNumber);
		pStream->EncodeFrame(Buffer, tv);

		usleep(1000000 / Fps);
	}

#if 0
	std::cout << "Writing png file\n";
	cairo_surface_write_to_png(surfaceLocal, "surface.png");
#endif

	delete pStream;

	cairo_surface_destroy(surfaceLocal);
}


int main()
{

	BuildHLS();

	return 0;
}
