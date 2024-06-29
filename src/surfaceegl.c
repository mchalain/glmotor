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

struct eglnative_motor_s *native = NULL;
struct GLMotor_Surface_s
{
	EGLNativeDisplayType native_disp;
	EGLNativeWindowType native_win;
	EGLContext egl_context;
	EGLSurface egl_surface;
};

GLMOTOR_EXPORT GLMotor_t *glmotor_create(int argc, char** argv)
{
	struct eglnative_motor_s *natives[] = {
#ifdef HAVE_WAYLAND_EGL
		eglnative_wl,
#endif
#ifdef HAVE_X11
		eglnative_x,
#endif
#ifdef HAVE_GBM
		eglnative_drm,
#endif
#ifdef HAVE_LIBJPEG
		eglnative_jpeg,
#endif
		NULL
	};

	GLuint width = 640;
	GLuint height = 480;
	GLchar *name = "GLMotor";
	native = natives[0];

	int opt;
	optind = 1;
	do
	{
		opt = getopt(argc, argv, "w:h:n:N:");
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
			case 'N':
				for (int i = 0; natives[i] != NULL; i++)
				{
					if (! strcmp(natives[i]->name, optarg))
					{
						native = natives[i];
						break;
					}
				}
			break;
		}
	} while (opt != -1);

	if (native == NULL)
		return NULL;
	warn("glmotor: use egl %s", native->name);

	EGLNativeDisplayType display = native->display();
	if (display == NULL)
		err("glmotor: unable to open the display");
#if 1
	egl_display = eglGetDisplay(display);
#else
	PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = eglGetProcAddress("eglGetPlatformDisplayEXT");
	egl_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR, display, NULL);
#endif
	const char * extensions = eglQueryString(display, EGL_EXTENSIONS);
	dbg("glmotor: egl extensions\n%s", extensions);

	EGLNativeWindowType native_win;
	native_win = native->createwindow(display, width, height, name);

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
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLConfig config;
	eglChooseConfig(egl_display, attributes, &config, 1, &num_config);

	GLMotor_Surface_t *window = calloc(1, sizeof(*window));
	window->native_disp = display;
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
	EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
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
	eglMakeCurrent(egl_display, motor->surf->egl_surface, motor->surf->egl_surface, motor->surf->egl_context);

	do
	{
		eglSwapBuffers(egl_display, motor->surf->egl_surface);
		draw(drawdata);
	} while (native->running(motor->surf->native_win));
	return 0;
}

GLMOTOR_EXPORT GLuint glmotor_swapbuffers(GLMotor_t *motor)
{
	return eglSwapBuffers(egl_display, motor->surf->egl_surface);
}

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	GLMotor_Surface_t *window = motor->surf;

	eglMakeCurrent(egl_display, EGL_NO_SURFACE,
                  EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(egl_display, window->egl_surface);
	eglDestroyContext(egl_display, window->egl_context);

	native->destroy(motor->surf->native_disp);
	free(window);
	free(motor);

	eglTerminate(egl_display);
}

GLMOTOR_EXPORT EGLDisplay* glmotor_egldisplay(GLMotor_t *motor)
{
	return egl_display;
}
