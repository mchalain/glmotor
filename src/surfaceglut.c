#include <stddef.h>
#include <stdlib.h>

#ifdef HAVE_GLESV2
# error "glut doesn't support gles"
#endif
#ifdef HAVE_GLEW
# include <GL/glew.h>
#endif
#include <GL/freeglut.h>

#define GLMOTOR_SURFACE_S struct GLMotor_Surface_s
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

	glutInit(&argc, argv);

	glutInitWindowPosition(-1, -1);
	glutInitWindowSize(width, height);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE |GLUT_DEPTH);

	glutCreateWindow(name);
#ifdef HAVE_GLEW
	if (init_glew())
		return NULL;
#endif

	GLMotor_t *motor = calloc(1, sizeof(*motor));
	motor->width = width;
	motor->height = height;

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

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	free(motor);
}
