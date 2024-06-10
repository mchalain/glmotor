#include <stddef.h>
#include <stdlib.h>
#include <math.h>

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
	GLuint ID;
	GLuint npoints;
	GLfloat *points;
};

GLMOTOR_EXPORT GLMotor_Object_t *object_create(GLMotor_t *motor, GLchar *name, GLuint npoints, GLfloat *points)
{
	GLuint objID = 0;
#ifdef HAVE_GLESV2
	//objID = glGetAttribLocation(motor->programID, name );
	glBindAttribLocation(motor->programID, objID, name );
//	glVertexAttribPointer(objID, 3, GL_FLOAT, GL_FALSE, 0, points);
#else
	glGenBuffers(1, &objID);
	glBindBuffer(GL_ARRAY_BUFFER, objID);
	glBufferData(GL_ARRAY_BUFFER, npoints * sizeof(GLfloat) * 3 /* size of buffer */, points, GL_STATIC_DRAW);
#endif

	GLMotor_Object_t *obj;
	obj = calloc(1, sizeof(*obj));
	obj->motor = motor;
	obj->ID = objID;
	obj->npoints = npoints;
	obj->points = points;

	return obj;
}

GLMOTOR_EXPORT GLuint object_draw(GLMotor_Object_t *obj)
{
	GLMotor_t *motor = obj->motor;
#ifdef HAVE_GLESV2
	//glVertexAttribPointer(obj->ID, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat)/* stride between elements */, obj->points);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, obj->points);
	glEnableVertexAttribArray(0);

	glDrawArrays(GL_TRIANGLES, 0 /* first points inside points */, obj->npoints);
	glDisableVertexAttribArray(obj->ID);
#else
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, obj->npoints);
	glDisableVertexAttribArray(obj->ID);
#endif
	return 0;
}

GLMOTOR_EXPORT void destroy_object(GLMotor_Object_t *obj)
{
	glDeleteBuffers(1, &obj->ID);
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
