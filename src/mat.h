#ifndef __MAT_H__
#define __MAT_H__

void mat4_multiply4(GLfloat A[], GLfloat B[], GLfloat AB[]);
void mat4_add4(GLfloat A[], GLfloat B[], GLfloat AB[], int sign);
float normalizef(float u, float v, float w);
void ra2mq(GLMotor_RotAxis_t *ra, GLfloat mq[]);

#endif
