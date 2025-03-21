#ifndef __LOADER_H__
#define __LOADER_H__

#include "glmotor.h"

GLMOTOR_EXPORT GLuint glmotor_load(GLMotor_t *motor, const char *vertex, const char *fragments[], int nbframents);
GLMOTOR_EXPORT GLMotor_Object_t * object_load(GLMotor_t *motor, GLchar *name, const char *file);

#endif
