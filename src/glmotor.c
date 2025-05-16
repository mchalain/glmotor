#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
# undef HAVE_GLEW
#else
# ifdef HAVE_GLEW
#  include <GL/glew.h>
# endif
# include <GL/gl.h>
#endif
#ifdef HAVE_GLU
# include <GL/glu.h>
#endif

#include "glmotor.h"
#include "loader.h"
#include "log.h"

#ifdef HAVE_GLEW
static int init_glew()
{
	if ( glewInit() != GLEW_OK )
	{
	    err("glmotor: glewInit error %m");
	    return -1;
	}

	if ( !glewIsSupported("GL_ARB_shading_language_100") )
	{
	    err("glmotor: GL_ARB_shading_language_100 error %m");
		return -2;
	}
	if ( !glewIsSupported("GL_ARB_shader_objects") )
	{
	    err("glmotor: GL_ARB_shader_objects error %m");
		return -3;
	}
	if ( !glewIsSupported("GL_ARB_vertex_shader") )
	{
	    err("glmotor: GL_ARB_vertex_shader error %m");
		return -4;
	}
	if ( !glewIsSupported("GL_ARB_fragment_shader") )
	{
	    err("glmotor: GL_ARB_fragment_shader error %m");
		return -5;
	}
	return 0;
}
#endif

GLMOTOR_EXPORT GLMotor_t *glmotor_create(GLMotor_config_t *config, int argc, char** argv)
{
	GLMotor_Surface_t *window = NULL;
	int mode = 0;
	int opt;
	optind = 1;
	do
	{
		opt = getopt(argc, argv, "Oh");
		switch (opt)
		{
			case 'O':
				mode = 1;
			break;
			case 'h':
				printf("\t-O\t\tOffscreeen mode\n");
				return NULL;
			break;
		}
	} while (opt != -1);
	glViewport(0, 0, config->width, config->height);

	if (mode == 0)
		window = surface_create(config, argc, argv);

	if (window == NULL)
	{
		warn("glmotor: offscreen mode");
	}
#ifdef HAVE_GLEW
	/**
	 * glew must be call after surface context
	 */
	glewExperimental = 1;
	if (init_glew())
		return NULL;
#endif
	GLMotor_t *motor = calloc(1, sizeof(*motor));
	motor->width = config->width;
	motor->height = config->height;
	motor->surf = window;
	warn("glmotor uses : %s", glGetString(GL_VERSION));
	warn("glmotor uses : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
#ifdef DEBUG
	dbg("glmotor: gl extensions");
	const char * extensions = glGetString(GL_EXTENSIONS);
	const char * end;
	if (extensions)
		end = strchr(extensions, ' ');
	else
		dbg("\tnone");
	while (extensions != NULL)
	{
		size_t length = strlen(extensions);
		if (end)
			length = end - extensions;
		dbg("\t%.*s", length, extensions);
		extensions = end;
		if (end)
		{
			extensions++;
			end = strchr(extensions, ' ');
		}
	}
#endif

	return motor;
}

GLMOTOR_EXPORT GLuint glmotor_load(GLMotor_t *motor, const char *vertex, const char *fragments[], int nbfragments)
{
	/**
	 * As the texture of the resulting of the first program is not mapped on the input of the next one
	 * it is useless to add several programs.
	 * It may be usefull for special object as screen.
	 */
	GLuint programID = program_load(vertex, fragments, nbfragments);
	if (motor->nbprograms < MAX_PROGS)
		motor->programID[motor->nbprograms++] = programID;
	return programID;
}

#ifndef GLUT
GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata)
{
	int ret = 0;
	do
	{
		surface_prepare(motor->surf, motor);
		draw(drawdata);
#ifdef DEBUG
		static uint32_t nbframes = 0;
		nbframes++;
		static time_t start = 0;
		time_t now = time(NULL);
		if (start == 0)
			start = time(NULL);
		else if (start < now)
		{
			dbg("glmotor: %lu fps", nbframes / (now - start));
			start = now;
			nbframes = 0;
		}
#endif
		glFlush();
		if (motor->surf)
		{
			surface_swapbuffers(motor->surf);
			ret = surface_running(motor->surf, motor);
		}
	} while (ret);
	return 0;
}
#endif

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	if (motor->surf)
		surface_destroy(motor->surf);
	free(motor);
}

GLMOTOR_EXPORT GLMotor_Offscreen_t *glmotor_offscreen_create(GLMotor_config_t *config)
{
	/**
	 * the framebuffer is accessible only if the context is enabledd (eglMakeCurrent)
	 **/
	dbg("glmotor: offscreen generator");
	GLuint fbo = 0, rbo = 0, rbo_depth = 0, rbo_stencil = 0, texture = 0;
	GLuint glget = 0;
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &glget);
	if (glget <= config->width)
		warn("glmotor: width to large max %d", glget);
	if (glget <= config->height)
		warn("glmotor: width to height max %d", glget);

	/*  Framebuffer */
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

#if GLMOTOR_RENDER_BUFFER
	GLuint samples, format , width, height;

	/* Color renderbuffer. */
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	/* Storage must be one of: */
	/* GL_RGBA4, GL_RGB565, GL_RGB5_A1, GL_DEPTH_COMPONENT16, GL_STENCIL_INDEX8. */
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, config->width, config->height);
	dbg("glmotor: render frame config size %ux%u", config->width, config->height);

	//glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &format);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);

	dbg("glmotor: render frame size %ux%u %u, %u", width, height, samples, format);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
#endif

#if GLMOTOR_TEX_BUFFER
	glGenTextures(1, &texture);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, config->width, config->height, 0,
				GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
#endif

#if GLMOTOR_DEPTH_BUFFER
	/* Depth renderbuffer. */
	glGenRenderbuffers(1, &rbo_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, config->width, config->height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);
#endif

#if GLMOTOR_STENCIL_BUFFER
	/* Depth renderbuffer. */
	glGenRenderbuffers(1, &rbo_stencil);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_stencil);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, config->width, config->height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_stencil);
#endif

	/* Sanity check. */
	GLint ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (ret != GL_FRAMEBUFFER_COMPLETE)
	{
		err("glmotor: offscreen generator failed");
		return NULL;
	}
	GLMotor_Offscreen_t *off = calloc(1, sizeof(*off));
	off->size = config->width * config->height * sizeof(uint32_t);
	off->width = config->width;
	off->height = config->height;
	off->fbo = fbo;
	off->rbo_color = rbo;
	off->rbo_depth = rbo_depth;
	off->rbo_stencil = rbo_stencil;
	off->texture = texture;
	return off;
}

GLMOTOR_EXPORT void glmotor_offscreen_destroy(GLMotor_Offscreen_t *off)
{
	glDeleteFramebuffers(1, &off->fbo);
	free(off);
}
