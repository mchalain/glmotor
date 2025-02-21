#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
# undef HAVE_GLEW
#else
# ifdef HAVE_GLEW
#  include <GL/glew.h>
# endif
# include <GL/gl.h>
#endif
#ifdef HAVE_GLU
# include <GL/glu.h>
#endif

#include "glmotor.h"
#include "log.h"

#ifdef HAVE_GLEW
static int init_glew()
{
	if ( glewInit() != GLEW_OK )
	{
	    err("glmotor: glewInit error %m");
	    return -1;
	}

	if ( !glewIsSupported("GL_ARB_shading_language_100") )
	{
	    err("glmotor: GL_ARB_shading_language_100 error %m");
		return -2;
	}
	if ( !glewIsSupported("GL_ARB_shader_objects") )
	{
	    err("glmotor: GL_ARB_shader_objects error %m");
		return -3;
	}
	if ( !glewIsSupported("GL_ARB_vertex_shader") )
	{
	    err("glmotor: GL_ARB_vertex_shader error %m");
		return -4;
	}
	if ( !glewIsSupported("GL_ARB_fragment_shader") )
	{
	    err("glmotor: GL_ARB_fragment_shader error %m");
		return -5;
	}
	return 0;
}
#endif

GLMOTOR_EXPORT GLMotor_t *glmotor_create(GLMotor_config_t *config, int argc, char** argv)
{
	GLMotor_Surface_t *window = NULL;
	int mode = 0;
	int opt;
	optind = 1;
	do
	{
		opt = getopt(argc, argv, "O");
		switch (opt)
		{
			case 'O':
				mode = 1;
			break;
		}
	} while (opt != -1);
	glViewport(0, 0, config->width, config->height);

	if (mode == 0)
		window = surface_create(config, argc, argv);

	GLuint fbo = 0, rbo_color = 0, rbo_depth = 0, rbo_stencil = 0, tex = 0;
	if (window == NULL)
	{
		GLuint glget = 0;
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glget);
		if (glget <= config->width)
			warn("glmotor: width to large max %d", glget);
		if (glget <= config->height)
			warn("glmotor: width to height max %d", glget);

		/*  Framebuffer */
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

#if 0
		/* Color renderbuffer. */
		glGenRenderbuffers(1, &rbo_color);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo_color);
		/* Storage must be one of: */
		/* GL_RGBA4, GL_RGB565, GL_RGB5_A1, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX8. */
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, config->width, config->height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo_color);
#else
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, config->width, config->height, 0,
					GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
#endif

		/* Sanity check. */
		GLint ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (ret != GL_FRAMEBUFFER_COMPLETE)
		{
			err("glmotor: offscreen generator failed");
			return NULL;
		}
	}
	GLMotor_t *motor = calloc(1, sizeof(*motor));
	motor->width = config->width;
	motor->height = config->height;
	motor->surf = window;
	motor->off.fbo = fbo;
	motor->off.rbo_color = rbo_color;
	motor->off.rbo_depth = rbo_depth;
	motor->off.texture = tex;
	return motor;
}

static void display_log(GLuint instance)
{
	GLint logSize = 0;
	GLchar* log = NULL;

	if (glIsProgram(instance))
		glGetProgramiv(instance, GL_INFO_LOG_LENGTH, &logSize);
	else
		glGetShaderiv(instance, GL_INFO_LOG_LENGTH, &logSize);
	log = (GLchar*)malloc(logSize);
	if ( log == NULL )
	{
		err("segl: Log memory allocation error %m");
		return;
	}
	if (glIsProgram(instance))
		glGetProgramInfoLog(instance, logSize, NULL, log);
	else
		glGetShaderInfoLog(instance, logSize, NULL, log);
	err("%s", log);
	free(log);
}

static void deleteProgram(GLuint programID, GLuint fragmentID, GLuint vertexID)
{
	if (programID)
	{
		glUseProgram(0);

		glDetachShader(programID, fragmentID);
		glDetachShader(programID, vertexID);

		glDeleteProgram(programID);
	}
	if (fragmentID)
		glDeleteShader(fragmentID);
	if (vertexID)
		glDeleteShader(vertexID);
}

static GLuint load_shader(GLenum type, GLchar *source, GLuint size)
{
	GLuint shaderID;

	shaderID = glCreateShader(type);
	if (shaderID == 0)
	{
		err("glmotor: shader creation error");
		return 0;
	}

	if (size == -1)
		glShaderSource(shaderID, 1, (const GLchar**)(&source), NULL);
	else
		glShaderSource(shaderID, 1, (const GLchar**)(&source), &size);
	glCompileShader(shaderID);

	GLint compilationStatus = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compilationStatus);
	if ( compilationStatus != GL_TRUE )
	{
		err("glmotor: shader compilation error");
		display_log(shaderID);
		glDeleteShader(shaderID);
		return 0;
	}
	return shaderID;
}

GLMOTOR_EXPORT GLuint glmotor_build(GLMotor_t *motor, GLchar *vertexSource, GLuint vertexSize, GLchar *fragmentSource, GLuint fragmentSize)
{
#ifdef HAVE_GLEW
	glewExperimental = 1;
	if (init_glew())
		return -1;
#endif
	warn("glmotor uses : %s", glGetString(GL_VERSION));
	warn("glmotor uses : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
#ifdef DEBUG
	dbg("glmotor: gl extensions");
	const char * extensions = glGetString(GL_EXTENSIONS);
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

	GLuint vertexID = load_shader(GL_VERTEX_SHADER, vertexSource, vertexSize);
	if (vertexID == 0)
		return 0;
	GLuint fragmentID = load_shader(GL_FRAGMENT_SHADER, fragmentSource, fragmentSize);
	if (fragmentID == 0)
		return 0;

	GLuint programID = glCreateProgram();

	glAttachShader(programID, vertexID);
	glAttachShader(programID, fragmentID);

	glBindAttribLocation(motor->programID, 0, "vPosition");

	glLinkProgram(programID);

	GLint programState = 0;
	glGetProgramiv(programID , GL_LINK_STATUS  , &programState);
	if ( programState != GL_TRUE)
	{
		display_log(programID);
		deleteProgram(programID, fragmentID, vertexID);
		return -1;
	}

	glDetachShader(programID, vertexID);
	glDetachShader(programID, fragmentID);

	glDeleteShader(vertexID);
	glDeleteShader(fragmentID);

	glUseProgram(programID);

	motor->programID = programID;

	return programID;
}

GLMOTOR_EXPORT void glmotor_seteventcb(GLMotor_t *motor, GLMotor_Event_func_t cb, void * cbdata)
{
	motor->eventcb = cb;
	motor->eventcbdata = cbdata;
}

GLMOTOR_EXPORT GLuint glmotor_newevent(GLMotor_t *motor, GLMotor_Event_t event)
{
	GLMotor_Event_t *evt = calloc(1, sizeof(*evt));
	memmove(evt, &event, sizeof(*evt));
	evt->next = motor->events;
	motor->events = evt;
	return 0;
}

#ifndef GLUT
GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata)
{
	int ret = 0;
	do
	{
		draw(drawdata);
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
		glmotor_swapbuffers(motor);
		if (motor->eventcb && motor->events)
		{
			GLMotor_Event_t *evt = NULL;
			for (evt = motor->events; evt != NULL; evt = evt->next)
				motor->eventcb(motor->eventcbdata, evt);
			free(evt);
			motor->events = evt;
		}
		if (motor->surf)
			ret = surface_running(motor->surf, motor);
	} while (ret);
	return 0;
}

GLMOTOR_EXPORT GLuint glmotor_swapbuffers(GLMotor_t *motor)
{
	if (motor->surf)
		return surface_swapbuffers(motor->surf);
	glFlush();
	return 0;
}
#endif

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	if (motor->surf)
		surface_destroy(motor->surf);
	free(motor);
}
