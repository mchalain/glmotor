#include <stddef.h>
#include <math.h>
#include <unistd.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
#else
# include <GL/gl.h>
#endif
#include "glmotor.h"
#include "loader.h"
#include "log.h"

static GLfloat g_angle = 0.0;
static GLfloat g_camera[] = {0.0, 0.0, 100.0};

typedef struct GLMotor_config_s GLMotor_config_t;
struct GLMotor_config_s
{
	const char *vertexshader;
	const char *fragmentshader;
	const char *object;
};

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
	GLMotor_Rotate_t *rot = NULL;
	GLMotor_Translate_t *tr = NULL;
	if (object_kinematic(obj, &tr, &rot) != -1)
	{
		object_move(obj, tr, rot);
	}
	scene_draw(scene);
}

int parseevent(void *data, GLMotor_Event_t *event)
{
	GLMotor_Scene_t *scene = (GLMotor_Scene_t *)data;
	GLMotor_Object_t *obj = scene_getobject(scene, "object");
	GLMotor_Rotate_t rotate = { 0 };
	rotate.ra.X = 0; rotate.ra.Y = 0; rotate.ra.Z = 0; rotate.ra.A = M_PI_4/100;
	GLMotor_Rotate_t *rot = NULL;
	GLMotor_Translate_t translate = {.X = 0.0, .Y = 0.0, .Z = 0.0, .L = 0.01};
	GLMotor_Translate_t *tr = NULL;
	//const GLfloat *move = object_positionmatrix(obj);
	if (event->type == EVT_KEY)
	{
		switch (event->data.key.code)
		{
		case 0x71:
			if (event->data.key.mode & MODE_SHIFT)
				rotate.ra.Y = 1;
			else if (event->data.key.mode & MODE_CTRL)
				rotate.ra.Z = 1;
			else
				translate.X = 1;
		break;
		case 0x72:
			rotate.ra.A *= -1;
			translate.L = - translate.L;
			if (event->data.key.mode & MODE_SHIFT)
				rotate.ra.Y = 1;
			else if (event->data.key.mode & MODE_CTRL)
				rotate.ra.Z = 1;
			else
				translate.X = 1;
		break;
		case 0x6F:
			if (event->data.key.mode & MODE_SHIFT)
				rotate.ra.X = 1;
			else if (event->data.key.mode & MODE_CTRL)
				translate.Z = 1;
			else
				translate.Y = 1;
		break;
		case 0x74:
			rotate.ra.A *= -1;
			translate.L *= -1;
			if (event->data.key.mode & MODE_SHIFT)
				rotate.ra.X = 1;
			else if (event->data.key.mode & MODE_CTRL)
				translate.Z = 1;
			else
				translate.Y = 1;
		break;
		}
		if (rotate.ra.X || rotate.ra.Y || rotate.ra.Z)
			rot = &rotate;
		if (translate.X || translate.Y || translate.Z)
			tr = &translate;
	}
	if (rot || tr)
		object_move(obj, tr, rot);
	return 0;
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
	GLMotor_config_t config = {
		.vertexshader = PKG_DATADIR"/simple.vert",
		.fragmentshader = PKG_DATADIR"/simple.frag",
		.object = NULL,
	};
	int opt;
	opterr = 0;
	do
	{
		opt = getopt(argc, argv, "-o:v:f:");
		switch (opt)
		{
			case 'o':
				config.object = optarg;
			break;
			case 'v':
				config.vertexshader = optarg;
			break;
			case 'f':
				config.fragmentshader = optarg;
			break;
		}
	} while (opt != -1);

	GLMotor_t *motor = glmotor_create(argc, argv);

	if (motor == NULL)
		return -1;

	glmotor_load(motor, config.vertexshader, config.fragmentshader);

	GLMotor_Scene_t *scene = scene_create(motor);

#if 0
	GLfloat target[] = {0.0, 0.0, 0.0};
	scene_movecamera(scene, g_camera, target);
#endif

	GLMotor_Object_t *obj = NULL;
	if (config.object == NULL)
		obj = load_staticobject(motor, "object");
	else
		obj = object_load(motor, "object", config.object);
	if (obj != NULL)
	{
		scene_appendobject(scene, obj);
	}
#if 1
	glmotor_seteventcb(motor, parseevent, scene);
#else
	GLMotor_Rotate_t rotate = { 0 };
	rotate.ra.X = 0; rotate.ra.Y = 0; rotate.ra.Z = 1; rotate.ra.A = M_PI_4/100;
	object_appendkinematic(obj, NULL, NULL, -50);
	object_appendkinematic(obj, NULL, &rotate, -100);
	object_appendkinematic(obj, NULL, NULL, -50);
	rotate.ra.A *= -1;
	object_appendkinematic(obj, NULL, &rotate, -100);
	object_appendkinematic(obj, NULL, NULL, 50);
	rotate.ra.A *= -1;
	object_appendkinematic(obj, NULL, &rotate, 50);
#endif
	glmotor_run(motor, render, scene);
	scene_destroy(scene);
	glmotor_destroy(motor);
}
