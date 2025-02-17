#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
#else
# include <GL/gl.h>
#endif

#define GLMOTOR_SURFACE_S struct GLMotor_Surface_s
#include "glmotor.h"
#include "log.h"
#include "eglnative.h"

static EGLDisplay* egl_display = NULL;

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

	EGLNativeWindowType native_win;
	native_win = native->createwindow(display, width, height, name);

#ifdef DEBUG
	dbg("glmotor: egl extensions");
	const char * extensions = eglQueryString(display, EGL_EXTENSIONS);
	const char * end;
	if (extensions)
		end = strchr(extensions, ' ');
	else
		dbg("\tnone");
	while (extensions != NULL)
	{
		size_t length = strlen(extensions);
		if (end)
			length = end - extensions;
		dbg("\t%.*s", length, extensions);
		extensions = end;
		if (end)
		{
			extensions++;
			end = strchr(extensions, ' ');
		}
	}
#endif

	egl_display = eglGetDisplay(display);

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
	EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	window->egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, contextAttribs);
	if (window->egl_context == NULL)
	{
		err("glmotor: egl context error");
		return NULL;
	}

	if (native_win)
	{
		window->native_win = native_win;
		window->egl_surface = eglCreateWindowSurface(egl_display, config, native_win, NULL);
	}
	else
	{
		warn("glmotor: egl buffer");
		static EGLint pbufferAttribs[] = {
			EGL_WIDTH,	600,
			EGL_HEIGHT, 800,
			EGL_NONE,
		};
		pbufferAttribs[1] = width;
		pbufferAttribs[3] = height;
		window->egl_surface = eglCreatePbufferSurface(egl_display, config, pbufferAttribs);
	}

	if (window->egl_surface == NULL)
	{
		err("glmotor: egl surface error");
		return NULL;
	}

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
		glmotor_swapbuffers(motor);
		if (motor->eventcb && motor->events)
		{
			GLMotor_Event_t *evt = NULL;
			for (evt = motor->events; evt != NULL; evt = evt->next)
				motor->eventcb(motor->eventcbdata, evt);
			free(evt);
			motor->events = evt;
		}
		draw(drawdata);
	} while (native->running(motor->surf->native_win, motor));
	return 0;
}

GLMOTOR_EXPORT GLuint glmotor_swapbuffers(GLMotor_t *motor)
{
#ifdef DEBUG
	static uint32_t nbframes = 0;
	nbframes++;
	static time_t start = 0;
	time_t now = time(NULL);
	if (start == 0)
		start = time(NULL);
	else if (start < now)
	{
		dbg("glmotor: %lu fps", nbframes / (now - start));
		start = now;
		nbframes = 0;
	}
#endif
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
