#ifndef __LOADER_H__
#define __LOADER_H__

#include "glmotor.h"

GLMOTOR_EXPORT GLuint load_shader(GLMotor_t *motor, const char *vertex, const char *fragment);
GLMOTOR_EXPORT GLMotor_Object_t * load_obj(GLMotor_t *motor, GLchar *name, const char *file);

#endif
