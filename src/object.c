#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
#else
# include <GL/glew.h>
# include <GL/gl.h>
#endif
#ifdef HAVE_GLU
#include <GL/glu.h>
#endif

#include "glmotor.h"
#include "mat.h"
#include "log.h"

/// the usage of half_float has no effect on the performances
//#define VERTEX_HALFFLOAT
#ifdef VERTEX_HALFFLOAT
#include "halffloat.h"
#endif

/***********************************************************************
 * GLMotor_Object_t
 **********************************************************************/
typedef struct GLMotor_Kinematic_s GLMotor_Kinematic_t;
struct GLMotor_Kinematic_s
{
	GLMotor_Translate_t tr;
	GLMotor_Rotate_t rot;
	int repeats;
	int rest;
	GLMotor_Kinematic_t *next;
};

struct GLMotor_Object_s
{
	GLMotor_t *motor;
	char *name;
	GLuint vertexArrayID;
	GLuint *ID;
	GLuint maxpoints;
	GLuint maxfaces;
	GLuint npoints;
	GLuint nfaces;

	GLfloat move[16];
	GLfloat defaultcolor[4];
	GLuint position;
	GLuint color;
	GLuint uv;
	GLuint normal;
	GLuint ncolors;
	GLuint nuvs;
	GLuint nnormals;

	GLuint programID;
	GLMotor_Kinematic_t *kinematic;
	GLMotor_Texture_t *texture;
	pthread_mutex_t kinmutex;
};

static GLuint _object_setup(GLMotor_Object_t *obj, GLuint programID)
{
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// assign vertices (points or positions)
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[0]);
	GLsizeiptr size = 0;
	obj->position = glGetAttribLocation(programID, "vPosition");
	if (obj->position < 0)
	{
		err("vertex shader doesn't contain vPosition entry");
	}
	else
	{
#ifdef VERTEX_HALFFLOAT
		size = obj->maxpoints * sizeof(uint16_t) * 3;
		glVertexAttribPointer(obj->position, 3, GL_HALF_FLOAT_OES, GL_FALSE, 0, (void*)0);
#else
		size = obj->maxpoints * sizeof(GLfloat) * 3;
		glVertexAttribPointer(obj->position, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
#endif
		glEnableVertexAttribArray(obj->position);
		glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	}

	if (obj->maxfaces)
	{
		// assign faces
		GLsizeiptr size = obj->maxfaces * sizeof(GLuint) * 3;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ID[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	}
	// assign colors
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[2]);
	size = obj->maxpoints * sizeof(GLfloat) * 3;
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	obj->color = glGetAttribLocation(obj->programID, "vColor");
	if (obj->color >= 0)
	{
		glVertexAttribPointer(obj->color, COLOR_COMPONENTS, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(obj->color);
	}

	// assign uvs
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[3]);
	obj->uv = glGetAttribLocation(obj->programID, "vUV");
	if (obj->uv >= 0)
	{
#ifdef VERTEX_HALFFLOAT
		size = obj->maxpoints * sizeof(uint16_t) * 2;
		glVertexAttribPointer(obj->uv, 2, GL_HALF_FLOAT_OES, GL_FALSE, 0, (void*)0);
#else
		size = obj->maxpoints * sizeof(GLfloat) * 2;
		glVertexAttribPointer(obj->uv, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
#endif
		glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
		glEnableVertexAttribArray(obj->uv);
	}

	// assign normal
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[4]);
	size = obj->maxpoints * sizeof(GLfloat) * 3;
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	obj->normal = glGetAttribLocation(obj->programID, "vNormal");
	if (obj->normal >= 0)
	{
		glVertexAttribPointer(obj->normal, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(obj->normal);
	}

	glBindVertexArray(0);
	return vao;
}

GLMOTOR_EXPORT GLMotor_Object_t *object_create(GLMotor_t *motor, const char *name, GLuint maxpoints, GLuint maxfaces)
{
	GLuint *objID = calloc(5, sizeof(*objID));
	glGenBuffers(5, objID);

	GLMotor_Object_t *obj;
	obj = calloc(1, sizeof(*obj));
	obj->name = strdup(name);
	obj->motor = motor;
	obj->ID = objID;
	obj->maxpoints = maxpoints;
	obj->maxfaces = maxfaces;
	pthread_mutex_init(&obj->kinmutex, NULL);

	mat4_diag(obj->move);

	return obj;
}

GLMOTOR_EXPORT GLint object_setprogram(GLMotor_Object_t *obj, GLuint programID)
{
	obj->programID = programID;
	obj->vertexArrayID = _object_setup(obj, programID);
	return 1;
}

GLMOTOR_EXPORT GLuint object_appendpoint(GLMotor_Object_t *obj, GLuint npoints, GLfloat points[])
{
# define point_vectorsize 3
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[0]);
	GLuint offset = 0;
#ifdef VERTEX_HALFFLOAT
	for (int i = 0; i < npoints; i++)
	{
		uint16_t halfpoints[point_vectorsize];
		for (int j = 0; j < point_vectorsize; j++)
		{
			halfpoints[j] = float16(points[j + i * point_vectorsize]);
		}
		offset += obj->npoints * sizeof(uint16_t) * point_vectorsize;
		glBufferSubData(GL_ARRAY_BUFFER, offset, 1 * sizeof(uint16_t) * point_vectorsize, halfpoints);
		obj->npoints++;
	}
#else
	offset += obj->npoints * sizeof(GLfloat) * point_vectorsize;
	glBufferSubData(GL_ARRAY_BUFFER, offset, npoints * sizeof(GLfloat) * point_vectorsize, points);
	obj->npoints += npoints;
#endif
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
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[2]);
	GLuint offset = 0;
	offset += obj->ncolors * sizeof(GLfloat) * COLOR_COMPONENTS;
	glBufferSubData(GL_ARRAY_BUFFER, offset, ncolors * sizeof(GLfloat) * COLOR_COMPONENTS, colors);
	obj->ncolors += ncolors;
	if (obj->ncolors == 1)
		memcpy(obj->defaultcolor, colors, ncolors * sizeof(GLfloat) * 4);
	return 0;
}

GLMOTOR_EXPORT void object_move(GLMotor_Object_t *obj, GLMotor_Translate_t *tr, GLMotor_Rotate_t *rot)
{
	// The matrix is inversed
	if (rot && rot->mq[15] == 0)
	{
		GLMotor_RotAxis_t *ra = &rot->ra;
		GLfloat mq[16];
		ra2mq(ra, mq);
		mat4_multiply4(obj->move, mq, obj->move);
	}
	else if (rot)
	{
		mat4_multiply4(obj->move, rot->mq, obj->move);
	}
	if (tr && tr->coord.L != 0)
	{
		for (int i = 0; i < 3; i++)
			obj->move[12 + i] += tr->mat[i] * tr->coord.L;
	}
}

GLMOTOR_EXPORT const GLfloat* object_positionmatrix(GLMotor_Object_t *obj)
{
	return obj->move;
}

GLMOTOR_EXPORT GLint object_kinematic(GLMotor_Object_t *obj, GLMotor_Translate_t **tr, GLMotor_Rotate_t **rot)
{
	GLMotor_Kinematic_t *kin = obj->kinematic;
	if (kin == NULL)
		return -1;
	if (kin->rest == 0)
	{
		/**
		 * the kinetic is complete, take the next one
		 */
		if (kin->repeats < 0)
		{
			pthread_mutex_lock(&obj->kinmutex);
			/**
			 * a negativ repeats will place the kinetic to the end
			 * of the list. This generates a infinity loop
			 */
			GLMotor_Kinematic_t *last = obj->kinematic;
			while (last->next != NULL) last = last->next;
			if (last != obj->kinematic)
			{
				last->next = kin;
				obj->kinematic = obj->kinematic->next;
			}
			kin->next = NULL;
			kin->rest = -kin->repeats;
			kin = obj->kinematic;
			pthread_mutex_unlock(&obj->kinmutex);
		}
		else
		{
			pthread_mutex_lock(&obj->kinmutex);
			obj->kinematic = obj->kinematic->next;
			pthread_mutex_unlock(&obj->kinmutex);
			free(kin);
			return -1;
		}
	}
	if (kin->rest > 0)
	{
		kin->rest--;
		*tr = &kin->tr;
		*rot = &kin->rot;
		return 1;
	}
	return 0;
}

/**
 * @brief add movement to play during drawing
 *
 * @param obj the object to affect
 * @param tr  the translacion vertex
 * @param rot the quaternion matrix or the couple vertex + angle
 * @param repeats the number of time to repeat the movement, minus value will push the movement to the end of kinematic list to repeat again
 */
GLMOTOR_EXPORT void object_appendkinematic(GLMotor_Object_t *obj, GLMotor_Translate_t *tr, GLMotor_Rotate_t *rot, int repeats)
{
	GLMotor_Rotate_t tmp = {0};
	pthread_mutex_lock(&obj->kinmutex);
	/**
	 * two different cases:
	 *  - repeats is 0 : it's a punctual event to append to the last kinetic
	 *  - otherwise : it's a kinetic to run between two rendering
	 */
	GLMotor_Kinematic_t *kin = obj->kinematic;
	if (repeats || kin == NULL)
		kin = calloc(1, sizeof(*kin));
	if (kin->rest == 0)
		memset(kin, 0, sizeof(*kin));
	if (tr)
	{
		for (int i = 0; i < sizeof(*tr)/sizeof(*tr->mat); i++)
			kin->tr.mat[i] += tr->mat[i];
	}
	if (rot && rot->mq[15] == 0)
	{
		ra2mq(&rot->ra, tmp.mq);
		rot = &tmp;
	}
	if (rot)
	{
		mat4_add4(kin->rot.mq, 1, rot->mq, kin->rot.mq);
	}
	if (kin != obj->kinematic)
	{
		kin->next = obj->kinematic;
	}
	else
		repeats = 1;
	kin->rest = kin->repeats = repeats;
	kin->rest *= (kin->repeats < 0)? -1: 1;
	obj->kinematic = kin;
	pthread_mutex_unlock(&obj->kinmutex);
}

GLMOTOR_EXPORT GLuint object_addtexture(GLMotor_Object_t *obj, GLMotor_Texture_t *tex)
{
	if (obj->texture)
		texture_destroy(obj->texture);
	obj->texture = tex;
	texture_setprogram(tex, obj->programID);
}

GLMOTOR_EXPORT GLuint object_appenduv(GLMotor_Object_t *obj, GLuint nuvs, GLfloat uvs[])
{
# define uv_vectorsize 2
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[3]);
	GLuint offset = 0;
#ifdef VERTEX_HALFFLOAT
	for (int i = 0; i < nuvs; i++)
	{
		uint16_t halfuvs[uv_vectorsize];
		for (int j = 0; j < uv_vectorsize; j++)
		{
			halfuvs[j] = float16(uvs[j + i * uv_vectorsize]);
		}
		offset += obj->nuvs * sizeof(uint16_t) * uv_vectorsize;
		glBufferSubData(GL_ARRAY_BUFFER, offset, 1 * sizeof(uint16_t) * uv_vectorsize, halfuvs);
		obj->nuvs++;
	}
#else
	offset += obj->nuvs * sizeof(GLfloat) * uv_vectorsize;
	glBufferSubData(GL_ARRAY_BUFFER, offset, nuvs * sizeof(GLfloat) * uv_vectorsize, uvs);
	obj->nuvs += nuvs;
#endif
	return 0;
}

GLMOTOR_EXPORT GLuint object_appendnormal(GLMotor_Object_t *obj, GLuint nnormals, GLfloat normals[])
{
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[4]);
	GLuint offset = 0;
	offset += obj->nnormals * sizeof(GLfloat) * 3;
	glBufferSubData(GL_ARRAY_BUFFER, offset, nnormals * sizeof(GLfloat) * 2, normals);
	obj->nnormals += nnormals;
	return 0;
}

GLMOTOR_EXPORT GLint object_draw(GLMotor_Object_t *obj)
{
	GLint ret = 0;
	GLMotor_t *motor = obj->motor;
	glUseProgram(obj->programID);

	glBindVertexArray(obj->vertexArrayID);
	if (obj->texture)
		ret = texture_draw(obj->texture);

	GLuint moveID = glGetUniformLocation(obj->programID, "vMove");
	glUniformMatrix4fv(moveID, 1, GL_FALSE, &obj->move[0]);

	if (obj->nfaces && obj->nfaces < UINT_MAX)
		glDrawElements(GL_TRIANGLE_STRIP, obj->nfaces * 3, GL_UNSIGNED_INT, 0);
	else
		glDrawArrays(GL_TRIANGLE_STRIP, 0, obj->npoints);
	glBindVertexArray(0);
	return ret;
}

GLMOTOR_EXPORT const char * object_name(GLMotor_Object_t *obj)
{
	return obj->name;
}

GLMOTOR_EXPORT void object_destroy(GLMotor_Object_t *obj)
{
	if (obj->texture)
		texture_destroy(obj->texture);
	glDeleteBuffers(2, obj->ID);
	free(obj->ID);
	free(obj->name);
	free(obj);
}
