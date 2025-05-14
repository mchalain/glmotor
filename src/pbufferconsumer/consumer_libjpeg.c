#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <jpeglib.h>

#include "log.h"
#include "glmotor.h"
#include "pbufferconsumer/pbufferconsumer.h"

typedef struct consumer_jpeg_s consumer_jpeg_t;
struct consumer_jpeg_s
{
	GLMotor_t *motor;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *frame;
};

#define FRAMERATE 30

void *pbufferconsumer_create(GLMotor_t *motor, uint32_t width, uint32_t height)
{
	unsigned char *frame = malloc(width * height * 4);
	consumer_jpeg_t *jpeg = calloc(1, sizeof(*jpeg));
	jpeg->motor = motor;
	jpeg->frame = frame;
	jpeg_create_compress(&jpeg->cinfo);
	jpeg->cinfo.image_width = width;
	jpeg->cinfo.image_height = height;
	jpeg->cinfo.input_components = 4;
	//jpeg->cinfo.in_color_space = JCS_RGB;
	jpeg->cinfo.in_color_space = JCS_EXT_XRGB;
	jpeg->cinfo.err = jpeg_std_error(&jpeg->jerr);
	jpeg_set_defaults(&jpeg->cinfo);
	jpeg_suppress_tables(&jpeg->cinfo, TRUE);
	return jpeg;
}

static int _encode(consumer_jpeg_t *jpeg, unsigned char *input, FILE *fout)
{
	int ret = 0;
	struct jpeg_compress_struct *pcinfo = &jpeg->cinfo;
	jpeg_stdio_dest(pcinfo, fout);
	jpeg_start_compress(pcinfo, TRUE);
	int height = jpeg->cinfo.image_height;
	for (int i = 0; i < height; i++)
	{
		JSAMPROW row_pointer[1];
		row_pointer[0] =  &input[i * pcinfo->image_width * pcinfo->input_components];
		jpeg_write_scanlines(pcinfo, row_pointer, 1);
	}
	if (pcinfo->next_scanline >= pcinfo->image_height)
	{
		jpeg_finish_compress(pcinfo);
	}
	return ret;
}

static char template[] = "/tmp/fileXXXXXX.jpg";
static FILE *fout = NULL;
void pbufferconsumer_queue(void * arg, GLuint client, pbufferconsumer_metadata_t metadata)
{
	if (fout == NULL)
	{
		dbg("coucou");
		int fd = mkstemps(template, 4);
		fout = fdopen(fd, "wb");

	}
	if (fout == NULL)
		err("file error %m");
	consumer_jpeg_t *jpeg = arg;
	//if (glIsTexture(client))
	{
 		//glBindTexture(GL_TEXTURE_2D, client);
		//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, avcodec->motor->width, avcodec->motor->height, 0, GL_RGBA, GL_UNSIGNED_INT_24_8, NULL);
		//glReadPixels(0, 0, avcodec->motor->width, avcodec->motor->height, GL_RGBA, GL_UNSIGNED_INT_24_8_OES, avcodec->frame->data);
		glReadPixels(0, 0, jpeg->motor->width, jpeg->motor->height, GL_RGBA, GL_UNSIGNED_BYTE, jpeg->frame);
		if (_encode(jpeg, jpeg->frame, fout))
			return;
	}
	fclose(fout);
	fout = NULL;
}

void pbufferconsumer_destroy(void * arg)
{
	consumer_jpeg_t *jpeg = arg;
	free(arg);
}
