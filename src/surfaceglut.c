#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#define GLMOTOR_SURFACE_S int
#include "glmotor.h"

#ifdef HAVE_GLESV2
# error "glut doesn't support gles"
#endif
#include <GL/freeglut.h>

#include "log.h"

GLMOTOR_EXPORT GLMotor_Surface_t *surface_create(GLMotor_config_t *config, int argc, char** argv)
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

	return (int)glutCreateWindow(name);
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

GLMOTOR_EXPORT void surface_destroy(GLMotor_Surface_t *window)
{
	glutDestroyWindow((int)window);
}
