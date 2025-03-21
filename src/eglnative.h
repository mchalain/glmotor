#ifndef __EGLNATIVE_H__
#define __EGLNATIVE_H__

#include <EGL/egl.h>

struct eglnative_motor_s
{
	const char *name;
	EGLint type;
	EGLNativeDisplayType (*display)();
	EGLNativeWindowType (*createwindow)(EGLNativeDisplayType native_display,
							GLuint width, GLuint height, const GLchar *name);
	void (*windowsize)(EGLNativeWindowType native_win, GLuint *width, GLuint *height);
	GLboolean (*running)(EGLNativeWindowType native_win, GLMotor_t *motor);
	void (*destroy)(EGLNativeDisplayType native_display);
};

extern struct eglnative_motor_s *eglnative_drm;
extern struct eglnative_motor_s *eglnative_jpeg;
extern struct eglnative_motor_s *eglnative_wl;
extern struct eglnative_motor_s *eglnative_x;
extern struct eglnative_motor_s *eglnative_pbuffer;

extern void* pbufferconsumerhdl;

#endif
