#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <GL/gl.h>

#define GLMOTOR_SURFACE_S struct GLMotor_Surface_s
#include "glmotor.h"
#include "log.h"
#include "eglnative.h"

static EGLDisplay* egl_display = NULL;

static char running = 1;

struct GLMotor_Surface_s
{
	EGLNativeWindowType native_win;
	EGLContext egl_context;
	EGLSurface egl_surface;
};

GLMOTOR_EXPORT GLMotor_t *glmotor_create(int argc, char** argv)
{
	GLuint width = 640;
	GLuint height = 480;
	GLchar *name = "GLMotor";

	int opt;
	optind = 1;
	do
	{
		opt = getopt(argc, argv, "w:h:n:");
		switch (opt)
		{
			case 'w':
				width = atoi(optarg);
			break;
			case 'h':
				height = atoi(optarg);
			break;
			case 'n':
				name = optarg;
			break;
		}
	} while (opt != -1);

	EGLNativeDisplayType display = native_display();
#if 1
	egl_display = eglGetDisplay(display);
#else
	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = eglGetProcAddress("eglGetPlatformDisplayEXT");
	egl_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, display, NULL);
#endif
	EGLNativeWindowType native_win;
	native_win = native_createwindow(width, height, name);

	EGLint majorVersion;
	EGLint minorVersion;
	eglInitialize(egl_display, &majorVersion, &minorVersion);
	warn("glmotor: egl %d.%d", majorVersion, minorVersion);

	EGLint num_config;

	/** window creation */
#ifdef HAVE_GLESV2
	eglBindAPI(EGL_OPENGL_ES_API);
#else
	eglBindAPI(EGL_OPENGL_API);
#endif
	eglGetConfigs(egl_display, NULL, 0, &num_config);

	EGLint attributes[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_NONE
	};
	EGLConfig config;
	eglChooseConfig(egl_display, attributes, &config, 1, &num_config);

	GLMotor_Surface_t *window = calloc(1, sizeof(*window));
	if (native_win)
	{
		window->native_win = native_win;
		window->egl_surface = eglCreateWindowSurface(egl_display, config, native_win, NULL);
	}
	else
	{
		static EGLint pbufferAttribs[] = {
			EGL_WIDTH,	600,
			EGL_HEIGHT, 800,
			EGL_NONE,
		};
		pbufferAttribs[1] = width;
		pbufferAttribs[3] = height;
		window->egl_surface = eglCreatePbufferSurface(egl_display, config, pbufferAttribs);
	}
	EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE, EGL_NONE };
	window->egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, contextAttribs);

	GLMotor_t *motor = calloc(1, sizeof(*motor));
	motor->width = width;
	motor->height = height;
	motor->surf = window;

	eglMakeCurrent(egl_display, window->egl_surface, window->egl_surface, window->egl_context);

	return motor;
}

GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata)
{
	do
	{
		eglSwapBuffers(egl_display, motor->surf->egl_surface);
		draw(drawdata);
	} while (native_running(motor->surf->native_win));
	return 0;
}

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	GLMotor_Surface_t *window = motor->surf;

	eglDestroySurface(egl_display, window->egl_surface);
	eglDestroyContext(egl_display, window->egl_context);

	free(window);
	free(motor);

	native_destroy();
	eglTerminate(egl_display);
}
