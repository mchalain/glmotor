#ifndef __LOADER_H__
#define __LOADER_H__

#include "glmotor.h"

GLMOTOR_EXPORT GLuint load_shader(GLMotor_t *motor, const char *vertex, const char *fragment);
GLMOTOR_EXPORT GLuint build_program(GLMotor_t *motor, GLchar *vertex, GLuint vertexSize, GLchar *fragment, GLuint fragmentSize);
GLMOTOR_EXPORT GLMotor_Object_t * load_obj(GLMotor_t *motor, const char *file);

#endif
