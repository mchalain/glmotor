#ifndef __MAT_H__
#define __MAT_H__

void mat4_scale(const GLfloat scale, GLfloat A[16]);
void mat4_diag(GLfloat A[16]);
void mat4_multiply4(const GLfloat A[16], const GLfloat B[16], GLfloat AB[16]);
void mat4_add4(const GLfloat A[16], int sign, const GLfloat B[16], GLfloat AB[16]);
void mat4_lookat(const GLfloat eye[3], const GLfloat center[3], const GLfloat up[3], GLfloat view[16]);
void mat4_frustum(const GLfloat frame[4], const GLfloat near, const GLfloat far, GLfloat O[16]);
void mat4_perspective(const GLfloat angle, const GLfloat aspect, const GLfloat near, const GLfloat far, GLfloat O[16]);
void mat4_ortho(const GLfloat frame[4], const GLfloat near, const GLfloat far, GLfloat O[16]);
void mat4_rotate(const GLfloat angle, const GLfloat axis[3], GLfloat O[16]);
void mat4_translate(const GLfloat axis[3], const GLfloat length, GLfloat AB[16]);

void mat_add(const GLfloat A[], int sign, const GLfloat B[], GLfloat AB[], unsigned int size);

void vec3_multiply(const GLfloat A[], const GLfloat B[], GLfloat AB[3]);
void vec3_normalize(const GLfloat I[], const GLfloat O[3]);
void vec3_add(const GLfloat A[], int sign, const GLfloat B[], GLfloat AB[3]);

void vec_add(const GLfloat A[], int sign, const GLfloat B[], GLfloat AB[], unsigned int size);

void ra2mq(GLMotor_RotAxis_t *ra, GLfloat mq[]);
float normalizef(float u, float v, float w);

#endif
