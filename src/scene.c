#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
#else
# ifdef HAVE_GLEW
#  include <GL/glew.h>
# endif
# include <GL/gl.h>
# include <GL/glext.h>
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
	GLuint buffermask;
};
static GLuint scene_setresolution(GLMotor_Scene_t *scene, GLuint width, GLuint height);

GLMOTOR_EXPORT GLMotor_Scene_t *scene_create(GLMotor_t *motor)
{
	GLuint buffermask = GL_COLOR_BUFFER_BIT;
#if GLMOTOR_DEPTH_BUFFER
	buffermask |= GL_DEPTH_BUFFER_BIT;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
#endif

#ifdef HAVE_GLESV2
	glClearDepthf(1.0);
#else
	glClearDepth(1.0);
#endif

	glClearColor(1.0, 1.0, 1.0, 1.0);
#if GLMOTOR_STENCIL_BUFFER
	buffermask |= GL_STENCIL_BUFFER_BIT;
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glClearStencil(0);
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
#endif
	glClear(buffermask);

#ifndef HAVE_GLESV2
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#ifdef HAVE_GLU
	gluPerspective(atan(tan(50.0 * 3.14159 / 360.0) / 1.0) * 360.0 / 3.141593, (GLfloat)motor->width / (GLfloat)motor->height, 0.1, 100.0);
#endif
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif

#if 0
	/// this is good for £D object but not on plane object
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
#endif

	GLMotor_Scene_t *scene;
	scene = calloc(1, sizeof(*scene));
	scene->motor = motor;
	scene->buffermask = buffermask;
	scene_setresolution(scene, motor->width, motor->height);
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

static GLuint scene_setresolution(GLMotor_Scene_t *scene, GLuint width, GLuint height)
{
	GLMotor_t * motor = scene->motor;
	GLfloat vResolution[] = {

		 1.0f, 1.0f,
	};
	vResolution[0] /= (float)width;
	vResolution[1] /= (float)height;
	for (int i = 0; i < motor->nbprograms; i++)
	{
		GLuint resID = glGetUniformLocation(motor->programID[i], "vResolution");
		glUniform2fv(resID, 1, vResolution);
	}

	return 0;
}

GLMOTOR_EXPORT void scene_appendobject(GLMotor_Scene_t *scene, GLMotor_Object_t *obj)
{
	GLMotor_list_t *entry = calloc(1, sizeof(*entry));
	entry->entity = obj;
	entry->next = scene->objects;
	scene->objects = entry;
	if(glBindVertexArray)
		glBindVertexArray(0);
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

GLMOTOR_EXPORT GLMotor_Object_t *scene_nextobject(GLMotor_Scene_t *scene, GLMotor_Object_t *prev)
{
	if (prev == NULL)
		return scene->objects->entity;
	for (GLMotor_list_t *entry = scene->objects; entry != NULL; entry = entry->next)
	{
		if (entry->entity == prev && entry->next)
			return entry->next->entity;
	}
	return NULL;
}

GLMOTOR_EXPORT GLint scene_draw(GLMotor_Scene_t *scene)
{
	GLint ret = 0;
	GLMotor_t *motor = scene->motor;
	glClear(scene->buffermask);
#ifndef HAVE_GLESV2
	glLoadIdentity();
#endif

	for (GLMotor_list_t *it = scene->objects; it != NULL; it = it->next)
	{
		ret = object_draw((GLMotor_Object_t *)it->entity);
		if (ret)
			break;
	}
	return ret;
}

GLMOTOR_EXPORT GLuint scene_width(GLMotor_Scene_t *scene)
{
	return scene->motor->width;
}

GLMOTOR_EXPORT GLuint scene_height(GLMotor_Scene_t *scene)
{
	return scene->motor->height;
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
