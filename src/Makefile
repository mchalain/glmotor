ifneq ($(findstring $(HAVE_GBM) $(HAVE_WAYLAND_EGL)$(HAVE_X11),y),)
NEED_NATIVEJPEG=y
endif
slib-y+=glsurface
glsurface_SOURCES-$(GLUT)+=surfaceglut.c
glsurface_LIBS-$(GLUT)+=glut
glsurface_SOURCES-$(HAVE_GLFW3)+=surfaceglfw.c
glsurface_LIBRARY-$(GLFW)+=glfw3
glsurface_SOURCES-$(HAVE_EGL)+=surfaceegl.c
glsurface_LIBRARY-$(EGL)+=egl
glsurface_SOURCES-$(HAVE_LIBJPEG)+=eglnativejpeg.c
glsurface_SOURCES-$(HAVE_WAYLAND_EGL)+=eglnativewl.c
glsurface_LIBRARY-$(HAVE_WAYLAND_EGL)+=xkbcommon
glsurface_SOURCES-$(HAVE_X11)+=eglnativex.c
ifeq ($(HAVE_GBM),y)
glsurface_SOURCES-$(HAVE_LIBDRM)+=eglnativedrm.c
endif
ifeq ($(WAYLAND),y)
glsurface_LIBRARY-$(EGL)+=wayland-egl
endif
glsurface_LIBRARY-$(EGL)+=x11
glsurface_LIBRARY-$(EGL)+=libdrm
glsurface_LIBRARY-$(EGL)+=gbm
glsurface_LIBRARY-$(EGL)+=libjpeg

bin-y+=glmotor
glmotor_SOURCES+=main.c
glmotor_SOURCES+=glmotor.c
glmotor_SOURCES+=scene.c
glmotor_SOURCES+=object.c
glmotor_SOURCES+=texture.c
glmotor_SOURCES+=dynloader.c
glmotor_LIBS+=glsurface
glmotor_LIBS-$(GLUT)+=glut
glmotor_LIBRARY-$(GLFW)+=glfw3
glmotor_LIBRARY-$(EGL)+=egl
ifeq ($(WAYLAND),y)
glmotor_LIBRARY-$(EGL)+=wayland-egl
endif
glmotor_LIBRARY-$(EGL)+=x11
glmotor_LIBRARY-$(EGL)+=libdrm
glmotor_LIBRARY-$(EGL)+=gbm
glmotor_LIBRARY-$(EGL)+=libjpeg
glmotor_LIBRARY-$(GLEW)+=glew
glmotor_LIBRARY-$(GLESV2)+=glesv2
#glmotor_LIBRARY-$(GLESV2:n=y)+=gl
glmotor_LIBS+=m
glmotor_LIBRARY+=glu
