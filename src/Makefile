subdir-$(EGL)+=pbufferconsumer.mk

ifneq ($(findstring $(HAVE_GBM) $(HAVE_WAYLAND_EGL)$(HAVE_X11),y),)
NEED_NATIVEJPEG=y
endif
slib-y+=glsurface
glsurface_SOURCES-$(GLUT)+=surfaceglut.c
glsurface_LIBS-$(GLUT)+=glut
glsurface_SOURCES-$(HAVE_GLFW3)+=surfaceglfw.c
glsurface_SOURCES-$(HAVE_EGL)+=surfaceegl.c
glsurface_SOURCES-$(EGL:y=$(HAVE_LIBJPEG))+=eglnativejpeg.c
glsurface_SOURCES-$(EGL:y=$(HAVE_WAYLAND_EGL))+=eglnativewl.c
glsurface_SOURCES-$(EGL:y=$(HAVE_X11))+=eglnativex.c
glsurface_SOURCES-$(EGL)+=eglnativepbuffer.c
glsurface_SOURCES-$(EGL:y=$(HAVE_GBM:y=$(HAVE_LIBDRM)))+=eglnativedrm.c
glsurface_LIBRARY+=wayland-egl
glsurface_LIBRARY+=libdrm
glsurface_LIBRARY+=gbm
glsurface_LIBRARY+=libjpeg
glsurface_LIBRARY+=x11

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
glmotor_LIBS+=m
glmotor_LIBS-$(HAVE_EGL)+=dl
glmotor_LIBS-$(GLUT)+=glut
glmotor_LIBRARY+=libconfig
glmotor_LIBRARY+=libinput
glmotor_LIBRARY+=libudev
glmotor_LIBRARY-$(GLFW)+=glfw3
glmotor_LIBRARY-$(EGL)+=egl
glmotor_LIBRARY-$(GLEW)+=glew
glmotor_LIBRARY-$(GLESV2)+=glesv2
#glmotor_LIBRARY-$(GLESV2:n=y)+=gl
glmotor_LIBRARY+=xkbcommon
glmotor_LIBRARY+=wayland-egl
glmotor_LIBRARY+=libdrm
glmotor_LIBRARY+=gbm
glmotor_LIBRARY+=libjpeg
glmotor_LIBRARY+=x11

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
