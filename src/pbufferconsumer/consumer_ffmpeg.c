#include <unistd.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

#include "log.h"
#include "glmotor.h"
#include "pbufferconsumer/pbufferconsumer.h"

extern EGLDisplay* glmotor_egldisplay(GLMotor_t *motor);
extern EGLContext glmotor_eglcontext(GLMotor_t *motor);

typedef struct consumer_avcodec_s consumer_avcodec_t;
struct consumer_avcodec_s
{
	GLMotor_t *motor;
	const AVCodec *codec;
	AVCodecContext *ctx;
	AVFrame *frame;
	AVPacket *out;
	unsigned char *image;
};

#define FRAMERATE 30

AVCodecContext *_test_codec(const AVCodec *codec, uint32_t width, uint32_t height)
{
	AVCodecContext *ctx = avcodec_alloc_context3(codec);
	if (ctx == NULL)
	{
		err("glmotor: video encoder not allowed");
		return NULL;
	}
	ctx->bit_rate = 400000;
	/* resolution must be a multiple of two */
	ctx->width = width;
	ctx->height = height;
	/* frames per second */
	ctx->time_base = (AVRational){1, FRAMERATE};
	ctx->framerate = (AVRational){FRAMERATE, 1};

	ctx->gop_size = 10;
	ctx->max_b_frames = 0;
	ctx->pix_fmt = AV_PIX_FMT_RGB24;
	//ctx->pix_fmt = AV_PIX_FMT_RGBA;
	//ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(ctx->priv_data, "preset", "slow", 0);

	if (avcodec_open2(ctx, codec, NULL) < 0)
	{
		err("glmotor: video encoder context creation ");
		return NULL;
	}
	return ctx;
}

void *pbufferconsumer_create(GLMotor_t *motor, uint32_t width, uint32_t height)
{
	const AVCodec *codec = NULL;
	AVCodecContext *ctx = NULL;
	void *i = 0;

	while ((codec = av_codec_iterate(&i)))
	{
		if (codec->type == AVMEDIA_TYPE_VIDEO && codec->pix_fmts)
		{
			for (int i = 0; codec->pix_fmts[i] != AV_PIX_FMT_NONE; i++)
			{
				if (codec->pix_fmts[i] == AV_PIX_FMT_RGB24)
					warn("glmotor: select video codec %s", codec->name);
			}
		}
	}

#if 0
	i = 0;
	while ((codec = av_codec_iterate(&i)) && ctx == NULL)
#else
	codec = avcodec_find_encoder_by_name("libx264rgb");
#endif
	{
		ctx = _test_codec(codec, width, height);
	}
	if (ctx == NULL)
	{
		return NULL;
	}
	warn("glmotor: select video codec %s", codec->name);

	AVFrame *frame = av_frame_alloc();
	if (frame == NULL)
	{
		err("glmotor: video encoder frame error ");
		return NULL;
	}
	frame->format = ctx->pix_fmt;
	frame->width  = ctx->width;
	frame->height = ctx->height;
	if (av_frame_get_buffer(frame, 0) < 0)
	{
		err("glmotor: video encoder frame error 2 ");
		return NULL;
	}
	AVPacket *pkt = av_packet_alloc();
	if (pkt == NULL)
	{
		err("glmotor: video encoder packet error ");
		return NULL;
	}
	consumer_avcodec_t *avcodec = calloc(1, sizeof(*avcodec));
	avcodec->motor = motor;
	avcodec->codec = codec;
	avcodec->ctx = ctx;
	avcodec->frame = frame;
	avcodec->out = pkt;
	avcodec->image = malloc(width * height * 4);
	dbg("glmotor: image siae %lu", avcodec->frame->height * avcodec->frame->linesize[0]);
	return avcodec;
}

static int _encode(AVCodecContext *ctx, AVPacket *pkt, FILE *fout)
{
	int ret = 0;
	while (ret == 0)
	{
		ret = avcodec_receive_packet(ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			ret = 0;
			break;
		}
		else if (ret < 0)
		{
			err("glmotor: encoding error");
			break;
		}
		fwrite(pkt->data, 1, pkt->size, fout);
		av_packet_unref(pkt);
	}
	return ret;
}

static FILE *fout = NULL;
void pbufferconsumer_queue(void * arg, GLuint client, pbufferconsumer_metadata_t metadata)
{
	if (fout == NULL)
		fout = fopen("test.h264", "wb");
	if (fout == NULL)
		err("file error %m");
	consumer_avcodec_t *avcodec = arg;
	av_frame_make_writable(avcodec->frame);

	//if (glIsTexture(client))
	{
		uint32_t width = avcodec->motor->width;
		uint32_t height = avcodec->motor->height;
		/**
		 * OpenGL ES support only GL_RGBA format for glReadPixels
		 */
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, avcodec->motor->width, avcodec->motor->height, 0, GL_RGBA, GL_UNSIGNED_INT_24_8, NULL);
		//glReadPixels(0, 0, avcodec->motor->width, avcodec->motor->height, GL_RGBA, GL_UNSIGNED_INT_24_8_OES, avcodec->frame->data);
		//glReadPixels(0, 0, avcodec->motor->width, avcodec->motor->height, GL_RGB, GL_UNSIGNED_BYTE, avcodec->frame->data[0]);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, avcodec->image);
		for (int h = 0; h < height; h++)
		{
			for (int w = 0; w < width; w++)
			{
				uint32_t color = ((uint32_t *)avcodec->image)[((h * width) + w)];
				avcodec->frame->data[0][((h * width) + w) * 3 + 0] = (color >> 0 * 8) & 0x0FF;
				avcodec->frame->data[0][((h * width) + w) * 3 + 1] = (color >> 1 * 8) & 0x0FF;
				avcodec->frame->data[0][((h * width) + w) * 3 + 2] = (color >> 2 * 8) & 0x0FF;
				//image[(w * renderBufferWidth) * h * pitch + 3] = 0;
			}
		}
		avcodec->frame->pts++;
		avcodec_send_frame(avcodec->ctx, avcodec->frame);
		if (_encode(avcodec->ctx, avcodec->out, fout))
			return;
	}
}

void pbufferconsumer_destroy(void * arg)
{
	consumer_avcodec_t *avcodec = arg;
	_encode(avcodec->ctx, avcodec->out, fout);
	if (avcodec->codec->id == AV_CODEC_ID_MPEG1VIDEO || avcodec->codec->id == AV_CODEC_ID_MPEG2VIDEO)
	{
		uint8_t endcode[] = { 0, 0, 1, 0xb7 };
		fwrite(endcode, 1, sizeof(endcode), fout);
	}
	fclose(fout);
	avcodec_free_context(&avcodec->ctx);
	av_frame_free(&avcodec->frame);
	av_packet_free(&avcodec->out);
	free(avcodec->image);
	free(arg);
}
