#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include <linux/videodev2.h>

#ifdef HAVE_GLESV2
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
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
	int max_texture_size = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	width = (width >  max_texture_size)? max_texture_size:width;
	height = (height >  max_texture_size)? max_texture_size:height;

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
	case FOURCC('R','G','2','4'):
		format = GL_RGB;
		break;
	default:
		dbg("format %.4s", (char*)&fourcc);
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
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, map);

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

#define MAX_CAMERA_BUFFERS 5

PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = NULL;
PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = NULL;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = NULL;

extern GLMOTOR_EXPORT EGLDisplay* glmotor_egldisplay(GLMotor_t *motor);

typedef struct GLMotor_TextureCameraBuffer_s GLMotor_TextureCameraBuffer_t;
struct GLMotor_TextureCameraBuffer_s
{
	GLuint ID;
	EGLImageKHR image;
};

typedef struct GLMotor_TextureCamera_s GLMotor_TextureCamera_t;
struct GLMotor_TextureCamera_s
{
	int fd;
	int nbuffers;
	int bufid;
	GLMotor_TextureCameraBuffer_t *textures;
	GLenum family;
};

static GLint texturecamera_draw(GLMotor_Texture_t *tex);
static void texturecamera_destroy(GLMotor_Texture_t *tex);

GLMOTOR_EXPORT GLMotor_Texture_t *texture_fromcamera(GLMotor_t *motor, const char *device, GLuint width, GLuint height, uint32_t fourcc)
{
	int nbuffers = MAX_CAMERA_BUFFERS;

	int max_texture_size = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	width = (width >  max_texture_size)? max_texture_size:width;
	height = (height >  max_texture_size)? max_texture_size:height;

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
		if (ffmt.pixelformat == fourcc)
			support_fmt = 1;
		ffmt.index++;
	}
	if (!support_fmt)
		err("glmotor: v4l2 %.4s format not supported", (char*)&fourcc);
	struct v4l2_frmsizeenum fsize = {0};
	fsize.index = 0;
	fsize.pixel_format = fourcc;
	int support_size = 0;
	while (!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize))
	{
		if (fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
		{
			if (width > (fsize.stepwise.max_width))
				width = fsize.stepwise.max_width;
			if (width < (fsize.stepwise.min_width))
				width = fsize.stepwise.min_width;
			if (height > (fsize.stepwise.max_height))
				height = fsize.stepwise.max_height;
			if (height < (fsize.stepwise.min_height))
				height = fsize.stepwise.min_height;
			support_size = 1;
		}
		else
		{
			dbg("frame size %dx%d", fsize.discrete.width, fsize.discrete.height);
			if (fsize.discrete.width == width && fsize.discrete.height == height)
				support_size = 1;
		}
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
	if (input.index > 0)
	{
		int index;
		if (ioctl(fd, VIDIOC_G_INPUT, &index))
			return NULL;
		dbg("current input index=%d", index);
	}
	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_G_FMT, &fmt))
	{
		err("glmotor: get v4l2 fmt error %m");
		return NULL;
	}
	dbg("fmt width=%d height=%d pfmt=%.4s",
		   fmt.fmt.pix.width, fmt.fmt.pix.height, (char*)&fmt.fmt.pix.pixelformat);

	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = fourcc;
	if (ioctl(fd, VIDIOC_S_FMT, &fmt))
	{
		err("glmotor: set v4l2 fmt error %m");
		return NULL;
	}
	/// verify fmt
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_G_FMT, &fmt))
	{
		err("glmotor: get v4l2 fmt error %m");
		return NULL;
	}
	if (fmt.fmt.pix.width != width)
	{
		err("glmotor: bad width %u", fmt.fmt.pix.width);
		return NULL;
	}
	if (fmt.fmt.pix.height != height)
	{
		err("glmotor: bad height %u", fmt.fmt.pix.height);
		return NULL;
	}
	if (fmt.fmt.pix.pixelformat != fourcc)
	{
		err("glmotor: bad pixelformat %.4s", (char*)&fmt.fmt.pix.pixelformat);
		return NULL;
	}
	GLuint stride = fmt.fmt.pix.bytesperline;

	struct v4l2_requestbuffers reqbuf;
	reqbuf.count = nbuffers;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf))
	{
		err("glmotor: request v4l2 buffers error %m");
		return NULL;
	}
	if (reqbuf.count != nbuffers)
	{
		err("glmotor: not enough buffers %u", reqbuf.count);
		return NULL;
	}

	EGLint *attrib_list;
	EGLint attrib_list_yuv12[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_LINUX_DRM_FOURCC_EXT, FOURCC('Y','U','1','2'),
		EGL_DMA_BUF_PLANE0_FD_EXT, 0,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, stride,
		EGL_DMA_BUF_PLANE1_FD_EXT, 0,
		EGL_DMA_BUF_PLANE1_OFFSET_EXT, stride * height,
		EGL_DMA_BUF_PLANE1_PITCH_EXT, stride / 2,
		EGL_DMA_BUF_PLANE2_FD_EXT, 0,
		EGL_DMA_BUF_PLANE2_OFFSET_EXT, (stride * height) * 5 / 4 ,
		EGL_DMA_BUF_PLANE2_PITCH_EXT, stride / 2,
		EGL_NONE
	};
	EGLint attrib_list_rgb[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_LINUX_DRM_FOURCC_EXT, FOURCC('A','B','2','4'),
		EGL_DMA_BUF_PLANE0_FD_EXT, 0,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, stride,
		EGL_NONE
	};
	switch (fourcc)
	{
	case FOURCC('Y','U','1','2'):
		attrib_list = attrib_list_yuv12;
		attrib_list[5] = fourcc;
	break;
	case FOURCC('Y','U','Y','V'):
		attrib_list = attrib_list_rgb;
		attrib_list[11] = width * sizeof(uint32_t);
	break;
	default:
		attrib_list = attrib_list_rgb;
		attrib_list[5] = fourcc;
	break;
	}
	GLenum texturefamily = GL_TEXTURE_2D;
	if (strstr(glGetString(GL_EXTENSIONS), "OES_EGL_image_external"))
	{
		glEnable(GL_TEXTURE_EXTERNAL_OES);
		texturefamily = GL_TEXTURE_EXTERNAL_OES;
	}
	GLMotor_TextureCameraBuffer_t *textures = calloc(nbuffers, sizeof(GLuint));
	for (int i = 0; i < nbuffers; i++)
	{
		glGenTextures(nbuffers, &textures[i].ID);
		struct v4l2_exportbuffer expbuf = {0};
		expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		expbuf.index = i;
		expbuf.flags = O_CLOEXEC | O_RDWR;
		if (ioctl(fd, VIDIOC_EXPBUF, &expbuf))
		{
			dbg("%s %d", __FILE__, __LINE__);
			nbuffers = i;
			break;
		}

		attrib_list[7] = expbuf.fd;
		if (fourcc == FOURCC('Y','U','1','2'))
		{
			attrib_list[13] = expbuf.fd;
			attrib_list[19] = expbuf.fd;
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glBindTexture(texturefamily, textures[i].ID);
		textures[i].image = eglCreateImageKHR(glmotor_egldisplay(motor),
			EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attrib_list);
		if (textures[i].image == EGL_NO_IMAGE_KHR)
		{
			nbuffers = i;
			break;
		}

		glTexParameteri(texturefamily, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(texturefamily, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glEGLImageTargetTexture2DOES(texturefamily, textures[i].image);
		eglDestroyImageKHR(glmotor_egldisplay(motor), textures[i].image);

		struct v4l2_buffer buf = {0};
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.index = i;
		buf.memory = V4L2_MEMORY_MMAP;
		if (ioctl(fd, VIDIOC_QBUF, &buf))
		{
			nbuffers = i;
			break;
		}
		dbg("glmotor: buffer %d queued dmafd %d", i, expbuf.fd);
	}
	if (nbuffers == 0)
	{
		err("glmotor: impossible to allocate buffers %m");
		return NULL;
	}
	GLMotor_TextureCamera_t *camera;
	camera = calloc(1, sizeof(*camera));
	camera->fd = fd;
	camera->nbuffers = nbuffers;
	camera->textures = textures;
	camera->family = texturefamily;

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
	dbg("glmotor: camera running");
	camera->bufid = -1;
	return tex;
}

static GLint texturecamera_draw(GLMotor_Texture_t *tex)
{
	GLMotor_TextureCamera_t *camera = tex->private;
	struct v4l2_buffer buf = {0};
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	// unbind texture may lag the video
	//glBindTexture(camera->family, 0);
	if (camera->bufid > -1)
	{
		buf.index = camera->bufid;
		if (ioctl(camera->fd, VIDIOC_QBUF, &buf))
			return -1;
	}
	if (ioctl(camera->fd, VIDIOC_DQBUF, &buf))
		return -1;

	glBindTexture(camera->family, camera->textures[buf.index].ID);
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
