#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>
#include <EGL/egl.h>

#include <jpeglib.h>

#include "log.h"
#include "glmotor.h"
#include "eglnative.h"

static GLuint g_width;
static GLuint g_height;

static EGLNativeDisplayType native_display()
{
	return (EGLNativeDisplayType)EGL_DEFAULT_DISPLAY;
}

static GLboolean native_running(EGLNativeWindowType native_win, GLMotor_t *motor)
{
	unsigned char *buffer =
		(unsigned char *)malloc(g_width * g_height * COLOR_COMPONENTS);

	GLuint components = (COLOR_COMPONENTS == 3)?GL_RGB : GL_RGBA;
    glReadPixels(0, 0, g_width, g_height, components, GL_UNSIGNED_BYTE,
                 buffer);

    FILE *output = fopen("screen.jpg", "wb");
    if (output)
    {
		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr jerr;

		jpeg_create_compress(&cinfo);
		cinfo.err = jpeg_std_error(&jerr);
		cinfo.image_width = g_width;
		cinfo.image_height = g_height;
		cinfo.input_components = COLOR_COMPONENTS;
		cinfo.in_color_space = (COLOR_COMPONENTS == 3)?JCS_EXT_RGB:JCS_EXT_RGBX;
		jpeg_set_defaults(&cinfo);

		jpeg_stdio_dest(&cinfo, output);
		jpeg_start_compress(&cinfo, TRUE);
		for (int i = 0; i < g_height; i++)
		{
			JSAMPROW row_pointer[1];
			row_pointer[0] =  &buffer[i * g_width * cinfo.input_components];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}
		jpeg_finish_compress(&cinfo);
        fclose(output);
    }
    else
    {
        err("glmotor:Failed to open file screen.raw for writing!");
    }
	return 0;
}

static EGLNativeWindowType native_createwindow(EGLNativeDisplayType display, GLuint width, GLuint height, const GLchar *name)
{
	g_width = width;
	g_height = height;
	
	return (EGLNativeWindowType) NULL;
}

static void native_destroy(EGLNativeDisplayType native_display)
{
}

struct eglnative_motor_s *eglnative_jpeg = &(struct eglnative_motor_s)
{
	.name = "jpeg",
	.display = native_display,
	.createwindow = native_createwindow,
	.running = native_running,
	.destroy = native_destroy,
};
