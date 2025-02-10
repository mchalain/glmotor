# glmotor
OpenGL samples

This application contains different window managements (glfw, glut, egl-wayland).

## build
Set the configuration first. Choice between GLEW (OpenGL) and GLESV2 (OpenGL ES).

The configuration tool accept standard GNU options:
 - prefix, sysconfdir, bindir...
 - CC, CXX, LD, CFLAGS...
 - SYSROOT, CROSS_PREFIX...

### GLEW version
This version is not recommanded.

It may use GLUT or GLFW as window manager.

```shell
$ make GLEW=y GLESV2=n GLUT=y GLFW=n defconfig
$ make
```

### EGL version
This is recommanded version

It may support several window manager:
 - X11
 - WAYLAND
 - DRM (direct rendering on screen without windowing)
 - JPEG (rendering image into JPEG file)

```shell
$ make GLEW=n GLESV2=y GLFW=n EGL=y GLUT=n defconfig
$ make
```

## run
The application is shared with several external files to play with the application.

The application option may depends on the window system:
 - w for the window's width
 - h for the window's height
 - n for the window's name
 - N for the EGL native support (x, wl, jpeg, drm)

### Generate a cube

```shell
$ ./src/glmotor -o data/cube.obj -v data/simple.vert -f data/texture.frag
```

### Generate camera image

```shell
$ ./src/glmotor -o data/camerascreen.obj -v data/simple.vert -f data/texture_ext.frag
```
