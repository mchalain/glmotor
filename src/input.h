#ifndef __GLMOTOR_INPUT_H__
#define __GLMOTOR_INPUT_H__

typedef struct GLMotor_Input_s GLMotor_Input_t;

GLMOTOR_EXPORT GLMotor_Input_t *input_create(GLMotor_Scene_t *scene);
GLMOTOR_EXPORT void input_destroy(GLMotor_Input_t *input);

#endif
