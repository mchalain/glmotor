#ifndef __MAT_H__
#define __MAT_H__

void mat4_diag(GLfloat A[]);
void mat4_multiply4(GLfloat A[], GLfloat B[], GLfloat AB[]);
void mat4_add4(GLfloat A[], int sign, GLfloat B[], GLfloat AB[]);
void mat4_lookat(GLfloat eye[], GLfloat center[], GLfloat up[], GLfloat view[]);
void mat4_frustum(GLfloat frame[], GLfloat near, GLfloat far, GLfloat O[]);
void mat4_perspective(GLfloat angle, GLfloat aspect, GLfloat near, GLfloat far, GLfloat O[]);

void mat_add(GLfloat A[], int sign, GLfloat B[], GLfloat AB[], unsigned int size);

void vec3_multiply(GLfloat A[], GLfloat B[], GLfloat AB[]);
void vec3_normalize(GLfloat I[], GLfloat O[]);
void vec3_add(GLfloat A[], int sign, GLfloat B[], GLfloat AB[]);

void vec_add(GLfloat A[], int sign, GLfloat B[], GLfloat AB[], unsigned int size);

void ra2mq(GLMotor_RotAxis_t *ra, GLfloat mq[]);
float normalizef(float u, float v, float w);

#endif
