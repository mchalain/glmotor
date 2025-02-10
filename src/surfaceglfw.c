#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define GLMOTOR_SURFACE_S GLFWwindow
#include "glmotor.h"

#include "log.h"

GLMOTOR_EXPORT GLMotor_t *glmotor_create(int argc, char** argv)
{
	GLuint width = 640;
	GLuint height = 480;
	GLchar *name = "GLMotor";

	int opt;
	optind = 1;
	do
	{
		opt = getopt(argc, argv, "w:h:n:");
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
	window = glfwCreateWindow(width, height, name, NULL, NULL);
	if (window == NULL)
	{
		err("glmotor: Window creation error");
		glfwTerminate();
	}
	glfwMakeContextCurrent(window);

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
		glmotor_swapbuffers(motor);
		glfwPollEvents();
	}
	while (glfwGetKey(motor->surf, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
			glfwWindowShouldClose(motor->surf) == 0 );
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
	glfwSwapBuffers(motor->surf);
	return 0;
}

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	glfwTerminate();
	free(motor);
}
