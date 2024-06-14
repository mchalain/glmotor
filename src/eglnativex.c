#include <string.h>
#include <GL/gl.h>
#include <EGL/egl.h>

#include "log.h"

#include  <X11/Xlib.h>
#include  <X11/Xatom.h>
#include  <X11/Xutil.h>

#ifndef TRUE
#define TRUE GL_TRUE
#define FALSE GL_FALSE
#endif

// X11 related local variables
static Display *display = NULL;

EGLNativeDisplayType native_display()
{
	if (display == NULL)
		/** environment management */
		display = XOpenDisplay(NULL);
	return (EGLNativeDisplayType)display;
}

GLboolean native_running(EGLNativeWindowType native_win)
{
	XEvent xev;
	KeySym key;
	GLboolean running = GL_TRUE;
	char text;

	while (XPending(display))
	{
		XNextEvent(display, &xev);
		if (xev.type == KeyPress)
		{
			if (XLookupString(&xev.xkey,&text,1,&key,0)==1)
			{
				if (text == 'q')
					running = GL_FALSE;
			}
			if (xev.xkey.keycode == 0x09)
				running = GL_FALSE;
		}
		if ( xev.type == DestroyNotify )
			running = GL_FALSE;
	}
	return running;
}

EGLNativeWindowType native_createwindow(EGLNativeDisplayType display, GLuint width, GLuint height, const GLchar *name)
{
	
	Window root = DefaultRootWindow(display);

	XSetWindowAttributes swa;
	swa.event_mask  =  ExposureMask | PointerMotionMask | KeyPressMask;
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

void native_destroy(EGLNativeDisplayType native_display)
{
}
