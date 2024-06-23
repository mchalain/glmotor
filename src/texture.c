#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include <linux/videodev2.h>

#ifndef HAVE_GLEW
# include <GLES2/gl2.h>
#else
# include <GL/glew.h>
#endif
#ifdef HAVE_GLU
#include <GL/glu.h>
#endif

#include "glmotor.h"
#include "log.h"

/***********************************************************************
 * GLMotor_Texture_t
 **********************************************************************/

struct GLMotor_Texture_s
{
	GLMotor_t *motor;
	GLuint ID;
	GLuint texture;
	GLuint npoints;
	GLuint nfaces;
	GLuint width;
	GLuint height;
	uint32_t fourcc;
	void *private;
	GLint (*draw)(GLMotor_Texture_t *tex);
	void (*destroy)(GLMotor_Texture_t *tex);
};

GLMOTOR_EXPORT GLMotor_Texture_t *texture_create(GLMotor_t *motor, GLuint width, GLuint height, uint32_t fourcc, GLuint mipmaps, GLchar *map)
{
	GLuint texture  = glGetUniformLocation(motor->programID, "vTexture");
	if (texture == -1)
	{
		err("glmotor: try to apply texture but shader doesn't contain 'vTexture'");
		return NULL;
	}
	dbg("glmotor: create texture");
	GLMotor_Texture_t *tex = NULL;

	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);	

	unsigned int components  = (fourcc == FOURCC('D','X','T','1')) ? 3 : 4;
	unsigned int format = 0;
	switch(fourcc)
	{
	case FOURCC('D','X','T','1'):
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case FOURCC('D','X','T','3'):
		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case FOURCC('D','X','T','5'):
		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;
	case FOURCC('A','B','2','4'):
		format = GL_RGBA;
		break;
	default:
		dbg("format %.4s", &fourcc);
	}
	if (format >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT && format <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
		unsigned int offset = 0;

		/* load the mipmaps */
		for (unsigned int level = 0; level < mipmaps && (width || height); ++level)
		{
			unsigned int size = ((width+3)/4)*((height+3)/4)*blockSize;
			glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,
				0, size, map + offset);

			offset += size;
			width  /= 2;
			height /= 2;

			// Deal with Non-Power-Of-Two textures. This code is not included in the webpage to reduce clutter
			if(width < 1) width = 1;
			if(height < 1) height = 1;
		} 
	}
	else
	{
		// Give the image to OpenGL
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, map);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		if (mipmaps)
			glGenerateMipmap(GL_TEXTURE_2D);
	}

	tex = calloc(1, sizeof(*tex));
	tex->ID = textureID;
	tex->texture = texture;
	tex->width = width;
	tex->height = height;
	tex->fourcc = fourcc;
	return tex;
}

GLMOTOR_EXPORT GLint texture_draw(GLMotor_Texture_t *tex)
{
	int ret = 0;
	glActiveTexture(GL_TEXTURE0);
	if (tex->private && tex->draw)
		ret = tex->draw(tex);
	else
	{
		glBindTexture(GL_TEXTURE_2D, tex->ID);
	}
	glUniform1i(tex->texture, 0);
	return ret;
}

GLMOTOR_EXPORT void texture_destroy(GLMotor_Texture_t *tex)
{
	if (tex->private && tex->destroy)
		tex->destroy(tex);
	else
	{
		free(tex);
	}
}

#ifdef HAVE_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>

PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = NULL;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = NULL;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = NULL;

extern GLMOTOR_EXPORT EGLDisplay* glmotor_egldisplay(GLMotor_t *motor);

typedef struct GLMotor_TextureCamera_s GLMotor_TextureCamera_t;
struct GLMotor_TextureCamera_s
{
	int fd;
	int nbuffers;
	int bufid;
	GLuint *textures;
};

static GLint texturecamera_draw(GLMotor_Texture_t *tex);
static void texturecamera_destroy(GLMotor_Texture_t *tex);

GLMOTOR_EXPORT GLMotor_Texture_t *texture_fromcamera(GLMotor_t *motor, const char *device, GLuint width, GLuint height, uint32_t fourcc)
{
	GLuint texture  = glGetUniformLocation(motor->programID, "vTexture");
	if (texture == -1)
	{
		err("glmotor: try to apply texture but shader doesn't contain 'vTexture'");
		return NULL;
	}

	if (! eglCreateImageKHR)
		eglCreateImageKHR = (void *) eglGetProcAddress("eglCreateImageKHR");
	if (! eglDestroyImageKHR)
		eglDestroyImageKHR = (void *) eglGetProcAddress("eglDestroyImageKHR");
	if (! glEGLImageTargetTexture2DOES)
		glEGLImageTargetTexture2DOES = (void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");

	if (fourcc == 0)
		fourcc = FOURCC('A','B','2','4');

	int fd = open(device, O_RDWR);
	if (fd < 0)
		return NULL;
	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap))
		return NULL;
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
		return NULL;
	if (!(cap.capabilities & V4L2_CAP_STREAMING))
		return NULL;
	struct v4l2_fmtdesc ffmt = {0};
	ffmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int support_fmt = 0;
	while (!ioctl(fd, VIDIOC_ENUM_FMT, &ffmt))
	{
		dbg("fmt %d %s", ffmt.pixelformat, ffmt.description);
		if (ffmt.pixelformat == V4L2_PIX_FMT_YUYV)
			support_fmt = 1;
		ffmt.index++;
	}
	struct v4l2_frmsizeenum fsize = {0};
	fsize.index = 0;
	fsize.pixel_format = V4L2_PIX_FMT_YUYV;
	int support_size = 0;
	while (!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize))
	{
		dbg("frame size %dx%d", fsize.discrete.width, fsize.discrete.height);
		if (fsize.discrete.width == width && fsize.discrete.height == height)
			support_size = 1;
		fsize.index++;
	}

	struct v4l2_input input = {0};
	input.index = 0;
	while (!ioctl(fd, VIDIOC_ENUMINPUT, &input))
	{
		dbg("input name=%s type=%d status=%d std=%lld",
			   input.name, input.type, input.status, input.std);
		input.index++;
	}
	int index;
	if (ioctl(fd, VIDIOC_G_INPUT, &index))
		return NULL;
	dbg("current input index=%d", index);

	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_G_FMT, &fmt))
		return NULL;
	dbg("fmt width=%d height=%d pfmt=%.4s",
		   fmt.fmt.pix.width, fmt.fmt.pix.height, &fmt.fmt.pix.pixelformat);

	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	if (ioctl(fd, VIDIOC_S_FMT, &fmt))
		return NULL;
	/// verify fmt
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_G_FMT, &fmt))
		return NULL;
	if (fmt.fmt.pix.width != width)
		return NULL;
	if (fmt.fmt.pix.height != height)
		return NULL;
	if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
		return NULL;

	int nbuffers = 5;

	struct v4l2_requestbuffers reqbuf;
	reqbuf.count = nbuffers;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf))
		return NULL;
	if (reqbuf.count != nbuffers)
		return NULL;

	GLuint *textures = calloc(nbuffers, sizeof(GLuint));
	glGenTextures(nbuffers, textures);
	for (int i = 0; i < nbuffers; i++)
	{
		struct v4l2_exportbuffer expbuf = {0};
		expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		expbuf.index = i;
		expbuf.flags = O_RDWR;
		if (ioctl(fd, VIDIOC_EXPBUF, &expbuf))
		{
			nbuffers = i;
			break;
		}

		EGLint attrib_list[] = {
			EGL_WIDTH, width,
			EGL_HEIGHT, height,
			EGL_LINUX_DRM_FOURCC_EXT, FOURCC('A','B','2','4'),
			EGL_DMA_BUF_PLANE0_FD_EXT, expbuf.fd,
			EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
			EGL_DMA_BUF_PLANE0_PITCH_EXT, width * sizeof(uint32_t),
			EGL_NONE
		};
		EGLImageKHR image = eglCreateImageKHR(glmotor_egldisplay(motor),
			EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attrib_list);
		if (image == EGL_NO_IMAGE_KHR)
		{
			nbuffers = i;
			break;
		}

		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
		glBindTexture(GL_TEXTURE_2D, 0);

		struct v4l2_buffer buf = {0};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.index = i;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(fd, VIDIOC_QBUF, &buf))
		{
			nbuffers = i;
			break;
		}
	}
	GLMotor_TextureCamera_t *camera;
	camera = calloc(1, sizeof(*camera));
	camera->fd = fd;
	camera->nbuffers = nbuffers;
	camera->textures = textures;

	GLMotor_Texture_t *tex;
	tex = calloc(1, sizeof(*tex));
	tex->ID = 0x12345678;
	tex->texture = texture;
	tex->width = width;
	tex->height = height;
	tex->fourcc = fourcc;
	tex->private = camera;
	tex->draw = texturecamera_draw;
	tex->destroy = texturecamera_destroy;

	// start camera
	int a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMON, &a))
	{
		free(camera);
		free(tex);
		close(fd);
		return NULL;
	}
#if 0
	struct v4l2_buffer buf = {0};
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(camera->fd, VIDIOC_DQBUF, &buf))
		return NULL;
	glBindTexture(GL_TEXTURE_2D, camera->textures[buf.index]);
	camera->bufid = buf.index;
#else
	camera->bufid = -1;
#endif
	return tex;
}

static GLint texturecamera_draw(GLMotor_Texture_t *tex)
{
	GLMotor_TextureCamera_t *camera = tex->private;
	struct v4l2_buffer buf = {0};

	glBindTexture(GL_TEXTURE_2D, 0);
	if (camera->bufid > -1)
	{
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.index = camera->bufid;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(camera->fd, VIDIOC_QBUF, &buf))
			return -1;
	}
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(camera->fd, VIDIOC_DQBUF, &buf))
		return -1;

	glBindTexture(GL_TEXTURE_2D, camera->textures[buf.index]);
	camera->bufid = buf.index;
	return 0;
}

static void texturecamera_destroy(GLMotor_Texture_t *tex)
{
	GLMotor_TextureCamera_t *camera = tex->private;;
	int a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(camera->fd, VIDIOC_STREAMOFF, &a);
	close(camera->fd);
	free(camera);
	free(tex);
}
#endif
