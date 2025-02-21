#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define GLMOTOR_SURFACE_S GLFWwindow
#include "glmotor.h"

#include "log.h"

GLMOTOR_EXPORT GLMotor_Surface_t *surface_create(GLMotor_config_t *config, int argc, char** argv);
{
	GLchar *name = "GLMotor";

	int opt;
	optind = 1;
	do
	{
		opt = getopt(argc, argv, "n:");
		switch (opt)
		{
			case 'n':
				name = optarg;
			break;
		}
	} while (opt != -1);

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
	window = glfwCreateWindow(config->width, config->height, name, NULL, NULL);
	if (window == NULL)
	{
		err("glmotor: Window creation error");
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);

	glfwSetInputMode(motor->surf, GLFW_STICKY_KEYS, GL_TRUE);
	return window;
}

GLMOTOR_EXPORT int surface_running(GLMotor_Surface_t *surf, GLMotor_t *motor)
{
	glfwPollEvents();
	return glfwGetKey(surf, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
			glfwWindowShouldClose(surf) == 0;
}

GLMOTOR_EXPORT GLuint surface_swapbuffers(GLMotor_t *motor)
{
	glfwSwapBuffers(motor->surf);
	return 0;
}

GLMOTOR_EXPORT void surface_destroy(GLMotor_Surface_t *window)
{
	glfwTerminate();
}
