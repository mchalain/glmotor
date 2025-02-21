#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

//#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
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

PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray = NULL;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays = NULL;
PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = NULL;
PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = NULL;

static EGLint egl_ContextType = EGL_OPENGL_ES2_BIT;
static int init_egl(void)
{
	glGenVertexArrays = (void *) eglGetProcAddress("glGenVertexArraysOES");
	if(glGenVertexArrays == NULL)
	{
		return -1;
	}
	glBindVertexArray = (void *) eglGetProcAddress("glBindVertexArrayOES");
	if(glBindVertexArray == NULL)
	{
		return -2;
	}
	eglQueryDevicesEXT =
		(PFNEGLQUERYDEVICESEXTPROC) eglGetProcAddress("eglQueryDevicesEXT");
	if(!eglQueryDevicesEXT)
	{
		err("ERROR: extension eglQueryDevicesEXT not available");
		return -3;
	}

	eglGetPlatformDisplayEXT =
		(PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	if(!eglGetPlatformDisplayEXT)
	{
		err("ERROR: extension eglGetPlatformDisplayEXT not available");
		return -4;
	}
	return 0;
}

static void initEGLContext()
{
#ifdef EGL_KHR_create_context
	const char *extensions = eglQueryString ( egl_display, EGL_EXTENSIONS );

	// check whether EGL_KHR_create_context is in the extension string
	if ( extensions != NULL && strstr( extensions, "EGL_KHR_create_context" ) )
	{
		// extension is supported
		egl_ContextType = EGL_OPENGL_ES3_BIT_KHR;
	}
#endif
}
GLMOTOR_EXPORT GLMotor_Surface_t *surface_create(GLMotor_config_t *config, int argc, char** argv)
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

	GLchar *name = "GLMotor";
	native = natives[0];

	int opt;
	optind = 1;
	do
	{
		opt = getopt(argc, argv, "n:N:");
		switch (opt)
		{
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
	native_win = native->createwindow(display, config->width, config->height, name);

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
	init_egl();
	initEGLContext();

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
//		EGL_ALPHA_SIZE, 8,
//		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLConfig eglconfig;
	eglChooseConfig(egl_display, attributes, &eglconfig, 1, &num_config);

	EGLint contextAttribs[] = {
#ifdef EGL3
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 3,
#else
		EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
		EGL_NONE
	};
	EGLContext egl_context = eglCreateContext(egl_display, eglconfig, EGL_NO_CONTEXT, contextAttribs);
	if (egl_context == NULL)
	{
		err("glmotor: egl context error");
		return NULL;
	}

	if (native_win == 0)
	{
		err("glmotor: egl window not set");
		return NULL;
	}
	EGLSurface egl_surface = eglCreateWindowSurface(egl_display, eglconfig, native_win, NULL);

	if (egl_surface == NULL)
	{
		err("glmotor: egl surface error");
		return NULL;
	}
	GLMotor_Surface_t *window = calloc(1, sizeof(*window));
	window->native_disp = display;
	window->egl_context = egl_context;
	window->native_win = native_win;
	window->egl_surface = egl_surface;

	if (native->windowsize)
		native->windowsize(window->native_win, &config->width, &config->height);

	eglMakeCurrent(egl_display, window->egl_surface, window->egl_surface, window->egl_context);

	return window;
}

GLMOTOR_EXPORT int surface_running(GLMotor_Surface_t *surf, GLMotor_t *motor)
{
	if (surf->native_win)
		return native->running(surf->native_win, motor);
	return 1;
}

GLMOTOR_EXPORT GLuint surface_swapbuffers(GLMotor_Surface_t *surf)
{
	return eglSwapBuffers(egl_display, surf->egl_surface);
}

GLMOTOR_EXPORT void surface_destroy(GLMotor_Surface_t *window)
{
	eglMakeCurrent(egl_display, EGL_NO_SURFACE,
                  EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(egl_display, window->egl_surface);
	eglDestroyContext(egl_display, window->egl_context);

	native->destroy(window->native_disp);
	free(window);

	eglTerminate(egl_display);
}

GLMOTOR_EXPORT EGLDisplay* glmotor_egldisplay(GLMotor_t *motor)
{
	return egl_display;
}
