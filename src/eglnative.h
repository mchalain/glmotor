#ifndef __EGLNATIVE_H__
#define __EGLNATIVE_H__

#include <EGL/egl.h>

GLboolean native_running(EGLNativeWindowType native_win);
EGLNativeDisplayType native_display();
EGLNativeWindowType native_createwindow(GLuint width, GLuint height, const GLchar *name);

#endif
