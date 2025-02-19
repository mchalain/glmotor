#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_GLESV2
# error "glut doesn't support gles"
#endif
#include <GL/gl.h>
#include <GL/freeglut.h>

#include "glmotor.h"
#include "log.h"

GLMOTOR_EXPORT GLMotor_t *glmotor_create(GLMotor_config_t *config, int argc, char** argv)
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

	glutInit(&argc, argv);

	glutInitWindowPosition(-1, -1);
	glutInitWindowSize(config->width, config->height);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE |GLUT_DEPTH);

	glutCreateWindow(name);

	GLMotor_t *motor = calloc(1, sizeof(*motor));
	motor->width = config->width;
	motor->height = config->height;

	return motor;
}

static void keyboardFeedback(unsigned char key, int x, int y)
{
	if ( key == 27 ) // Escape
	{
		glutLeaveMainLoop();
	}
}

static GLMotor_Draw_func_t glut_draw = NULL;
static void *glut_drawdata = NULL;

static void render(void)
{
	if (glut_draw != NULL)
		glut_draw(glut_drawdata);
	glutSwapBuffers();
}

GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata)
{
	glut_draw = draw;
	glut_drawdata = drawdata;

	glutKeyboardFunc(keyboardFeedback);
	glutDisplayFunc(render);
	//glutIdleFunc(render);

	glutMainLoop();
	return 0;
}

GLMOTOR_EXPORT GLuint glmotor_swapbuffers(GLMotor_t *motor)
{
	err("glmotor: swapbuffers unsupported with glut");
	return -1;
}

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	free(motor);
}
