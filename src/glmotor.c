#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
# ifdef HAVE_EGL
#  include <EGL/egl.h>
# endif
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
#else
PFNGLBINDVERTEXARRAYOESPROC glBindVertexArray = NULL;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArrays = NULL;

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
	return 0;
}
#endif

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
#elif defined(HAVE_EGL)
	if (init_egl())
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
