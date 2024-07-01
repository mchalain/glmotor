#ifndef __GLMOTOR_H__
#define __GLMOTOR_H__

#define GLMOTOR_EXPORT

#ifndef GLMOTOR_SURFACE_S
#define GLMOTOR_SURFACE_S void
#endif

#ifndef FOURCC
#define FOURCC(a,b,c,d)	((a << 0) | (b << 8) | (c << 16) | (d << 24))
#endif

typedef void (*GLMotor_Draw_func_t)(void *);

typedef GLMOTOR_SURFACE_S GLMotor_Surface_t;

typedef struct GLMotor_list_s GLMotor_list_t;
struct GLMotor_list_s
{
	void *entity;
	GLMotor_list_t *next;
};

typedef struct GLMotor_Event_s GLMotor_Event_t;
typedef enum
{
	MODE_SHIFT = 0x01,
	MODE_CTRL = 0x02,
	MODE_ALT = 0x04,
} GLMotor_Event_Keymode_t;
struct GLMotor_Event_s
{
	enum{
		EVT_KEY,
	} type;
	union{
		struct
		{
			GLMotor_Event_Keymode_t mode;
			int code;
			char value;
		}key;
	} data;
	GLMotor_Event_t *next;
};

typedef int (*GLMotor_Event_func_t)(void *cbdata, GLMotor_Event_t *event);

typedef struct GLMotor_s GLMotor_t;
struct GLMotor_s
{
	GLuint programID;
	GLMotor_Surface_t *surf;
	GLuint width;
	GLuint height;
	GLMotor_Event_func_t eventcb;
	void *eventcbdata;
	GLMotor_Event_t *events;
};

typedef struct GLMotor_RotAxis_s GLMotor_RotAxis_t;
struct GLMotor_RotAxis_s
{
	GLfloat A;
	GLfloat X;
	GLfloat Y;
	GLfloat Z;
};

GLMOTOR_EXPORT GLMotor_t *glmotor_create(int argc, char** argv);
GLMOTOR_EXPORT GLuint glmotor_build(GLMotor_t *motor, GLchar *vertex, GLuint vertexSize, GLchar *fragment, GLuint fragmentSize);
GLMOTOR_EXPORT void glmotor_seteventcb(GLMotor_t *motor, GLMotor_Event_func_t cb, void * cbdata);
GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata);
GLMOTOR_EXPORT GLuint glmotor_swapbuffers(GLMotor_t *motor);
GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor);

typedef struct GLMotor_Texture_s GLMotor_Texture_t;
GLMOTOR_EXPORT GLMotor_Texture_t *texture_create(GLMotor_t *motor, GLuint width, GLuint height, uint32_t fourcc, GLuint mipmaps, GLchar *map);
GLMOTOR_EXPORT GLMotor_Texture_t *texture_fromcamera(GLMotor_t *motor, const char *device, GLuint width, GLuint height, uint32_t fourcc);
GLMOTOR_EXPORT GLMotor_Texture_t * texture_load(GLMotor_t *motor, char *fileName);
GLMOTOR_EXPORT GLint texture_draw(GLMotor_Texture_t *tex);
GLMOTOR_EXPORT void texture_destroy(GLMotor_Texture_t *tex);

#define COLOR_COMPONENTS 4
typedef struct GLMotor_Object_s GLMotor_Object_t;
GLMOTOR_EXPORT GLMotor_Object_t *object_create(GLMotor_t *motor, const char *name, GLuint maxpoints, GLuint maxfaces);
GLMOTOR_EXPORT GLuint object_appendpoint(GLMotor_Object_t *obj, GLuint npoints, GLfloat points[]);
GLMOTOR_EXPORT GLuint object_appendface(GLMotor_Object_t *obj, GLuint nfaces, GLuint face[]);
GLMOTOR_EXPORT GLuint object_appendcolor(GLMotor_Object_t *obj, GLuint ncolors, GLfloat colors[]);
GLMOTOR_EXPORT GLuint object_appenduv(GLMotor_Object_t *obj, GLuint nuvs, GLfloat uvs[]);
GLMOTOR_EXPORT GLuint object_appendnormal(GLMotor_Object_t *obj, GLuint nnormals, GLfloat normals[]);
GLMOTOR_EXPORT GLuint object_addtexture(GLMotor_Object_t *obj, GLMotor_Texture_t *tex);
GLMOTOR_EXPORT void object_move(GLMotor_Object_t *obj, GLfloat translate[], GLMotor_RotAxis_t *ra);
GLMOTOR_EXPORT GLfloat* object_positionmatrix(GLMotor_Object_t *obj);
GLMOTOR_EXPORT GLint object_draw(GLMotor_Object_t *obj);
GLMOTOR_EXPORT const char * object_name(GLMotor_Object_t *obj);
GLMOTOR_EXPORT void object_destroy(GLMotor_Object_t *obj);

typedef struct GLMotor_Scene_s GLMotor_Scene_t;
GLMOTOR_EXPORT GLMotor_Scene_t *scene_create(GLMotor_t *motor);
GLMOTOR_EXPORT void scene_appendobject(GLMotor_Scene_t *scene, GLMotor_Object_t *obj);
GLMOTOR_EXPORT GLMotor_Object_t *scene_getobject(GLMotor_Scene_t *scene, const char *name);
GLMOTOR_EXPORT void scene_movecamera(GLMotor_Scene_t *scene, const GLfloat *camera, const GLfloat *target);
GLMOTOR_EXPORT GLint scene_draw(GLMotor_Scene_t *scene);
GLMOTOR_EXPORT void scene_destroy(GLMotor_Scene_t *scene);

#endif
