#include <stddef.h>
#include <math.h>
#include <unistd.h>

#include <GL/gl.h>
#include "glmotor.h"
#include "loader.h"
#include "log.h"

static GLfloat g_angle = 0.0;
static GLfloat g_camera[] = {0.0, 0.0, 100.0};

static void render(void *data)
{
	GLMotor_Scene_t *scene = (GLMotor_Scene_t *)data;
#if 0

	g_camera[0] += 5.0 * sin(g_angle);
	g_camera[2] += 5.0 * cos(g_angle);
	g_angle += M_2_PI / 100;
	scene_movecamera(scene, g_camera, NULL);
#endif
	GLMotor_Object_t *obj = scene_getobject(scene, "object");
	GLMotor_RotAxis_t rotate = { .X = 0, .Y = 0, .Z = 1, .A = M_PI_4/100 };
	GLfloat translate[3] = {0.01, 0.01, 0};
	GLfloat *tr = NULL;
	//tr = translate;
	GLMotor_RotAxis_t *rot = NULL;
	rot = &rotate;
	object_move(obj, tr, rot);

	scene_draw(scene);
}

GLMotor_Object_t *load_staticobject(GLMotor_t *motor, const char *name)
{
	GLMotor_Object_t *obj = NULL;
	GLfloat vVertices[] = {
		 0.000f,  0.500f, -0.250f, // A
		-0.433f, -0.250f, -0.250f, // B
		 0.433f, -0.250f, -0.250f, // C

		 0.000f,  0.000f,  0.500f, // D
	};
	GLuint size = sizeof(vVertices) / sizeof(GLfloat) / 3;
	GLfloat vColors[] = {

		 1.0f, 0.0f, 0.0f, //1.0f, // A
		 0.0f, 1.0f, 0.0f, //1.0f, // B
		 0.0f, 0.0f, 1.0f, //1.0f, // C

		 0.0f, 0.0f, 0.0f, //1.0f, // D
	};
	GLuint ncolors = sizeof(vColors) / sizeof(GLfloat) / COLOR_COMPONENTS;
	GLuint vFaces[] = {
		0, 1, 2,
		0, 2, 3,
		0, 1, 3,
		1, 2, 3,
	};
	GLuint nfaces = sizeof(vFaces) / sizeof(GLuint) / 3;

	obj = object_create(motor, name, size, nfaces);
	object_appendpoint(obj, size, &vVertices[0]);
	object_appendcolor(obj, ncolors, &vColors[0]);

	object_appendface(obj, nfaces, &vFaces[0]);
	return obj;
}

int main(int argc, char** argv)
{

	const char *vertexshader = "data/simple.vert";
	const char *fragmentshader = "data/simple.frag";
	const char *object = NULL;
	int opt;
	opterr = 0;
	do
	{
		opt = getopt(argc, argv, "-o:v:f:");
		switch (opt)
		{
			case 'o':
				object = optarg;
			break;
			case 'v':
				vertexshader = optarg;
			break;
			case 'f':
				fragmentshader = optarg;
			break;
		}
	} while (opt != -1);

	GLMotor_t *motor = glmotor_create(argc, argv);

	if (motor == NULL)
		return -1;
	
	glmotor_load(motor, vertexshader, fragmentshader);

	GLMotor_Scene_t *scene = scene_create(motor);

#if 0
	GLfloat target[] = {0.0, 0.0, 0.0};
	scene_movecamera(scene, g_camera, target);
#endif

	GLMotor_Object_t *obj = NULL;
	if (object == NULL)
		obj = load_staticobject(motor, "object");
	else
		obj = object_load(motor, "object", object);
	if (obj != NULL)
	{
		scene_appendobject(scene, obj);
	}
	glmotor_run(motor, render, scene);
	scene_destroy(scene);
	glmotor_destroy(motor);
}
