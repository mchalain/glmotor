#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "glmotor.h"
#include "mat.h"
#include "log.h"

/***********************************************************************
 * GLMotor_Scene_t
 **********************************************************************/
typedef struct GLMotor_list_s GLMotor_list_t;
struct GLMotor_list_s
{
	void *entity;
	GLMotor_list_t *next;
};

struct GLMotor_Scene_s
{
	GLMotor_t *motor;
	GLMotor_list_t *objects;
	GLuint buffermask;
	GLfloat eye[3];
	GLfloat center[3];
	GLfloat up[3];
	GLfloat view[16];
	GLfloat *pview;
	GLfloat background[4];
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
	scene->eye[0] = 0.0;
	scene->eye[1] = 0.0;
	scene->eye[2] = 1.0;
	scene->center[0] = 0.0;
	scene->center[1] = 0.0;
	scene->center[2] = 0.0;
	scene->up[0] = 0.0;
	scene->up[1] = 1.0;
	scene->up[2] = 0.0;
	mat4_diag(scene->view);
	return scene;
}

#if SCENE_MOVECAMERA == y
GLMOTOR_EXPORT void scene_movecamera(GLMotor_Scene_t *scene, const GLfloat *camera, const GLfloat *target)
{
	if (camera != NULL)
	{
		scene->eye[0] = camera[0];
		scene->eye[1] = camera[1];
		scene->eye[2] = camera[2];
	}
	if (target != NULL)
	{
		scene->center[0] = target[0];
		scene->center[1] = target[1];
		scene->center[2] = target[2];
	}
#ifndef HAVE_GLESV2
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
	mat4_lookat(scene->eye, scene->center, scene->up, scene->view);
	scene->pview = scene->view;
}
GLMOTOR_EXPORT void scene_perspective(GLMotor_Scene_t *scene, const GLfloat angle, const GLfloat near, const GLfloat far)
{
	GLfloat aspect = ((GLfloat)scene->motor->width) / ((GLfloat)scene->motor->height);
	GLfloat *perspective = scene->view;
	GLfloat tmp[16];
	mat4_diag(tmp);
	if (scene->pview)
		perspective = tmp;
	mat4_perspective(angle, aspect, near, far, perspective);
	if (perspective == tmp)
		mat4_multiply4(tmp, scene->view, scene->view);
	scene->pview = scene->view;
}
#else
GLMOTOR_EXPORT void scene_movecamera(GLMotor_Scene_t *scene, const GLfloat *camera, const GLfloat *target)
{}
GLMOTOR_EXPORT void scene_perspective(GLMotor_Scene_t *scene, const GLfloat angle, const GLfloat near, const GLfloat far)
{}
#endif

GLMOTOR_EXPORT void scene_setbackground(GLMotor_Scene_t *scene, GLfloat color[4])
{
	memcpy(scene->background, color, sizeof(scene->background));
	glClearColor(scene->background[0], scene->background[1], scene->background[2], scene->background[3]);
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
		ret = object_draw((GLMotor_Object_t *)it->entity, scene->pview);
		scene->pview = NULL;
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
