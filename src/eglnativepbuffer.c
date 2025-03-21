#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

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
#include "pbufferconsumer/pbufferconsumer.h"

#ifndef USE_FRAMEBUFFER
#define USE_FRAMEBUFFER 1
#endif

extern EGLDisplay* glmotor_egldisplay(GLMotor_t *motor);
extern EGLContext glmotor_eglcontext(GLMotor_t *motor);

void *(*pbufferconsumer_create)(GLMotor_t *motor, uint32_t width, uint32_t height);
void (*pbufferconsumer_destroy)(void * arg);
void (*pbufferconsumer_queue)(void * arg, GLuint client, pbufferconsumer_metadata_t data);
static void *pbufferconsumer = NULL;

void* pbufferconsumerhdl = NULL;

static EGLNativeDisplayType *display = NULL;
static GLMotor_Offscreen_t *offscreen = NULL;
static GLMotor_config_t config = {0};

static EGLNativeDisplayType native_display()
{
	if (pbufferconsumerhdl)
	{
		pbufferconsumer_create = dlsym(pbufferconsumerhdl, "pbufferconsumer_create");
		pbufferconsumer_destroy = dlsym(pbufferconsumerhdl, "pbufferconsumer_destroy");
		pbufferconsumer_queue = dlsym(pbufferconsumerhdl, "pbufferconsumer_queue");
		if (pbufferconsumer_create == NULL)
		{
			err("glmotor: library errpr %s", dlerror());
			dlclose(pbufferconsumerhdl);
			pbufferconsumerhdl = NULL;
		}
	}
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
	GLuint client = 0;
	pbufferconsumer_metadata_t metadata = {
		.target = EGL_GL_TEXTURE_2D,
		.type = GL_RGBA,
	};
#if USE_FRAMEBUFFER
	metadata.target = EGL_GL_TEXTURE_2D;
	client = (EGLClientBuffer)offscreen->texture[0];
	if (offscreen->rbo[0] != 0)
	{
		metadata.target = EGL_GL_RENDERBUFFER;
		client = offscreen->rbo[0];
	}
#endif
	if (pbufferconsumerhdl && pbufferconsumer == NULL)
	{
		pbufferconsumer = pbufferconsumer_create(motor, config.width, config.height);
	}
	if (pbufferconsumerhdl && pbufferconsumer)
	{
		pbufferconsumer_queue(pbufferconsumer, client, metadata);
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
#if USE_FRAMEBUFFER
	/// offscreen is create here because eglContext must be ready
	offscreen = glmotor_offscreen_create(&config);
	dbg("glmotor: offscreen %p", offscreen);
	if (offscreen == NULL)
		exit(1);
#endif
	*width = config.width;
	*height = config.height;
}

static void native_destroy(EGLNativeDisplayType native_display)
{
#if USE_FRAMEBUFFER
	if (offscreen)
	{
		glmotor_offscreen_destroy(offscreen);
	}
#endif
	if (pbufferconsumerhdl && pbufferconsumer)
		pbufferconsumer_destroy(pbufferconsumer);
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
