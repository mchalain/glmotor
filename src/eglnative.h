#ifndef __EGLNATIVE_H__
#define __EGLNATIVE_H__

#include <EGL/egl.h>

EGLNativeDisplayType native_display();
EGLNativeWindowType native_createwindow(EGLNativeDisplayType native_display,
							GLuint width, GLuint height, const GLchar *name);
GLboolean native_running(EGLNativeWindowType native_win);
void native_destroy(EGLNativeDisplayType native_display);

#endif
