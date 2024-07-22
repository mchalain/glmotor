#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>

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
#include "log.h"

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

	GLMotor_Kinematic_t *kinematic;
	GLMotor_Texture_t *texture;
};

GLMOTOR_EXPORT GLMotor_Object_t *object_create(GLMotor_t *motor, const char *name, GLuint maxpoints, GLuint maxfaces)
{
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	GLuint *objID = calloc(4, sizeof(*objID));
	glGenBuffers(5, objID);

	glBindVertexArray(VAO);
	// assign vertices (points or positions)
	glBindBuffer(GL_ARRAY_BUFFER, objID[0]);
	GLsizeiptr size = maxpoints * sizeof(GLfloat) * 3;
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	GLuint position = glGetAttribLocation(motor->programID, "vPosition");
	if (position < 0)
	{
		err("vertex shader doesn't contain vPosition entry");
	}
	else
		glEnableVertexAttribArray(position);

	if (maxfaces)
	{
		// assign faces
		GLsizeiptr size = maxfaces * sizeof(GLuint) * 3;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objID[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	}
	// assign colors
	glBindBuffer(GL_ARRAY_BUFFER, objID[2]);
	size = maxpoints * sizeof(GLfloat) * 3;
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	GLint color = glGetAttribLocation(motor->programID, "vColor");
	if (color >= 0)
		glEnableVertexAttribArray(color);
	
	// assign uvs
	glBindBuffer(GL_ARRAY_BUFFER, objID[3]);
	size = maxpoints * sizeof(GLfloat) * 2;
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	GLint uv = glGetAttribLocation(motor->programID, "vUV");
	if (uv >= 0)
		glEnableVertexAttribArray(uv);

	// assign normal
	glBindBuffer(GL_ARRAY_BUFFER, objID[4]);
	size = maxpoints * sizeof(GLfloat) * 3;
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	GLint normal = glGetAttribLocation(motor->programID, "vNormal");
	if (normal >= 0)
		glEnableVertexAttribArray(normal);

	GLMotor_Object_t *obj;
	obj = calloc(1, sizeof(*obj));
	obj->name = strdup(name);
	obj->vertexArrayID = VAO;
	obj->motor = motor;
	obj->ID = objID;
	obj->maxpoints = maxpoints;
	obj->maxfaces = maxfaces;

	obj->move[ 0] =
	 obj->move[ 5] =
	 obj->move[10] =
	 obj->move[15] = 1;
	obj->position = position;
	obj->color = color;
	obj->uv = uv;
	obj->normal = normal;

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
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[2]);
	GLuint offset = 0;
	offset += obj->ncolors * sizeof(GLfloat) * COLOR_COMPONENTS;
	glBufferSubData(GL_ARRAY_BUFFER, offset, ncolors * sizeof(GLfloat) * COLOR_COMPONENTS, colors);
	obj->ncolors += ncolors;
	if (obj->ncolors == 1)
		memcpy(obj->defaultcolor, colors, ncolors * sizeof(GLfloat) * 4);
	return 0;
}

static void mat4_multiply4(GLfloat A[], GLfloat B[], GLfloat AB[])
{
	GLfloat TMP[16];
	// line 1
	TMP[ 0] = A[ 0] * B[ 0] + A[ 1] * B[ 4] + A[ 2] * B[ 8] + A[ 3] * B[12];
	TMP[ 1] = A[ 0] * B[ 1] + A[ 1] * B[ 5] + A[ 2] * B[ 9] + A[ 3] * B[13];
	TMP[ 2] = A[ 0] * B[ 2] + A[ 1] * B[ 6] + A[ 2] * B[10] + A[ 3] * B[14];
	TMP[ 3] = A[ 0] * B[ 3] + A[ 1] * B[ 7] + A[ 2] * B[11] + A[ 3] * B[15];
	// line 2
	TMP[ 4] = A[ 4] * B[ 0] + A[ 5] * B[ 4] + A[ 6] * B[ 8] + A[ 7] * B[12];
	TMP[ 5] = A[ 4] * B[ 1] + A[ 5] * B[ 5] + A[ 6] * B[ 9] + A[ 7] * B[13];
	TMP[ 6] = A[ 4] * B[ 2] + A[ 5] * B[ 6] + A[ 6] * B[10] + A[ 7] * B[14];
	TMP[ 7] = A[ 4] * B[ 3] + A[ 5] * B[ 7] + A[ 6] * B[11] + A[ 7] * B[15];
	// line 3
	TMP[ 8] = A[ 8] * B[ 0] + A[ 9] * B[ 4] + A[10] * B[ 8] + A[11] * B[12];
	TMP[ 9] = A[ 8] * B[ 1] + A[ 9] * B[ 5] + A[10] * B[ 9] + A[11] * B[13];
	TMP[10] = A[ 8] * B[ 2] + A[ 9] * B[ 6] + A[10] * B[10] + A[11] * B[14];
	TMP[11] = A[ 8] * B[ 3] + A[ 9] * B[ 7] + A[10] * B[11] + A[11] * B[15];
	// line 4
	TMP[12] = A[12] * B[ 0] + A[13] * B[ 4] + A[14] * B[ 8] + A[15] * B[12];
	TMP[13] = A[12] * B[ 1] + A[13] * B[ 5] + A[14] * B[ 9] + A[15] * B[13];
	TMP[14] = A[12] * B[ 2] + A[13] * B[ 6] + A[14] * B[10] + A[15] * B[14];
	TMP[15] = A[12] * B[ 3] + A[13] * B[ 7] + A[14] * B[11] + A[15] * B[15];

	memcpy(AB, TMP, sizeof(TMP));
}

#if 0
static void mat4_log(GLfloat mat[])
{
	for (int i = 0; i < 4; i++)
	{
		printf("% 0.3f % 0.3f % 0.3f % 0.3f\n",
			mat[0 + i * 4],
			mat[1 + i * 4],
			mat[2 + i * 4],
			mat[3 + i * 4]
		);
	}
}
#else
#define mat4_log(...)
#endif

float squaref(float value)
{
	return value * value;
}

float normalizef(float u, float v, float w)
{
	return sqrtf(squaref(u) + squaref(v) + squaref(w));
}

void ra2mq(GLMotor_RotAxis_t *ra, GLfloat mq[])
{
	GLfloat rcos = cos(ra->A);
	GLfloat ircos = 1 - rcos;
	GLfloat rsin = sin(ra->A);
	GLfloat xx = squaref(ra->X);
	GLfloat yy = squaref(ra->Y);
	GLfloat zz = squaref(ra->Z);
	GLfloat xy = ra->X * ra->Y;
	GLfloat xz = ra->X * ra->Z;
	GLfloat yz = ra->Y * ra->Z;

	mq[ 0] =          rcos + xx * ircos;
	mq[ 4] =  ra->Z * rsin + xy * ircos;
	mq[ 8] = -ra->Y * rsin + yz * ircos;

	mq[ 1] = -ra->Z * rsin + xy * ircos;
	mq[ 5] =          rcos + yy * ircos;
	mq[ 9] =  ra->X * rsin + yz * ircos;

	mq[ 2] =  ra->Y * rsin + xz * ircos;
	mq[ 6] = -ra->X * rsin + yz * ircos;
	mq[10] =          rcos + zz * ircos;

	mq[3]  = mq[7] = mq[11] = mq[12] = mq[13] = mq[14] = 0;
	mq[15] = 1;
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
	if (tr && tr->L != 0)
	{
		if (tr->X + tr->Y + tr->Z != 1)
			tr->L /= normalizef(tr->X, tr->Y, tr->Z);
		obj->move[12] += tr->X * tr->L;
		obj->move[13] += tr->Y * tr->L;
		obj->move[14] += tr->Z * tr->L;
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
	kin->rest--;
	if (kin->rest == 0)
	{
		if (kin->repeats > 0)
		{
			obj->kinematic = obj->kinematic->next;
			free(kin);
			return -1;
		}
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
	}
	if (kin->rest > 0)
	{
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
	GLMotor_Kinematic_t *kin = calloc(1, sizeof(*kin));
	if (tr)
		memcpy(&kin->tr, tr, sizeof(kin->tr));
	if (rot && rot->mq[15] == 0)
	{
		ra2mq(&rot->ra, kin->rot.mq);
	}
	else if (rot)
	{
		memcpy(kin->rot.mq, rot->mq, sizeof(kin->rot.mq));
	}
	kin->next = obj->kinematic;
	kin->rest = kin->repeats = repeats;
	kin->rest *= (kin->repeats < 0)? -1: 1;
	obj->kinematic = kin;
}

GLMOTOR_EXPORT GLuint object_addtexture(GLMotor_Object_t *obj, GLMotor_Texture_t *tex)
{
	obj->texture = tex;
}

GLMOTOR_EXPORT GLuint object_appenduv(GLMotor_Object_t *obj, GLuint nuvs, GLfloat uvs[])
{
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[3]);
	GLuint offset = 0;
	offset += obj->nuvs * sizeof(GLfloat) * 2;
	glBufferSubData(GL_ARRAY_BUFFER, offset, nuvs * sizeof(GLfloat) * 2, uvs);
	obj->nuvs += nuvs;
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

	glBindVertexArray(obj->vertexArrayID);
	glBindBuffer(GL_ARRAY_BUFFER, obj->ID[0]);
	glEnableVertexAttribArray(obj->position);
	glVertexAttribPointer(obj->position, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	if (obj->ncolors == 1)
	{
		glVertexAttrib4fv(obj->color, obj->defaultcolor);
	}
	else if (obj->ncolors)
	{
		glBindBuffer(GL_ARRAY_BUFFER, obj->ID[2]);
		glEnableVertexAttribArray(obj->color);
		glVertexAttribPointer(obj->color, COLOR_COMPONENTS, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}
	if (obj->nuvs)
	{
		glBindBuffer(GL_ARRAY_BUFFER, obj->ID[3]);
		glEnableVertexAttribArray(obj->uv);
		glVertexAttribPointer(obj->uv, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}
	if (obj->nnormals)
	{
		glBindBuffer(GL_ARRAY_BUFFER, obj->ID[4]);
		glEnableVertexAttribArray(obj->normal);
		glVertexAttribPointer(obj->normal, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}
	if (obj->nfaces)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ID[1]);
	}
	GLuint moveID = glGetUniformLocation(motor->programID, "vMove");
	glUniformMatrix4fv(moveID, 1, GL_FALSE, &obj->move[0]);

	if (obj->texture)
		ret = texture_draw(obj->texture);

	if (obj->nfaces && obj->nfaces < UINT_MAX)
		glDrawElements(GL_TRIANGLE_STRIP, obj->nfaces * 3, GL_UNSIGNED_INT, 0);
	else
		glDrawArrays(GL_TRIANGLE_STRIP, 0, obj->npoints);
	glDisableVertexAttribArray(obj->position);
	if (obj->ncolors)
		glDisableVertexAttribArray(obj->color);
	if (obj->nuvs)
		glDisableVertexAttribArray(obj->uv);
	if (obj->nnormals)
		glDisableVertexAttribArray(obj->normal);

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
