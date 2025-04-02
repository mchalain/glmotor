#include <stddef.h>
#include <string.h>
#include <stdlib.h>
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
#ifdef HAVE_LIBINPUT
#include "input.h"
#endif
#ifdef HAVE_LIBCONFIG
#include <libconfig.h>

static config_t configfile = {0};
#endif

#define MODE_INTERACTIV 0x01

#ifdef HAVE_LIBCONFIG
static void program_parsesetting(config_setting_t *parser, GLMotor_config_t *config)
{
	const char *value;
	if (config_setting_lookup_string(parser, "vertex", &value) == CONFIG_TRUE)
	{
		config->vertexshader = value;
	}
	if (config_setting_lookup_string(parser, "fragment", &value) == CONFIG_TRUE)
	{
		config->fragmentshader[0] = value;
	}
	config_setting_t *fragments = config_setting_lookup(parser, "fragments");
	if (fragments != NULL)
	{
		for (config->nbfragmentshaders; config->nbfragmentshaders < MAX_FRAGS &&
				config->nbfragmentshaders < config_setting_length(fragments); config->nbfragmentshaders++)
		{
			config->fragmentshader[config->nbfragmentshaders] = config_setting_get_string_elem(fragments, config->nbfragmentshaders);
		}
	}
}
#endif

static int main_parseconfig(const char *file, GLMotor_config_t *config)
{
	const char *value;
#ifdef HAVE_LIBCONFIG
	config_init(&configfile);
	if (config_read_file(&configfile, file) != CONFIG_TRUE)
	{
		err("glmotor: Configuration file error");
		return -1;
	}
	GLuint intvalue;
	if (config_lookup_int(&configfile, "width", &intvalue) == CONFIG_TRUE)
	{
		config->width = intvalue;
	}
	if (config_lookup_int(&configfile, "height", &intvalue) == CONFIG_TRUE)
	{
		config->height = intvalue;
	}
	program_parsesetting(config_root_setting(&configfile), config);
	if (config_lookup_string(&configfile, "object", &value) == CONFIG_TRUE)
	{
		config->object = value;
	}
	if (config_lookup_string(&configfile, "texture", &value) == CONFIG_TRUE)
	{
		config->texture = value;
	}

#endif
}

static void render(void *data)
{
	GLMotor_Scene_t *scene = (GLMotor_Scene_t *)data;
	GLMotor_Object_t *obj = scene_nextobject(scene, NULL);
	for (; obj != NULL; obj = scene_nextobject(scene, obj))
	{
		GLMotor_Rotate_t *rot = NULL;
		GLMotor_Translate_t *tr = NULL;
		if (object_kinematic(obj, &tr, &rot) != -1)
		{
			object_move(obj, tr, rot);
		}
	}
	scene_draw(scene);
}

GLMotor_Object_t *load_staticobject(GLMotor_t *motor, const char *name, GLuint programID)
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
	object_setprogram(obj, programID);
	object_appendpoint(obj, size, &vVertices[0]);
	object_appendcolor(obj, ncolors, &vColors[0]);

	object_appendface(obj, nfaces, &vFaces[0]);
	return obj;
}

int main(int argc, char** argv)
{
	GLMotor_config_t config = {
		.vertexshader = PKG_DATADIR"/simple.vert",
		.fragmentshader = {PKG_DATADIR"/simple.frag", NULL, NULL, NULL},
		.nbfragmentshaders = 0,
		.object = NULL,
		.texture = NULL,
		.width = 640,
		.height = 480,
	};
	int mode = 0;
	int opt;
	opterr = 0;
	do
	{
		opt = getopt(argc, argv, "-W:H:o:v:f:C:t:i");
		switch (opt)
		{
			case 'W':
				config.width = atoi(optarg);
			break;
			case 'H':
				config.height = atoi(optarg);
			break;
			case 'C':
				main_parseconfig(optarg, &config);
			break;
			case 'o':
				config.object = optarg;
			break;
			case 'v':
				config.vertexshader = optarg;
			break;
			case 'f':
				if (config.nbfragmentshaders < MAX_FRAGS)
					config.fragmentshader[config.nbfragmentshaders ++] = optarg;
			break;
			case 't':
				config.texture = optarg;
			break;
			case 'i':
				mode = MODE_INTERACTIV;
			break;
		}
	} while (opt != -1);
	if (config.nbfragmentshaders == 0)
		config.nbfragmentshaders = 1;

	GLMotor_t *motor = glmotor_create(&config, argc, argv);

	if (motor == NULL)
		return -1;
	GLuint programID = glmotor_load(motor, config.vertexshader, config.fragmentshader, config.nbfragmentshaders);

	GLMotor_Object_t *obj = NULL;
	if (config.object == NULL)
		obj = load_staticobject(motor, "object", programID);
	else
		obj = object_load(motor, "object", config.object, programID);
	if (obj == NULL)
	{
		err("glmotor: impossible to create object");
		glmotor_destroy(motor);
		return 1;
	}

	GLMotor_Texture_t *tex = NULL;
	if (config.texture)
	{
		tex = texture_load(motor, config.texture);
	}

	if (tex)
		object_addtexture(obj, tex);

	GLMotor_Scene_t *scene = scene_create(motor);
	if (scene == NULL)
	{
		err("glmotor: impossible to create scene");
		glmotor_destroy(motor);
		return 1;
	}
	scene_appendobject(scene, obj);

#ifdef HAVE_LIBINPUT
	GLMotor_Input_t *input = NULL;
	if (mode & MODE_INTERACTIV)
		input = input_create(scene);
#endif
	glmotor_run(motor, render, scene);
	scene_destroy(scene);
	glmotor_destroy(motor);
#ifdef HAVE_LIBINPUT
	if (input)
		input_destroy(input);
#endif
}
