#include <unistd.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "log.h"
#include "glmotor.h"
#include "pbufferconsumer/pbufferconsumer.h"
#include "pbufferconsumer/socket.h"

#define consumer_dbg(...)

extern PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR;
extern PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA;
extern PFNEGLEXPORTDMABUFIMAGEMESAPROC eglExportDMABUFImageMESA;

#ifndef DRM_FORMAT_MOD_LINEAR
#define DRM_FORMAT_MOD_LINEAR 0ULL
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

typedef struct GLMotor_s GLMotor_t;
typedef struct consumer_dmabuf_s consumer_dmabuf_t;
struct consumer_dmabuf_s
{
	GLMotor_t *motor;
	void *private;
};

typedef struct texture_storage_metadata_s texture_storage_metadata_t;
struct texture_storage_metadata_s
{
	int fourcc;
	EGLuint64KHR modifiers;
	EGLint stride;
	EGLint offset;
};

typedef struct _dmabuf_socket_s _dmabuf_socket_t;
struct _dmabuf_socket_s
{
	int fd;
};

int _dmabuf_init(consumer_dmabuf_t *dmabuf, uint32_t width, uint32_t height)
{
	_dmabuf_socket_t *data = calloc(1, sizeof(*data));
	const char *SERVER_FILE = "/tmp/test_server";
	const char *CLIENT_FILE = "/tmp/test_client";

	int sock = create_socket(SERVER_FILE);
	while (connect_socket(sock, CLIENT_FILE) != 0);
	data->fd = sock;
	dmabuf->private = data;
	warn("consumer: dmabuf initialized on %s", CLIENT_FILE);
	return 0;
}

int _dmabuf_send(consumer_dmabuf_t *dmabuf, int dma_buf, texture_storage_metadata_t *metadata)
{
	_dmabuf_socket_t *data = (_dmabuf_socket_t *)dmabuf->private;
	write_fd(data->fd, dma_buf, metadata, sizeof(*metadata));
	return 0;
}

void _dmabuf_uninit(consumer_dmabuf_t *dmabuf)
{
	_dmabuf_socket_t *data = (_dmabuf_socket_t *)dmabuf->private;
	close(data->fd);
	free(data);
}

void *pbufferconsumer_create(GLMotor_t *motor, uint32_t width, uint32_t height)
{
	consumer_dmabuf_t *dmabuf = calloc(1, sizeof(*dmabuf));
	dmabuf->motor = motor;
	if (_dmabuf_init(dmabuf, width, height))
	{
		free(dmabuf);
		return NULL;
	}

	return dmabuf;
}

void pbufferconsumer_queue(void * arg, GLuint client, pbufferconsumer_metadata_t metadata)
{
	consumer_dmabuf_t *dmabuf = arg;
	EGLDisplay* egl_display = eglGetCurrentDisplay();
	EGLContext egl_context = eglGetCurrentContext();

	EGLImage image = eglCreateImage(egl_display, egl_context, metadata.target, (EGLClientBuffer)client, NULL);

	if (image)
	{
		int numplanes = 0;
		int fourcc = 0;
		EGLuint64KHR modifier;

		eglExportDMABUFImageQueryMESA(egl_display, image,
									&fourcc, &numplanes, &modifier);
		if (modifier != DRM_FORMAT_MOD_LINEAR)
			consumer_dbg("glmotor: Mesa modifier not supported");

		for (int i = 0; i < numplanes && i < 5; i++)
		{
			int dma_buf = 0;
			texture_storage_metadata_t metadata = {
					.fourcc = fourcc,
					.modifiers = modifier,
				};
			eglExportDMABUFImageMESA(egl_display, image, &dma_buf, &metadata.stride, &metadata.offset);
			consumer_dbg("glmotor: image(%d) %.4s (%#x), %lu %lu %#llX", dma_buf,
				(char*)&metadata.fourcc, metadata.fourcc, metadata.stride,
				metadata.stride * dmabuf->motor->height, metadata.modifiers);
			if (_dmabuf_send(dmabuf, dma_buf, &metadata))
				close(dma_buf);
		}
		eglDestroyImage(egl_display, image);
	}
	else
	{
		err("glmotor: pbuffer image failed (%#x)", eglGetError());
	}
}

void pbufferconsumer_destroy(void * arg)
{
	consumer_dmabuf_t *dmabuf = arg;
	_dmabuf_uninit(dmabuf);
	free(arg);
}
