#include <stdlib.h>
#include <string.h>

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "log.h"
#include "glmotor.h"
#include "eglnative.h"

#include  <X11/Xlib.h>
#include  <X11/Xatom.h>
#include  <X11/Xutil.h>

#ifndef TRUE
#define TRUE GL_TRUE
#define FALSE GL_FALSE
#endif

// X11 related local variables
static Display *display = NULL;

static EGLNativeDisplayType native_display()
{
	if (display == NULL)
		/** environment management */
		display = XOpenDisplay(NULL);
	return (EGLNativeDisplayType)display;
}

static GLboolean native_running(EGLNativeWindowType native_win, GLMotor_t *motor)
{
	XEvent xev;
	KeySym key;
	GLboolean running = GL_TRUE;
	static GLMotor_Event_Keymode_t mode = 0;

	while (XPending(display))
	{
		char text = 0;
		XNextEvent(display, &xev);
		if (xev.type == KeyPress)
		{
			if (xev.xkey.keycode == 0x09)
				running = GL_FALSE;
		}
		if ( xev.type == DestroyNotify )
			running = GL_FALSE;
	}
	return running;
}

static EGLNativeWindowType native_createwindow(EGLNativeDisplayType display, GLuint width, GLuint height, const GLchar *name)
{

	Window root = DefaultRootWindow(display);

	XSetWindowAttributes swa;
	swa.event_mask  =  ExposureMask | PointerMotionMask | KeyPressMask | KeyReleaseMask;
	Window win;
	win = XCreateWindow(
				display, root,
				0, 0, width, height, 0,
				CopyFromParent, InputOutput,
				CopyFromParent, CWEventMask,
				&swa );

	XSetWindowAttributes  xattr;
	xattr.override_redirect = FALSE;
	XChangeWindowAttributes(display, win, CWOverrideRedirect, &xattr );

	XWMHints hints;
	hints.input = TRUE;
	hints.flags = InputHint;
	XSetWMHints(display, win, &hints);

	XMapWindow(display, win);
	XStoreName(display, win, name);

	Atom wm_state;
	wm_state = XInternAtom(display, "_NET_WM_STATE", FALSE);

	XEvent xev;
	memset(&xev, 0, sizeof(xev) );
	xev.type                 = ClientMessage;
	xev.xclient.window       = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format       = 32;
	xev.xclient.data.l[0]    = 1;
	xev.xclient.data.l[1]    = FALSE;
	XSendEvent(display, DefaultRootWindow(display ),
		FALSE, SubstructureNotifyMask, &xev );
	return (EGLNativeWindowType) win;
}

static void native_destroy(EGLNativeDisplayType native_display)
{
}

struct eglnative_motor_s *eglnative_x = &(struct eglnative_motor_s)
{
	.name = "x",
	.type = EGL_PBUFFER_BIT, /// EGL config must bind to texture
	.display = native_display,
	.createwindow = native_createwindow,
	.windowsize = NULL,
	.running = native_running,
	.destroy = native_destroy,
};
