#include <string.h>
#include <math.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
#else
# include <GL/gl.h>
#endif

#include "glmotor.h"

void mat4_multiply4(GLfloat A[], GLfloat B[], GLfloat AB[])
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

void mat4_add4(GLfloat A[], GLfloat B[], GLfloat AB[], int sign)
{
	if (sign < 0)
		sign = -1;
	else
		sign = 1;
	AB[ 0] = A[ 0] + sign * B[ 0];
	AB[ 1] = A[ 1] + sign * B[ 1];
	AB[ 2] = A[ 2] + sign * B[ 2];
	AB[ 3] = A[ 3] + sign * B[ 3];
	AB[ 4] = A[ 4] + sign * B[ 4];
	AB[ 5] = A[ 5] + sign * B[ 5];
	AB[ 6] = A[ 6] + sign * B[ 6];
	AB[ 7] = A[ 7] + sign * B[ 7];
	AB[ 8] = A[ 8] + sign * B[ 8];
	AB[ 9] = A[ 9] + sign * B[ 9];
	AB[10] = A[10] + sign * B[10];
	AB[11] = A[11] + sign * B[11];
	AB[12] = A[12] + sign * B[12];
	AB[13] = A[13] + sign * B[13];
	AB[14] = A[14] + sign * B[14];
	AB[15] = A[15] + sign * B[15];
}

#if 0
void mat4_log(GLfloat mat[])
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
