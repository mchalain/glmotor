#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
# undef HAVE_GLEW
#else
#ifdef HAVE_GLEW
# include <GL/glew.h>
#endif
# include <GL/gl.h>
#endif
#ifdef HAVE_GLU
#include <GL/glu.h>
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
	const char * extensions = glGetString(GL_EXTENSIONS);
	dbg("glmotor uses extensions:\n%s", extensions);
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
