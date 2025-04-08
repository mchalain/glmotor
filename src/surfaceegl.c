#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

#include <dlfcn.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>

PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES = NULL;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES = NULL;
PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT = NULL;
PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = NULL;
PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = NULL;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = NULL;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = NULL;

#define GLMOTOR_SURFACE_S struct GLMotor_Surface_s
#include "glmotor.h"
#include "log.h"
#include "eglnative.h"

#ifndef USE_PBUFFER
#define USE_PBUFFER 1
#endif

static EGLDisplay* egl_display = NULL;

struct eglnative_motor_s *native = NULL;
struct GLMotor_Surface_s
{
	EGLNativeDisplayType native_disp;
	EGLNativeWindowType native_win;
	EGLContext egl_context;
	EGLSurface egl_surface;
};

static EGLint egl_ContextType = EGL_OPENGL_ES2_BIT;
static int init_egl(void)
{
	glGenVertexArraysOES = (void *) eglGetProcAddress("glGenVertexArraysOES");
	if(glGenVertexArraysOES == NULL)
	{
		return -1;
	}
	glBindVertexArrayOES = (void *) eglGetProcAddress("glBindVertexArrayOES");
	if(glBindVertexArrayOES == NULL)
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

	eglCreateImageKHR = (void *) eglGetProcAddress("eglCreateImageKHR");
	if(!eglCreateImageKHR)
	{
		err("ERROR: extension eglCreateImageKHR not available");
		return -5;
	}

	eglDestroyImageKHR = (void *) eglGetProcAddress("eglDestroyImageKHR");
	if(!eglDestroyImageKHR)
	{
		err("ERROR: extension eglDestroyImageKHR not available");
		return -6;
	}

	glEGLImageTargetTexture2DOES = (void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");
	if(!glEGLImageTargetTexture2DOES)
	{
		err("ERROR: extension glEGLImageTargetTexture2DOES not available");
		return -7;
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
#if USE_PBUFFER
		eglnative_pbuffer,
#endif
		NULL
	};

	GLchar *name = "GLMotor";
	native = natives[0];
	EGLNativeDisplayType display = EGL_DEFAULT_DISPLAY;

	int opt;
	optind = 1;
	do
	{
		opt = getopt(argc, argv, "hn:N:L:");
		switch (opt)
		{
			case 'h':
				fprintf(stderr, "\t-n name\tset window name\n");
				fprintf(stderr, "\t-N native\tset native windowing system\n\t\twl: wayland\n\t\tx: X11\n\t\tdrm: direct rendering\n\t\tjpeg: jpeg file\n\t\tpbuffer: offscreen buffers\n");
				fprintf(stderr, "\t-L Library\tset library for offscreen buffers consumer\n");
				native = NULL;
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
			case 'L':
				pbufferconsumerhdl = dlopen(optarg, RTLD_LAZY | RTLD_GLOBAL);
				if (pbufferconsumerhdl == NULL)
				{
					err("glmotor: library errpr %s", dlerror());
				}
			break;
		}
	} while (opt != -1);

	if (native == NULL)
		return NULL;
	warn("glmotor: use egl %s", native->name);

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

	display = native->display();
	if (display == NULL)
	{
#if USE_PBUFFER
		native = eglnative_pbuffer;
		display = native->display();
#else
		err("glmotor: display disconnected");
		return NULL;
#endif
	}
	initEGLContext();

	egl_display = eglGetDisplay(display);
	if (egl_display == EGL_NO_DISPLAY)
	{
		err("glmotor: display not available");
		return NULL;
	}

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
#if GLMOTOR_DEPTH_BUFFER
			EGL_DEPTH_SIZE, 8,
#endif
#if GLMOTOR_STENCIL_BUFFER
			EGL_STENCIL_SIZE, 8,
#endif
			EGL_BUFFER_SIZE, 24,
			EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
			EGL_BIND_TO_TEXTURE_RGB , (native->type == EGL_PBUFFER_BIT)?EGL_TRUE:EGL_FALSE,
			EGL_RENDERABLE_TYPE, egl_ContextType,
			EGL_SURFACE_TYPE, native->type,
			EGL_NONE
		};

	EGLConfig eglconfig = EGL_NO_CONFIG_KHR;
	EGLint status = eglChooseConfig(egl_display, attributes, &eglconfig, 1, &num_config);
	if (status != EGL_TRUE || num_config == 0 || eglconfig == NULL)
	{
		err("glmotor: unable to find good eglconfig");
		return NULL;
	}

	EGLint contextAttribs[] = {
#ifdef EGL3
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 3,
#else
		EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
		EGL_NONE
	};
	EGLContext egl_context = EGL_NO_CONTEXT;
	egl_context = eglCreateContext(egl_display, eglconfig, EGL_NO_CONTEXT, contextAttribs);
	if (egl_context == NULL)
	{
		err("glmotor: egl context error");
		return NULL;
	}

	EGLSurface egl_surface = EGL_NO_SURFACE;
	EGLNativeWindowType native_win = 0;
	native_win = native->createwindow(display, config->width, config->height, name);

	if (native_win != 0)
	{
		egl_surface = eglCreateWindowSurface(egl_display, eglconfig, native_win, NULL);
	}
#if USE_PBUFFER
	else
	{
		EGLint pbufferAttribs[] = {
			EGL_WIDTH, config->width,
			EGL_HEIGHT, config->height,
			EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
			EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
			EGL_NONE,
		};

		egl_surface = eglCreatePbufferSurface(egl_display, eglconfig, pbufferAttribs);
	}
#endif
	if (egl_surface == EGL_NO_SURFACE)
	{
		err("glmotor: egl surface error");
		return NULL;
	}

	GLMotor_Surface_t *window = calloc(1, sizeof(*window));
	window->native_disp = display;
	window->egl_context = egl_context;
	window->native_win = native_win;
	window->egl_surface = egl_surface;

	eglMakeCurrent(egl_display, window->egl_surface, window->egl_surface, window->egl_context);

	if (native->windowsize)
		native->windowsize(window->native_win, &config->width, &config->height);

	return window;
}

GLMOTOR_EXPORT int surface_running(GLMotor_Surface_t *surf, GLMotor_t *motor)
{
	return native->running(surf->native_win, motor);
}

GLMOTOR_EXPORT GLuint surface_swapbuffers(GLMotor_Surface_t *surf)
{
	if (surf->native_win)
	{
		eglSwapBuffers(egl_display, surf->egl_surface);
	}
	return 0;
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

EGLDisplay* glmotor_egldisplay(GLMotor_t *motor)
{
	return egl_display;
}

EGLContext glmotor_eglcontext(GLMotor_t *motor)
{
	GLMotor_Surface_t *window = motor->surf;
	return window->egl_context;
}

GLMOTOR_EXPORT int glmotor_eglTexImage2D(GLuint texturefamily, GLuint ID, GLuint width, GLuint height, EGLAttrib *attribs)
{
		EGLImageKHR image = eglCreateImage(egl_display,
			EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
		if (image == EGL_NO_IMAGE_KHR)
		{
			return -1;
		}

		glBindTexture(texturefamily, ID);
		glTexParameteri(texturefamily, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(texturefamily, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glEGLImageTargetTexture2DOES(texturefamily, image);
		eglDestroyImage(egl_display, image);
	return 0;
}
