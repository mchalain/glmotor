#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
#else
# include <GL/gl.h>
#endif
#ifdef HAVE_GLU
#include <GL/glu.h>
#endif

#include "glmotor.h"
#include "log.h"

/***********************************************************************
 * GLMotor_Scene_t
 **********************************************************************/
struct GLMotor_Scene_s
{
	GLMotor_t *motor;
	GLMotor_list_t *objects;
};

GLMOTOR_EXPORT GLMotor_Scene_t *scene_create(GLMotor_t *motor)
{
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glViewport(0, 0, motor->width, motor->height);
	glEnable(GL_DEPTH_TEST);
#ifdef HAVE_GLESV2
	glClearDepthf(1.0);
#else
	glClearDepth(1.0);
#endif
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifndef HAVE_GLESV2
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#ifdef HAVE_GLU
	gluPerspective(atan(tan(50.0 * 3.14159 / 360.0) / 1.0) * 360.0 / 3.141593, (GLfloat)motor->width / (GLfloat)motor->height, 0.1, 100.0);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
	GLMotor_Scene_t *scene;
	scene = calloc(1, sizeof(*scene));
	scene->motor = motor;
	return scene;
}

GLMOTOR_EXPORT void scene_movecamera(GLMotor_Scene_t *scene, const GLfloat *camera, const GLfloat *target)
{
	const GLfloat defaultTarget[] = {0.0, 0.0, 0.0}; //target the center of the world
	const GLfloat *applyTarget = defaultTarget;
	if (target != NULL)
		applyTarget = target;
#ifndef HAVE_GLESV2
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
#ifdef HAVE_GLU
	gluLookAt(camera[0], camera[1], camera[2],
			applyTarget[0], applyTarget[1], applyTarget[2],
			0, 1, 0);
#endif
}

GLMOTOR_EXPORT void scene_appendobject(GLMotor_Scene_t *scene, GLMotor_Object_t *obj)
{
	GLMotor_list_t *entry = calloc(1, sizeof(entry));
	entry->entity = obj;
	entry->next = scene->objects;
	scene->objects = entry;
}

GLMOTOR_EXPORT GLMotor_Object_t *scene_getobject(GLMotor_Scene_t *scene, const char *name)
{
	GLMotor_Object_t *obj = NULL;
	for (GLMotor_list_t *entry = scene->objects; entry != NULL; entry = entry->next)
	{
		if (!strcmp(object_name((GLMotor_Object_t *)entry->entity), name))
		{
			obj = entry->entity;
			break;
		}
	}
	return obj;
	
}

GLMOTOR_EXPORT GLint scene_draw(GLMotor_Scene_t *scene)
{
	GLint ret = 0;
	GLMotor_t *motor = scene->motor;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#ifndef HAVE_GLESV2
	glLoadIdentity();
#endif
	glUseProgram(motor->programID);

	for (GLMotor_list_t *it = scene->objects; it != NULL; it = it->next)
	{
		ret = object_draw((GLMotor_Object_t *)it->entity);
		if (ret)
			break;
	}
	return ret;
}

GLMOTOR_EXPORT void scene_destroy(GLMotor_Scene_t *scene)
{
	for (GLMotor_list_t *it = scene->objects, *next = it->next; it != NULL; it = next, next = it->next)
	{
		object_destroy(it->entity);
		free(it);
		if (! next)
			break;
	}
	free(scene);
}
