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
	GLfloat view[16];
	GLfloat *pview;
	GLfloat background[4];
	GLfloat scale;
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
	/// this is good for 3D object but not on plane object
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
#else
	glDisable(GL_CULL_FACE);
#endif
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLMotor_Scene_t *scene;
	scene = calloc(1, sizeof(*scene));
	scene->motor = motor;
	scene->buffermask = buffermask;
	scene_setresolution(scene, motor->width, motor->height);
	mat4_diag(scene->view);
	scene->scale = 1.0;
	return scene;
}

#if SCENE_MOVECAMERA == y
GLMOTOR_EXPORT void scene_movecamera(GLMotor_Scene_t *scene, const GLfloat eye[3], const GLfloat center[3])
{
	GLfloat up[3] = {0.0,1.0,0.0};
	if (eye[2] != 0.0)
	{
		GLfloat *perspective = scene->view;
		GLfloat tmp[16];
		mat4_diag(tmp);
		if (scene->pview)
			perspective = tmp;
		mat4_lookat(eye, center, up, perspective);
		if (perspective == tmp)
			mat4_multiply4(tmp, scene->view, scene->view);
		scene->pview = scene->view;
	}
dbg("move camera");
mat4_log(scene->view);
}

static void _scene_perspective(GLMotor_Scene_t *scene, const GLfloat angle, const GLfloat near, const GLfloat far, GLfloat *perspective)
{
	GLfloat aspect = ((GLfloat)scene->motor->width) / ((GLfloat)scene->motor->height);
	mat4_perspective(angle, aspect, near, far, perspective);
}

static void _scene_ortho(GLMotor_Scene_t *scene, const GLfloat near, const GLfloat far, GLfloat *perspective)
{
	GLMotor_t * motor = scene->motor;
	GLfloat aspect = ((GLfloat)scene->motor->width) / ((GLfloat)scene->motor->height);
	GLfloat frame[4] = {-1 / (2.0 * aspect), 1 / (2.0 * aspect), -1 / (2.0 * aspect), 1 / (2.0 * aspect)};
	mat4_ortho(frame, near, far, perspective);
}

GLMOTOR_EXPORT void scene_perspective(GLMotor_Scene_t *scene, const GLfloat angle, const GLfloat near, const GLfloat far)
{
	GLfloat *perspective = scene->view;
	GLfloat tmp[16];
	mat4_diag(tmp);
	if (scene->pview)
		perspective = tmp;
	if (angle != 0.0)
		_scene_perspective(scene, angle, near, far, perspective);
	else if (far != near)
		_scene_ortho(scene, near, far, perspective);
	else
		mat4_diag(perspective);
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

GLMOTOR_EXPORT void scene_zoom(GLMotor_Scene_t *scene, const GLfloat scale)
{
	if (((scene->scale + scale) <= 0.01) ||
		((scene->scale + scale) >= 2.0))
		return;
	scene->scale += scale;
	GLfloat *perspective = scene->view;
	GLfloat tmp[16];
//	if (scene->pview)
//		perspective = tmp;
	mat4_scale(scene->scale, perspective);
//	if (perspective == tmp)
//		mat4_multiply4(tmp, scene->view, scene->view);
	scene->pview = scene->view;
}

GLMOTOR_EXPORT void scene_setbackground(GLMotor_Scene_t *scene, GLfloat color[4])
{
	memcpy(scene->background, color, sizeof(scene->background));
	glClearColor(scene->background[0], scene->background[1], scene->background[2], scene->background[3]);
}

static GLuint scene_setresolution(GLMotor_Scene_t *scene, GLuint width, GLuint height)
{
	GLMotor_t * motor = scene->motor;
	GLfloat vResolution[] = {
		 width, height, 1.0f, 1.0f,
	};
	vResolution[2] /= (float)width;
	vResolution[3] /= (float)height;
	for (int i = 0; i < motor->nbprograms; i++)
	{
		GLuint resID = glGetUniformLocation(motor->programID[i], "vResolution");
		glUniform4fv(resID, 1, vResolution);
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

GLMOTOR_EXPORT void scene_reset(GLMotor_Scene_t *scene)
{
	scene->scale = 1.0;
	mat4_diag(scene->pview);
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
