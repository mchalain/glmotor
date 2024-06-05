#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#ifdef HAVE_GLEW
#include <GL/glew.h>
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

static void deleteShader(GLuint programID, GLuint fragmentID, GLuint vertexID)
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

static char checkShaderCompilation(GLuint shaderID)
{
	GLint compilationStatus = 0;

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compilationStatus);
	if ( compilationStatus != GL_TRUE )
	{
		err("glmotor: shader compilation error");
		display_log(shaderID);
		return 0;
	}

	return 1;
}

GLMOTOR_EXPORT GLuint build_program(GLMotor_t *motor, GLchar *vertexSource, GLuint vertexSize, GLchar *fragmentSource, GLuint fragmentSize)
{
	GLint programState = 0;
	GLuint vertexID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentID = glCreateShader(GL_FRAGMENT_SHADER);

	warn("glmotor: vertex shader");
	glShaderSource(vertexID, 1, (const GLchar**)(&vertexSource), &vertexSize);
	glCompileShader(vertexID);
	if ( !checkShaderCompilation(vertexID))
	{
		deleteShader(0, vertexID, 0);
		return 0;
	}

	warn("glmotor: fragment shader");
	glShaderSource(fragmentID, 1, (const GLchar**)(&fragmentSource), &fragmentSize);
	glCompileShader(fragmentID);
	if (!checkShaderCompilation(fragmentID))
	{
		deleteShader(0, vertexID, fragmentID);
		return 0;
	}

	GLuint programID = glCreateProgram();

	glAttachShader(programID, vertexID);
	glAttachShader(programID, fragmentID);

	glLinkProgram(programID);

	glGetProgramiv(programID , GL_LINK_STATUS  , &programState);
	if ( programState != GL_TRUE)
	{
		display_log(programID);
		deleteShader(programID, fragmentID, vertexID);
		return -1;
	}

    glDetachShader(programID, vertexID);
    glDetachShader(programID, fragmentID);

	glDeleteShader(vertexID);
	glDeleteShader(fragmentID);

	glUseProgram(programID);

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
	GLuint nfaces;
};

GLMOTOR_EXPORT GLMotor_Object_t *create_object(GLMotor_t *motor, GLuint npoints, GLfloat *points)
{
	GLuint objID;
	glGenBuffers(1, &objID);
	glBindBuffer(GL_ARRAY_BUFFER, objID);
	glBufferData(GL_ARRAY_BUFFER, npoints * sizeof(GLfloat) * 3, points, GL_STATIC_DRAW);

	GLMotor_Object_t *obj;
	obj = calloc(1, sizeof(*obj));
	obj->motor = motor;
	obj->ID = objID;
	obj->npoints = npoints;

	return obj;
}

GLMOTOR_EXPORT GLuint add_objectpoint(GLMotor_Object_t *obj, GLfloat *point)
{
	obj->npoints++;
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID);
	return -1;
}

GLMOTOR_EXPORT GLuint draw_object(GLMotor_Object_t *obj)
{
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID);
	glVertexAttribPointer(
	   0,
	   3,
	   GL_FLOAT,
	   GL_FALSE,
	   0,
	   (void*)0 
	);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, obj->npoints);
	glDisableVertexAttribArray(0);
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

GLMOTOR_EXPORT GLMotor_Scene_t *create_scene(GLMotor_t *motor)
{
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glViewport(0, 0, motor->width, motor->height);
	glClearDepthf(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
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

GLMOTOR_EXPORT void move_camera(GLMotor_Scene_t *scene, const GLfloat *camera, const GLfloat *target)
{
	const GLfloat defaultTarget[] = {0.0, 0.0, 0.0}; //target the center of the world
	const GLfloat *applyTarget = defaultTarget;
	if (target != NULL)
		applyTarget = target;
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(camera[0], camera[1], camera[2],
			applyTarget[0], applyTarget[1], applyTarget[2],
			0, 1, 0);
}

GLMOTOR_EXPORT void append_object(GLMotor_Scene_t *scene, GLMotor_Object_t *obj)
{
	GLMotor_list_t *entry = calloc(1, sizeof(entry));
	entry->entity = obj;
	entry->next = scene->objects;
	scene->objects = entry;
}

GLMOTOR_EXPORT void draw_scene(GLMotor_Scene_t *scene)
{
	GLMotor_t *motor = scene->motor;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	for (GLMotor_list_t *it = scene->objects; it != NULL; it = it->next)
		draw_object((GLMotor_Object_t *)it->entity);
}

GLMOTOR_EXPORT void destroy_scene(GLMotor_Scene_t *scene)
{
	for (GLMotor_list_t *it = scene->objects, *next = it->next; it != NULL; it = next, next = it->next)
	{
		destroy_object(it->entity);
		free(it);
	}
	free(scene);
}

