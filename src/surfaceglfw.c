#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef HAVE_GLEW
# include <GLES2/gl2.h>
#else
# include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

#define GLMOTOR_SURFACE_S GLFWwindow
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

GLMOTOR_EXPORT GLMotor_t *glmotor_create(int argc, char** argv)
{
	GLuint width = 640;
	GLuint height = 480;
	GLchar *name = "GLMotor";

	if (!glfwInit())
	{
		err("glmotor: Window manager error");
		return NULL;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window;
	window = glfwCreateWindow(width, height, name, NULL, NULL);
	if (window == NULL)
	{
		err("glmotor: Window creation error");
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);
#ifdef HAVE_GLEW
	glewExperimental = true;
	if (init_glew())
		return NULL;
#endif

	GLMotor_t *motor = calloc(1, sizeof(*motor));
	motor->width = width;
	motor->height = height;
	motor->surf = window;

	return motor;
}

GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata)
{
	glfwSetInputMode(motor->surf, GLFW_STICKY_KEYS, GL_TRUE);
	do
	{
		draw(drawdata);
		glfwSwapBuffers(motor->surf);
		glfwPollEvents();
	}
	while (glfwGetKey(motor->surf, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
			glfwWindowShouldClose(motor->surf) == 0 );
	return 0;
}

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	glfwTerminate();
	free(motor);
}
