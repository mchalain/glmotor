#include <stdio.h>
#include <string.h>
#include <math.h>

#include "glmotor.h"
#include "log.h"

void mat4_scale(const GLfloat scale, GLfloat A[16])
{
	memset(A, 0, 16 * sizeof(*A));
	A[0] = A[5] = A[10] = scale;
	A[15] = 1.0;
}

void mat4_diag(GLfloat A[16])
{
	mat4_scale(1.0, A);
}

void mat4_multiply4(const GLfloat A[], const GLfloat B[], GLfloat AB[])
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

static void _add(const GLfloat A[], const GLfloat B[], GLfloat AB[], unsigned int size, int sign)
{
	if (sign < 0)
		sign = -1;
	else
		sign = 1;
	for (int i = 0; i < size; i++)
	{
		AB[i] = A[i] + sign * B[i];
	}
}

void mat_add(const GLfloat A[], int sign, const GLfloat B[], GLfloat AB[], unsigned int size)
{
	_add(A, B, AB, size * size, sign);
}

void mat4_add4(const GLfloat A[], int sign, const GLfloat B[], GLfloat AB[])
{
	_add(A, B, AB, 16, sign);
}

void vec_add(const GLfloat A[], int sign, const GLfloat B[], GLfloat AB[], unsigned int size)
{
	_add(A, B, AB, size, sign);
}

void vec3_add(const GLfloat A[], int sign, const GLfloat B[], GLfloat AB[])
{
	_add(A, B, AB, 3, sign);
}

void mat4_frustum(const GLfloat frame[], const GLfloat near, const GLfloat far, GLfloat O[])
{
	if ((frame[0] - frame[1] == 0.0f) ||
		(frame[2] - frame[3] == 0.0f) ||
		(near - far == 0.0f))
	{
		mat4_diag(O);
		return;
	}
	memset(O, 0, 16 *sizeof(GLfloat));
	GLfloat large = frame[1] - frame[0];
	GLfloat tail = frame[3] - frame[2];
	O[ 0] = 1.0f / large;
	O[ 5] = 1.0f / tail;
	O[ 8] = (frame[0] + frame[1]) / large;
	O[ 9] = (frame[2] + frame[3]) / tail;
	O[10] = -(far + near) / (far - near);
	O[14] = -1.0f;
	O[11] = -(2.f * far * near) / (far- near);
}
#if 1
void mat4_perspective(const GLfloat angle, const GLfloat aspect, const GLfloat near, const GLfloat far, GLfloat O[])
{
	if (angle <= 0.0f || angle >= M_PI)
		return;
	GLfloat frame[4];
	frame[3] = tanf(angle/2);
	frame[2] = - frame[3];
	frame[1] = frame[3] * aspect;
	frame[0] = frame[2] * aspect;
	mat4_frustum(frame, near, far, O);
}
#else
void mat4_perspective(const GLfloat angle, const GLfloat aspect, const GLfloat near, const GLfloat far, GLfloat O[])
{
	if (angle <= 0.0f || angle >= M_PI)
		return;
	memset(O, 0, 16 *sizeof(GLfloat));
	GLfloat tanFOV = tanf(angle / 2);
	GLfloat range = far - near;
	O[ 0] = 1.0f / (tanFOV * aspect);
	O[ 5] = 1.0f / tanFOV;
	O[10] = -(near + far) / (range);
	O[11] = -2.0f * far *near / range;
	O[14] = -1.0f;
	O[15] = 0.0f;
}
#endif

#ifdef DEBUG
void mat4_log(const GLfloat mat[])
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

static float squaref(float value)
{
	return value * value;
}

float normalizef(float u, float v, float w)
{
	return sqrtf(squaref(u) + squaref(v) + squaref(w));
}

void vec3_normalize(const GLfloat I[], GLfloat O[])
{
	GLfloat N = normalizef(I[0], I[1], I[2]);
	if (N == 0)
		N = 1;
	O[0] = I[0] / N;
	O[1] = I[1] / N;
	O[2] = I[2] / N;
}

void vec3_multiply(const GLfloat A[], const GLfloat B[], GLfloat AB[])
{
	GLfloat TMP[3];
	TMP[0] = (A[1] * B[2]) - (A[2] * B[1]);
	TMP[1] = (A[2] * B[0]) - (A[0] * B[2]);
	TMP[2] = (A[0] * B[1]) - (A[1] * B[0]);
	memcpy(AB, TMP, sizeof(TMP));
}

GLfloat vec3_dot(const GLfloat A[], const GLfloat B[])
{
	GLfloat TMP = 0;
	for (int i = 0; i < 3; i++)
		TMP += A[i] * B[i];
	return TMP;
}

void mat4_lookat(const GLfloat eye[], const GLfloat center[], const GLfloat up[], GLfloat view[])
{
	GLfloat forward[3];
	vec3_add(center, -1, eye, forward);
	vec3_normalize(forward, forward);

	GLfloat right[3];
	vec3_multiply(forward, up, right);
	vec3_normalize(right, right);

	GLfloat uptmp[3];
	vec3_multiply(right, forward, uptmp);
	memset(view, 0, 16 * sizeof(GLfloat));
	view[0] = right[0];
	view[1] = uptmp[0];
	view[2] = -forward[0];

	view[4] = right[1];
	view[5] = uptmp[1];
	view[6] = -forward[1];

	view[8] = right[2];
	view[9] = uptmp[2];
	view[10] = -forward[2];
	//vec3_normalize(uptmp, uptmp);
	view[ 3] = -eye[0];
	view[ 7] = -eye[1];
	view[11] = -eye[2];

	view[15] = 1.0;
}

void mat4_ortho(const GLfloat frame[4], const GLfloat near, const GLfloat far, GLfloat O[16])
{
	memset(O, 0, sizeof(GLfloat) * 16);
	O[ 0] = 2.0f / (frame[1] - frame[0]);
	O[ 5] = 2.0f / (frame[3] - frame[2]);
	O[10] = -2.0f / (far - near);
	O[12] = -(frame[1] + frame[0]) / (frame[1] - frame[0]);
	O[13] = -(frame[3] + frame[2]) / (frame[3] - frame[2]);
	O[14] = -(far + near) / (far - near);
	O[15] = 1.0f;
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
	mq[ 1] =  ra->Z * rsin + xy * ircos;
	mq[ 2] = -ra->Y * rsin + xz * ircos;

	mq[ 4] = -ra->Z * rsin + xy * ircos;
	mq[ 5] =          rcos + yy * ircos;
	mq[ 6] =  ra->X * rsin + yz * ircos;

	mq[ 8] =  ra->Y * rsin + xz * ircos;
	mq[ 9] = -ra->X * rsin + yz * ircos;
	mq[10] =          rcos + zz * ircos;

	mq[3]  = mq[7] = mq[11] = mq[12] = mq[13] = mq[14] = 0;
	mq[15] = 1;
}

void mat4_rotate(const GLfloat angle, const GLfloat axis[3], GLfloat AB[16])
{
	GLMotor_RotAxis_t ra = {.A = angle, .X = axis[0], .Z = axis[1], .X = axis[2]};
	ra2mq(&ra, AB);
}

void mat4_translate(const GLfloat axis[3], const GLfloat length, GLfloat AB[16])
{
	AB[12] += axis[0] * length;
	AB[13] += axis[1] * length;
	/**
	 * !!! I don't understand
	 * the theory should modify index 14
	 */
#if 0
	AB[14] += axis[2] * length;
#else
	AB[11] += axis[2] * length;
#endif
}
