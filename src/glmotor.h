#ifndef __GLMOTOR_H__
#define __GLMOTOR_H__

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>

extern PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES;
extern PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES;

# define glBindVertexArray glBindVertexArrayOES
# define glGenVertexArrays glGenVertexArraysOES

# ifndef GL_R8
#  define GL_R8 GL_R8_EXT
# endif
# ifndef GL_UNPACK_ROW_LENGTH
#  define GL_UNPACK_ROW_LENGTH GL_UNPACK_ROW_LENGTH_EXT
# endif
# ifndef GL_RGBA8
#  define GL_RGBA8 GL_RGBA8_OES
# endif
#else
# ifdef HAVE_GLEW
#  include <GL/glew.h>
# endif
# include <GL/gl.h>
# include <GL/glext.h>
#endif

#define GLMOTOR_RENDER_BUFFER 0
#define GLMOTOR_TEX_BUFFER 1
#define GLMOTOR_DEPTH_BUFFER 1
#define GLMOTOR_STENCIL_BUFFER 0

#define GLMOTOR_EXPORT

#ifndef GLMOTOR_SURFACE_S
#define GLMOTOR_SURFACE_S void
#endif

#ifndef FOURCC
#define FOURCC(a,b,c,d)	((a << 0) | (b << 8) | (c << 16) | (d << 24))
#endif

#define MAX_FRAGS 4
#define MAX_PROGS 4

typedef void (*GLMotor_Draw_func_t)(void *);

typedef GLMOTOR_SURFACE_S GLMotor_Surface_t;

typedef struct GLMotor_config_s GLMotor_config_t;
struct GLMotor_config_s
{
	const char *rootfs;
	const char *vertexshader;
	const char *fragmentshader[MAX_FRAGS];
	const char *object;
	const char *texture;
	GLuint width;
	GLuint height;
	int nbfragmentshaders;
	GLfloat camera[3];
	GLfloat perspective[3];
	GLfloat background[4];
	GLfloat zoom;
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

typedef struct GLMotor_Offscreen_s GLMotor_Offscreen_t;
struct GLMotor_Offscreen_s
{
	size_t size;
	GLuint width;
	GLuint height;
	GLuint fbo;
	GLuint rbo_color;
	GLuint rbo_depth;
	GLuint rbo_stencil;
	GLuint texture;
};

typedef struct GLMotor_s GLMotor_t;
struct GLMotor_s
{
	GLuint programID[MAX_PROGS];
	int nbprograms;
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
	GLfloat X;
	GLfloat Y;
	GLfloat Z;
	GLfloat A;
};

typedef union GLMotor_Rotate_s GLMotor_Rotate_t;
union GLMotor_Rotate_s
{
	GLMotor_RotAxis_t ra;
	GLfloat mq[16];
};

typedef union GLMotor_Translate_s GLMotor_Translate_t;
union GLMotor_Translate_s
{
	GLfloat mat[4];
	struct {
		GLfloat X;
		GLfloat Y;
		GLfloat Z;
		GLfloat L;
	} coord;
};

GLMOTOR_EXPORT GLMotor_t *glmotor_create(GLMotor_config_t *config, int argc, char** argv);
GLMOTOR_EXPORT GLuint glmotor_load(GLMotor_t *motor, const char *vertex, const char *fragments[], int nbframents);
GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata);
GLMOTOR_EXPORT GLuint glmotor_swapbuffers(GLMotor_t *motor);
GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor);

GLMOTOR_EXPORT GLMotor_Surface_t *surface_create(GLMotor_config_t *config, int argc, char** argv);
GLMOTOR_EXPORT int surface_running(GLMotor_Surface_t *surface, GLMotor_t *motor);
GLMOTOR_EXPORT int surface_prepare(GLMotor_Surface_t *surface, GLMotor_t *motor);
GLMOTOR_EXPORT GLuint surface_swapbuffers(GLMotor_Surface_t *surf);
GLMOTOR_EXPORT void surface_destroy(GLMotor_Surface_t *surface);

typedef struct GLMotor_Texture_s GLMotor_Texture_t;
GLMOTOR_EXPORT GLMotor_Texture_t *texture_create(GLMotor_t *motor, GLuint width, GLuint height, uint32_t fourcc, GLuint mipmaps, GLchar *map);
GLMOTOR_EXPORT GLint texture_setprogram(GLMotor_Texture_t *tex, GLuint programID);
GLMOTOR_EXPORT GLMotor_Texture_t *texture_fromcamera(GLMotor_t *motor, const char *device, GLuint width, GLuint height, uint32_t fourcc);
GLMOTOR_EXPORT GLMotor_Texture_t * texture_load(GLMotor_t *motor, const char *fileName);
GLMOTOR_EXPORT GLint texture_draw(GLMotor_Texture_t *tex);
GLMOTOR_EXPORT void texture_destroy(GLMotor_Texture_t *tex);

#define COLOR_COMPONENTS 4
typedef struct GLMotor_Object_s GLMotor_Object_t;
GLMOTOR_EXPORT GLMotor_Object_t *object_create(GLMotor_t *motor, const char *name, GLuint maxpoints, GLuint maxfaces);
GLMOTOR_EXPORT GLint object_setprogram(GLMotor_Object_t *obj, GLuint programID);
GLMOTOR_EXPORT GLuint object_appendpoint(GLMotor_Object_t *obj, GLuint npoints, GLfloat points[]);
GLMOTOR_EXPORT GLuint object_appendface(GLMotor_Object_t *obj, GLuint nfaces, GLuint face[]);
GLMOTOR_EXPORT GLuint object_appendcolor(GLMotor_Object_t *obj, GLuint ncolors, GLfloat colors[]);
GLMOTOR_EXPORT GLuint object_appenduv(GLMotor_Object_t *obj, GLuint nuvs, GLfloat uvs[]);
GLMOTOR_EXPORT GLuint object_appendnormal(GLMotor_Object_t *obj, GLuint nnormals, GLfloat normals[]);
GLMOTOR_EXPORT GLuint object_addtexture(GLMotor_Object_t *obj, GLMotor_Texture_t *tex);
GLMOTOR_EXPORT void object_move(GLMotor_Object_t *obj, GLMotor_Translate_t *tr, GLMotor_Rotate_t *rot);
GLMOTOR_EXPORT void object_movereset(GLMotor_Object_t *obj);
GLMOTOR_EXPORT const GLfloat* object_positionmatrix(GLMotor_Object_t *obj);
GLMOTOR_EXPORT GLint object_kinematic(GLMotor_Object_t *obj, GLMotor_Translate_t **tr, GLMotor_Rotate_t **rot);
GLMOTOR_EXPORT void object_appendkinematic(GLMotor_Object_t *obj, GLMotor_Translate_t *tr, GLMotor_Rotate_t *rot, int repeats);
GLMOTOR_EXPORT GLint object_draw(GLMotor_Object_t *obj, GLfloat *view);
GLMOTOR_EXPORT const char * object_name(GLMotor_Object_t *obj);
GLMOTOR_EXPORT void object_destroy(GLMotor_Object_t *obj);

typedef struct GLMotor_Scene_s GLMotor_Scene_t;
GLMOTOR_EXPORT GLMotor_Scene_t *scene_create(GLMotor_t *motor);
GLMOTOR_EXPORT void scene_setbackground(GLMotor_Scene_t *scene, GLfloat color[4]);
GLMOTOR_EXPORT void scene_appendobject(GLMotor_Scene_t *scene, GLMotor_Object_t *obj);
GLMOTOR_EXPORT GLMotor_Object_t *scene_getobject(GLMotor_Scene_t *scene, const char *name);
GLMOTOR_EXPORT GLMotor_Object_t *scene_nextobject(GLMotor_Scene_t *scene, GLMotor_Object_t *prev);
GLMOTOR_EXPORT void scene_movecamera(GLMotor_Scene_t *scene, const GLfloat camera[3], const GLfloat target[3]);
GLMOTOR_EXPORT void scene_perspective(GLMotor_Scene_t *scene, const GLfloat angle, const GLfloat near, const GLfloat far);
GLMOTOR_EXPORT void scene_zoom(GLMotor_Scene_t *scene, const GLfloat scale);
GLMOTOR_EXPORT void scene_reset(GLMotor_Scene_t *scene);
GLMOTOR_EXPORT GLint scene_draw(GLMotor_Scene_t *scene);
GLMOTOR_EXPORT GLuint scene_width(GLMotor_Scene_t *scene);
GLMOTOR_EXPORT GLuint scene_height(GLMotor_Scene_t *scene);
GLMOTOR_EXPORT void scene_destroy(GLMotor_Scene_t *scene);

GLMOTOR_EXPORT GLMotor_Offscreen_t *glmotor_offscreen_create(GLMotor_config_t *config);
GLMOTOR_EXPORT void glmotor_offscreen_destroy(GLMotor_Offscreen_t *off);

#ifdef HAVE_EGL
#define EGLAttrib intptr_t
GLMOTOR_EXPORT int glmotor_eglTexImage2D(GLuint texturefamily, GLuint ID, GLuint width, GLuint height, EGLAttrib *attribs);
#endif

#endif
