#ifndef __GLMOTOR_H__
#define __GLMOTOR_H__

#define GLMOTOR_EXPORT

#ifndef GLMOTOR_SURFACE_S
#define GLMOTOR_SURFACE_S void
#endif

typedef void (*GLMotor_Draw_func_t)(void *);

typedef GLMOTOR_SURFACE_S GLMotor_Surface_t;

typedef struct GLMotor_s GLMotor_t;
struct GLMotor_s
{
	GLuint programID;
	GLMotor_Surface_t *surf;
	GLuint width;
	GLuint height;
};

GLMOTOR_EXPORT GLMotor_t *glmotor_create(int argc, char** argv);
GLMOTOR_EXPORT GLuint glmotor_build(GLMotor_t *motor, GLchar *vertex, GLuint vertexSize, GLchar *fragment, GLuint fragmentSize);
GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata);
GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor);

typedef struct GLMotor_Object_s GLMotor_Object_t;
GLMOTOR_EXPORT GLMotor_Object_t *create_object(GLMotor_t *motor, GLchar *name, GLuint npoints, GLfloat *points);
GLMOTOR_EXPORT GLuint draw_object(GLMotor_Object_t *obj);

typedef struct GLMotor_Texture_s GLMotor_Texture_t;
GLMOTOR_EXPORT GLMotor_Texture_t *create_texture(GLMotor_t *motor, GLuint width, GLuint height, GLchar *map);

typedef struct GLMotor_Scene_s GLMotor_Scene_t;
GLMOTOR_EXPORT GLMotor_Scene_t *create_scene(GLMotor_t *motor);
GLMOTOR_EXPORT void append_object(GLMotor_Scene_t *scene, GLMotor_Object_t *obj);
GLMOTOR_EXPORT void move_camera(GLMotor_Scene_t *scene, const GLfloat *camera, const GLfloat *target);
GLMOTOR_EXPORT void draw_scene(GLMotor_Scene_t *scene);
GLMOTOR_EXPORT void destroy_scene(GLMotor_Scene_t *scene);

#endif
