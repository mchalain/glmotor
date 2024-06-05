#include <stddef.h>
#include <math.h>
#include <unistd.h>

#include "glmotor.h"
#include "loader.h"

static GLfloat g_angle = 0.0;
static GLfloat g_camera[] = {0.0, 0.0, 5.0};

static void render(void *data)
{
	GLMotor_Scene_t *scene = (GLMotor_Scene_t *)data;

#if 0
	g_camera[0] += 5.0 * sin(g_angle);
	g_camera[2] += 5.0 * cos(g_angle);
	g_angle += M_2_PI / 100;
	move_camera(scene, g_camera, NULL);
#endif

	draw_scene(scene);
}

int main(int argc, char** argv)
{
	const char *vertexshader = "data/simple.vert";
	const char *fragmentshader = "data/simple.frag";
	const char *object = "data/cube.obj";
	int opt;
	do
	{
		opt = getopt(argc, argv, "o:v:f:");
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
	
	GLMotor_Scene_t *scene = create_scene(motor);

	GLfloat target[] = {0.0, 0.0, 0.0};
	move_camera(scene, g_camera, target);

	load_shader(motor, vertexshader, fragmentshader);

	GLMotor_Object_t *obj = load_obj(motor, object);
	append_object(scene, obj);

	glmotor_run(motor, render, scene);
	destroy_scene(scene);
	glmotor_destroy(motor);
}
