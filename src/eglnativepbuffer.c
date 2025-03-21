#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
#else
# include <GL/gl.h>
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "log.h"
#include "glmotor.h"
#include "eglnative.h"

extern EGLDisplay* glmotor_egldisplay(GLMotor_t *motor);
extern EGLContext glmotor_eglcontext(GLMotor_t *motor);

static EGLNativeDisplayType *display = NULL;
static GLMotor_Offscreen_t *offscreen = NULL;
static GLMotor_config_t config = {0};

static EGLNativeDisplayType native_display()
{
#if 0
	static const int MAX_DEVICES = 4;
	EGLDeviceEXT eglDevs[MAX_DEVICES];
	EGLint numDevices = 0;
	eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &numDevices);
	dbg("num devices %d", numDevices);
	for (int i = 0; i < numDevices && display == NULL; i++)
		display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevs[0], 0);
#else
	display =  EGL_DEFAULT_DISPLAY;
#endif
	return display;
}

static GLboolean native_running(EGLNativeWindowType native_win, GLMotor_t *motor)
{
	GLboolean running = GL_TRUE;
	EGLDisplay* egl_display = glmotor_egldisplay(motor);
	EGLContext egl_context = glmotor_eglcontext(motor);
	EGLint target = EGL_GL_TEXTURE_2D;
	EGLClientBuffer client = (EGLClientBuffer)offscreen->texture[0];
	if (offscreen->rbo[0] != 0)
	{
		target = EGL_GL_RENDERBUFFER;
		client = (EGLClientBuffer)offscreen->rbo[0];
	}
	EGLImage image = eglCreateImage(egl_display, egl_context,
			target, client, NULL);

	if (image)
	{
		int texture_dmabuf_fd = 0;
		struct texture_storage_metadata_t
		{
			uint32_t fourcc;
			uint32_t offset;
			uint32_t stride;
		} texture_storage_metadata;

		PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC eglExportDMABUFImageQueryMESA =
			(PFNEGLEXPORTDMABUFIMAGEQUERYMESAPROC)eglGetProcAddress("eglExportDMABUFImageQueryMESA");
		PFNEGLEXPORTDMABUFIMAGEMESAPROC eglExportDMABUFImageMESA =
			(PFNEGLEXPORTDMABUFIMAGEMESAPROC)eglGetProcAddress("eglExportDMABUFImageMESA");

		eglExportDMABUFImageQueryMESA(egl_display, image,
									&texture_storage_metadata.fourcc, NULL, NULL);
		eglExportDMABUFImageMESA(egl_display, image, &texture_dmabuf_fd,
									&texture_storage_metadata.stride, &texture_storage_metadata.offset);
		warn("glmotor: image(%d) %.4s (%#x), %lu", texture_dmabuf_fd, (char*)&texture_storage_metadata.fourcc, texture_storage_metadata.fourcc, texture_storage_metadata.stride);
		close(texture_dmabuf_fd);
	}
	else
	{
		err("glmotor: pbuffer image failed (%#x)", eglGetError());
	}
	return running;
}

static EGLNativeWindowType native_createwindow(EGLNativeDisplayType display, GLuint width, GLuint height, const GLchar *name)
{
	config.width = width;
	config.height = height;
	return (EGLNativeWindowType) 0;
}

static void native_windowsize(EGLNativeWindowType native_win, GLuint *width, GLuint *height)
{
	offscreen = glmotor_offscreen_create(&config);
	dbg("glmotor: offscreen %p", offscreen);
	if (offscreen == NULL)
		exit(1);
	*width = config.width;
	*height = config.height;
}

static void native_destroy(EGLNativeDisplayType native_display)
{
	if (offscreen)
	{
		glmotor_offscreen_destroy(offscreen);
	}
}

struct eglnative_motor_s *eglnative_pbuffer = &(struct eglnative_motor_s)
{
	.name = "pbuffer",
	.type = EGL_PBUFFER_BIT,
	.display = native_display,
	.createwindow = native_createwindow,
	.windowsize = native_windowsize,
	.running = native_running,
	.destroy = native_destroy,
};
