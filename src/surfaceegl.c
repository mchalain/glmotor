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

	EGLNativeWindowType native_win;
	native_win = native_createwindow(width, height, name);
	if (native_win == 0)
		return NULL;

	egl_display = eglGetDisplay(native_display());
	EGLint majorVersion;
	EGLint minorVersion;
	eglInitialize(egl_display, &majorVersion, &minorVersion);

	EGLint num_config;

	/** window creation */
	//eglBindAPI(EGL_OPENGL_API);
	eglGetConfigs(egl_display, NULL, 0, &num_config);
#if 0
	EGLint attributes[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE
	};
#else
	EGLint attributes[] =
	{
		EGL_RED_SIZE,       5,
		EGL_GREEN_SIZE,     6,
		EGL_BLUE_SIZE,      5,
		EGL_ALPHA_SIZE,     EGL_DONT_CARE,
		EGL_DEPTH_SIZE,     EGL_DONT_CARE,
		EGL_STENCIL_SIZE,   EGL_DONT_CARE,
		EGL_SAMPLE_BUFFERS, 0,
		EGL_NONE
	};
#endif
	EGLConfig config;
	eglChooseConfig(egl_display, attributes, &config, 1, &num_config);

	GLMotor_Surface_t *window = calloc(1, sizeof(*window));
	window->native_win = native_win;
	window->egl_surface = eglCreateWindowSurface(egl_display, config, native_win, NULL);
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
	while (native_running(motor->surf->native_win))
	{
		draw(drawdata);
		eglSwapBuffers(egl_display, motor->surf->egl_surface);
	}
	return 0;
}

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	GLMotor_Surface_t *window = motor->surf;

	eglDestroySurface(egl_display, window->egl_surface);
	eglDestroyContext(egl_display, window->egl_context);

	free(window);
	free(motor);

	eglTerminate(egl_display);
}
