subdir-$(EGL)+=pbufferconsumer.mk

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
glsurface_SOURCES-$(EGL:y=$(HAVE_LIBJPEG))+=eglnativejpeg.c
glsurface_SOURCES-$(HAVE_WAYLAND_EGL)+=eglnativewl.c
glsurface_LIBRARY-$(HAVE_WAYLAND_EGL)+=xkbcommon
glsurface_SOURCES-$(EGL:y=$(HAVE_X11))+=eglnativex.c
glsurface_SOURCES-$(EGL)+=eglnativepbuffer.c
glsurface_SOURCES-$(EGL:y=$(HAVE_GBM:y=$(HAVE_LIBDRM)))+=eglnativedrm.c
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
glmotor_SOURCES+=mat.c
glmotor_SOURCES+=scene.c
glmotor_SOURCES+=object.c
glmotor_SOURCES+=texture.c
glmotor_SOURCES+=dynloader.c
glmotor_SOURCES-$(HAVE_LIBINPUT)+=input.c
glmotor_GENERATED-$(HAVE_WAYLAND_EGL)+=xdg-shell-protocol.c
glmotor_LIBS+=glsurface
glmotor_LIBS-$(EGL)+=dl
glmotor_LIBS-$(GLUT)+=glut
glmotor_LIBRARY-y+=libconfig
glmotor_LIBRARY-$(HAVE_LIBINPUT)+=libinput
glmotor_LIBRARY-$(HAVE_LIBINPUT)+=libudev
glmotor_LIBRARY-$(GLFW)+=glfw3
glmotor_LIBRARY-$(EGL)+=egl
glmotor_LIBRARY-$(EGL)+=wayland-egl
glmotor_LIBRARY-$(EGL)+=xkbcommon
glmotor_LIBRARY-$(EGL)+=x11
glmotor_LIBRARY-$(EGL)+=libdrm
glmotor_LIBRARY-$(EGL)+=gbm
glmotor_LIBRARY-$(EGL)+=libjpeg
glmotor_LIBRARY-$(GLEW)+=glew
glmotor_LIBRARY-$(GLESV2)+=glesv2
#glmotor_LIBRARY-$(GLESV2:n=y)+=gl
glmotor_LIBS+=m
glmotor_LIBRARY+=glu

PKG_CONFIG?=pkg-config
WAYLAND_FLAGS = $(shell $(PKG_CONFIG) wayland-client --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) wayland-protocols --variable=pkgdatadir)
WAYLAND_SCANNER = $(shell $(PKG_CONFIG) --variable=wayland_scanner wayland-scanner)
XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml

ifneq ($(wildcard $(XDG_SHELL_PROTOCOL)),)
$(shell sed -i '/HAVE_XDG_SHELL.*/d' $(CONFIG))
$(shell echo "HAVE_XDG_SHELL=y" >> $(CONFIG))
endif

$(objdir)xdg-shell-client-protocol.h: $(objdir)
	$(WAYLAND_SCANNER) client-header $(XDG_SHELL_PROTOCOL) $@

$(objdir)xdg-shell-protocol.c: $(objdir)xdg-shell-client-protocol.h
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) $@
