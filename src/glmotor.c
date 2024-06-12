#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>

#ifndef HAVE_GLEW
# include <GLES2/gl2.h>
#else
# include <GL/glew.h>
#endif
#include <GL/glu.h>

#include "glmotor.h"
#include "log.h"

typedef struct GLMotor_list_s GLMotor_list_t;
struct GLMotor_list_s
{
	void *entity;
	GLMotor_list_t *next;
};

static void display_log(GLuint instance)
{
	GLint logSize = 0;
	GLchar* log = NULL;

	if (glIsProgram(instance))
		glGetProgramiv(instance, GL_INFO_LOG_LENGTH, &logSize);
	else
		glGetShaderiv(instance, GL_INFO_LOG_LENGTH, &logSize);
	log = (GLchar*)malloc(logSize);
	if ( log == NULL )
	{
		err("segl: Log memory allocation error %m");
		return;
	}
	if (glIsProgram(instance))
		glGetProgramInfoLog(instance, logSize, NULL, log);
	else
		glGetShaderInfoLog(instance, logSize, NULL, log);
	err("%s", log);
	free(log);
}

static void deleteProgram(GLuint programID, GLuint fragmentID, GLuint vertexID)
{
	if (programID)
	{
		glUseProgram(0);

		glDetachShader(programID, fragmentID);
		glDetachShader(programID, vertexID);

		glDeleteProgram(programID);
	}
	if (fragmentID)
		glDeleteShader(fragmentID);
	if (vertexID)
		glDeleteShader(vertexID);
}

static GLuint load_shader(GLenum type, GLchar *source, GLuint size)
{
	GLuint shaderID;

	shaderID = glCreateShader(type);
	if (shaderID == 0)
	{
		err("glmotor: shader creation error");
		return 0;
	}

	if (size == -1)
		glShaderSource(shaderID, 1, (const GLchar**)(&source), NULL);
	else
		glShaderSource(shaderID, 1, (const GLchar**)(&source), &size);
	glCompileShader(shaderID);

	GLint compilationStatus = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compilationStatus);
	if ( compilationStatus != GL_TRUE )
	{
		err("glmotor: shader compilation error");
		display_log(shaderID);
		glDeleteShader(shaderID);
		return 0;
	}
	return shaderID;
}

GLMOTOR_EXPORT GLuint glmotor_build(GLMotor_t *motor, GLchar *vertexSource, GLuint vertexSize, GLchar *fragmentSource, GLuint fragmentSize)
{
	warn("glmotor use : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	GLuint vertexID = load_shader(GL_VERTEX_SHADER, vertexSource, vertexSize);
	if (vertexID == 0)
		return 0;
	GLuint fragmentID = load_shader(GL_FRAGMENT_SHADER, fragmentSource, fragmentSize);
	if (fragmentID == 0)
		return 0;

	GLuint programID = glCreateProgram();

	glAttachShader(programID, vertexID);
	glAttachShader(programID, fragmentID);

	glBindAttribLocation(motor->programID, 0, "vPosition");

	glLinkProgram(programID);

	GLint programState = 0;
	glGetProgramiv(programID , GL_LINK_STATUS  , &programState);
	if ( programState != GL_TRUE)
	{
		display_log(programID);
		deleteProgram(programID, fragmentID, vertexID);
		return -1;
	}

	glDetachShader(programID, vertexID);
	glDetachShader(programID, fragmentID);

	glDeleteShader(vertexID);
	glDeleteShader(fragmentID);

	glUseProgram(programID);

	motor->programID = programID;
	return programID;
}

/***********************************************************************
 * GLMotor_Object_t
 **********************************************************************/
struct GLMotor_Object_s
{
	GLMotor_t *motor;
	GLuint ID[2];
	GLuint maxpoints;
	GLuint maxfaces;
	GLuint npoints;
	GLuint nfaces;

	GLfloat color[4];
	GLintptr offsetcolors;
	GLintptr offsetuvs;
	GLintptr offsetnormals;
	GLuint ncolors;
	GLuint nuvs;
	GLuint nnormals;
};

GLMOTOR_EXPORT GLMotor_Object_t *object_create(GLMotor_t *motor, GLchar *name, GLuint maxpoints, GLuint maxfaces)
{
	GLuint objID[2] = {0};
	glGenBuffers(2, objID);
	glBindBuffer(GL_ARRAY_BUFFER, objID[0]);
	GLsizeiptr size = maxpoints * sizeof(GLfloat) * 3;
	GLintptr offsetcolors = 0;
	GLintptr offsetuvs = 0;
	GLintptr offsetnormals = 0;
	offsetcolors = size;
	size += maxpoints * sizeof(GLfloat) * COLOR_COMPONENTS;
#if 0
	offsetuvs = size;
	size += maxpoints * sizeof(GLfloat) * 2;
	offsetnormals = size;
	size += maxpoints * sizeof(GLfloat) * 3;
#endif

	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);

	if (maxfaces)
	{
		GLsizeiptr size = maxfaces * sizeof(GLuint) * 3;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objID[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
		glBindAttribLocation(motor->programID, objID[1], name );
	}
	else
		glBindAttribLocation(motor->programID, objID[0], name );

	GLMotor_Object_t *obj;
	obj = calloc(1, sizeof(*obj));
	obj->motor = motor;
	obj->ID[0] = objID[0];
	obj->ID[1] = objID[1];
	obj->maxpoints = maxpoints;
	obj->maxfaces = maxfaces;

	obj->offsetcolors = offsetcolors;
	obj->offsetuvs = offsetuvs;
	obj->offsetnormals = offsetnormals;

	return obj;
}

GLMOTOR_EXPORT GLuint object_appendpoint(GLMotor_Object_t *obj, GLuint npoints, GLfloat points[])
{
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[0]);
	GLuint offset = 0;
	offset += obj->npoints * sizeof(GLfloat) * 3;
	glBufferSubData(GL_ARRAY_BUFFER, offset, npoints * sizeof(GLfloat) * 3, points);
	obj->npoints += npoints;
	return 0;
}

GLMOTOR_EXPORT GLuint object_appendface(GLMotor_Object_t *obj, GLuint nfaces, GLuint face[])
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ID[1]);
	GLuint offset = 0;
	offset += obj->nfaces * sizeof(GLuint) * 3;
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, nfaces * sizeof(GLuint) * 3, face);
	obj->nfaces += nfaces;
	return 0;
}

GLMOTOR_EXPORT GLuint object_appendcolor(GLMotor_Object_t *obj, GLuint ncolors, GLfloat colors[])
{
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[0]);
	GLuint offset = obj->offsetcolors;
	offset += obj->ncolors * sizeof(GLfloat) * COLOR_COMPONENTS;
	glBufferSubData(GL_ARRAY_BUFFER, offset, ncolors * sizeof(GLfloat) * COLOR_COMPONENTS, colors);
	obj->ncolors += ncolors;
	if (obj->ncolors == 1)
		memcpy(obj->color, colors, ncolors * sizeof(GLfloat) * 4);
	return 0;
}

GLMOTOR_EXPORT GLuint object_appenduv(GLMotor_Object_t *obj, GLuint nuvs, GLfloat uvs[])
{
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[0]);
	GLuint offset = obj->offsetuvs;
	offset += obj->nuvs * sizeof(GLfloat) * 2;
	glBufferSubData(GL_ARRAY_BUFFER, offset, nuvs * sizeof(GLfloat) * 2, uvs);
	obj->nuvs += nuvs;
	return 0;
}

GLMOTOR_EXPORT GLuint object_appendnormal(GLMotor_Object_t *obj, GLuint nnormals, GLfloat normals[])
{
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[0]);
	GLuint offset = obj->offsetnormals;
	offset += obj->nnormals * sizeof(GLfloat) * 3;
	glBufferSubData(GL_ARRAY_BUFFER, offset, nnormals * sizeof(GLfloat) * 2, normals);
	obj->nnormals += nnormals;
	return 0;
}

GLMOTOR_EXPORT GLuint object_draw(GLMotor_Object_t *obj)
{
	GLMotor_t *motor = obj->motor;
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	if (obj->ncolors == 1)
	{
		glVertexAttrib4fv ( 1, obj->color );
	}
	else if (obj->ncolors)
	{
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, COLOR_COMPONENTS, GL_FLOAT, GL_FALSE, obj->offsetcolors, (void*)0);
	}
	if (obj->nuvs)
	{
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, obj->offsetuvs, (void*)0);
	}
	if (obj->nnormals)
	{
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, obj->offsetnormals, (void*)0);
	}
	if (obj->nfaces)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ID[1]);
	}

	if (obj->nfaces && obj->nfaces < UINT_MAX)
		glDrawElements(GL_TRIANGLE_STRIP, obj->nfaces * 3, GL_UNSIGNED_INT, 0);
	else
		glDrawArrays(GL_TRIANGLE_STRIP, 0, obj->npoints);
	glDisableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0 );
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0 );
   	return 0;
}

GLMOTOR_EXPORT void destroy_object(GLMotor_Object_t *obj)
{
	glDeleteBuffers(2, obj->ID);
	free(obj);
}

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
#if 1
	glClearDepthf(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

#if HAVE_GLEW
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(atan(tan(50.0 * 3.14159 / 360.0) / 1.0) * 360.0 / 3.141593, (GLfloat)motor->width / (GLfloat)motor->height, 0.1, 100.0);
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
#ifdef HAVE_GLEW
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
	gluLookAt(camera[0], camera[1], camera[2],
			applyTarget[0], applyTarget[1], applyTarget[2],
			0, 1, 0);
}

GLMOTOR_EXPORT void scene_appendobject(GLMotor_Scene_t *scene, GLMotor_Object_t *obj)
{
	GLMotor_list_t *entry = calloc(1, sizeof(entry));
	entry->entity = obj;
	entry->next = scene->objects;
	scene->objects = entry;
}

GLMOTOR_EXPORT void scene_draw(GLMotor_Scene_t *scene)
{
	GLMotor_t *motor = scene->motor;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#ifdef HAVE_GLEW
	glLoadIdentity();
#endif

	for (GLMotor_list_t *it = scene->objects; it != NULL; it = it->next)
		object_draw((GLMotor_Object_t *)it->entity);
}

GLMOTOR_EXPORT void scene_destroy(GLMotor_Scene_t *scene)
{
	for (GLMotor_list_t *it = scene->objects, *next = it->next; it != NULL; it = next, next = it->next)
	{
		destroy_object(it->entity);
		free(it);
	}
	free(scene);
}

/***********************************************************************
 * GLMotor_Texture_t
 **********************************************************************/
struct GLMotor_Texture_s
{
	GLMotor_t *motor;
	GLuint ID;
	GLuint npoints;
	GLuint nfaces;
};

GLMOTOR_EXPORT GLMotor_Texture_t *texture_create(GLMotor_t *motor, GLuint width, GLuint height, GLchar *map)
{
	GLMotor_Texture_t *texture = NULL;
	return texture;
}
