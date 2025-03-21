#ifndef __LOADER_H__
#define __LOADER_H__

#include "glmotor.h"

GLMOTOR_EXPORT GLuint program_load(const char *vertex, const char *fragments[], int nbfragments);
GLMOTOR_EXPORT GLMotor_Object_t * object_load(GLMotor_t *motor, GLchar *name, const char *file, GLuint programID);

#endif
