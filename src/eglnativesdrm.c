#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 

#include <GL/gl.h>
#include <EGL/egl.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <drm/drm_fourcc.h>
#ifdef HAVE_GBM
#include <gbm.h>
#endif

#include "log.h"
#include "eglnative.h"

static int running = 1;

#define MAX_BUFFERS 2

typedef struct DisplayFormat_s DisplayFormat_t;
struct DisplayFormat_s
{
	uint32_t	fourcc;
	int		depth;
} g_formats[] =
{
	{
		.fourcc = DRM_FORMAT_ARGB8888,
		.depth = 4,
	},
	{0}
};

typedef struct DisplayBuffer_s DisplayBuffer_t;
struct DisplayBuffer_s
{
#ifdef HAVE_LIBKMS
	struct kms_bo *bo;
#endif
	int bo_handle;
	int dma_fd;
	uint32_t fb_id;
	uint32_t pitch;
	uint32_t size;
};

typedef struct Display_s Display_t;
struct Display_s
{
#ifdef HAVE_LIBKMS
	struct kms_driver *kms;
#endif
	uint32_t connector_id;
	uint32_t encoder_id;
	uint32_t crtc_id;
	uint32_t plane_id;
	drmModeCrtc *crtc;
	DisplayFormat_t *format;
	int type;
	int fd;
	drmModeModeInfo mode;
	DisplayBuffer_t buffers[MAX_BUFFERS];
	int nbuffers;
	int buf_id;
};

static int sdrm_ids(Display_t *disp, uint32_t *conn_id, uint32_t *enc_id, uint32_t *crtc_id, drmModeModeInfo *mode)
{
	drmModeResPtr resources;
	resources = drmModeGetResources(disp->fd);

	uint32_t connector_id = 0;
	for(int i = 0; i < resources->count_connectors; ++i)
	{
		connector_id = resources->connectors[i];
		drmModeConnectorPtr connector = drmModeGetConnector(disp->fd, connector_id);
		if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
		{
			drmModeModeInfo *preferred = NULL;
			if (mode->hdisplay && mode->vdisplay)
				preferred = mode;
			*conn_id = connector_id;
			for (int m = 0; m < connector->count_modes; m++)
			{
				dbg("sdrm: mode: %s %dx%d %s",
						connector->modes[m].name,
						connector->modes[m].hdisplay,
						connector->modes[m].vdisplay,
						connector->modes[m].type & DRM_MODE_TYPE_PREFERRED ? "*" : "");
				if (!preferred && connector->modes[m].type & DRM_MODE_TYPE_PREFERRED)
				{
					preferred = &connector->modes[m];
				}
				if (preferred && connector->modes[m].hdisplay == preferred->hdisplay &&
								connector->modes[m].vdisplay == preferred->vdisplay)
				{
					preferred = &connector->modes[m];
				}
			}
			if (preferred == NULL || preferred == mode)
				preferred = &connector->modes[0];
			memcpy(mode, preferred, sizeof(*mode));
			*enc_id = connector->encoder_id;
			drmModeFreeConnector(connector);
			break;
		}
		drmModeFreeConnector(connector);
	}

	if (*conn_id == 0 || *enc_id == 0)
	{
		drmModeFreeResources(resources);
		return -1;
	}

	for(int i=0; i < resources->count_encoders; ++i)
	{
		drmModeEncoderPtr encoder;
		encoder = drmModeGetEncoder(disp->fd, resources->encoders[i]);
		if(encoder != NULL)
		{
			dbg("sdrm: encoder %d found", encoder->encoder_id);
			if(encoder->encoder_id == *enc_id)
			{
				*crtc_id = encoder->crtc_id;
				drmModeFreeEncoder(encoder);
				break;
			}
			drmModeFreeEncoder(encoder);
		}
		else
			err("sdrm: get a null encoder pointer");
	}

	int crtcindex = -1;
	for(int i=0; i < resources->count_crtcs; ++i)
	{
		if (resources->crtcs[i] == *crtc_id)
		{
			crtcindex = i;
			break;
		}
	}
	if (crtcindex == -1)
	{
		drmModeFreeResources(resources);
		err("sdrm: crtc mot available");
		return -1;
	}
	dbg("sdrm: screen size %ux%u", disp->mode.hdisplay, disp->mode.vdisplay);
	drmModeFreeResources(resources);
	return 0;
}

static uint64_t sdrm_properties(Display_t *disp, uint32_t plane_id, const char *property)
{
	uint64_t ret = 0;
#if 1
	drmModeObjectPropertiesPtr props;

	props = drmModeObjectGetProperties(disp->fd, plane_id, DRM_MODE_OBJECT_PLANE);
	for (int i = 0; i < props->count_props; i++)
	{
		drmModePropertyPtr prop;

		prop = drmModeGetProperty(disp->fd, props->props[i]);
		dbg("sdrm: property %s", prop->name);
		if (prop && !strcmp(prop->name, property))
		{
			ret = props->prop_values[i];
			break;
		}
		if (prop)
			drmModeFreeProperty(prop);
	}
	drmModeFreeObjectProperties(props);
#else
	struct drm_mode_obj_get_properties counter = {
		.obj_id = plane_id,
		.obj_type = DRM_MODE_OBJECT_PLANE,
	};
	if (drmIoctl(disp->fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &counter) == -1)
	{
		err("sdrm: Properties count error %m");
		return -1;
	}
    size_t count = counter.count_props;
    uint32_t *ids = calloc(count, sizeof (*ids));
    uint64_t *values = calloc(count, sizeof (*values));
	struct drm_mode_obj_get_properties props = {
        .props_ptr = (uintptr_t)(void *)ids,
        .prop_values_ptr = (uintptr_t)(void *)values,
        .count_props = count,
        .obj_id = plane_id,
        .obj_type = DRM_MODE_OBJECT_PLANE,
    };
	if (drmIoctl(disp->fd, DRM_IOCTL_MODE_OBJ_GETPROPERTIES, &props) == -1)
	{
		err("sdrm: Properties get error %m");
		return -1;
	}
    for (size_t i = 0; i < props.count_props; i++)
    {
		struct drm_mode_get_property prop = {
			.prop_id = ids[i],
		};
		if (drmIoctl(disp->fd, DRM_IOCTL_MODE_GETPROPERTY, &prop) == -1)
		{
			err("sdrm: Property %#x get error %m", ids[i]);
			continue;
		}
		if (!strcmp(property, prop.name))
		{
			ret = values[i];
			break;
		}
	}
#endif
	return ret;
}

static int sdrm_plane(Display_t *disp, uint32_t *plane_id)
{
	drmModePlaneResPtr planes;

	planes = drmModeGetPlaneResources(disp->fd);

	drmModePlanePtr plane;
	for (int i = 0; i < planes->count_planes; ++i)
	{
		int found = 0;
		plane = drmModeGetPlane(disp->fd, planes->planes[i]);
		int type = (int)sdrm_properties(disp, plane->plane_id, "type");
		if (type != disp->type)
			continue;
		dbg("sdrm: plane %s", (type == DRM_PLANE_TYPE_PRIMARY)?"primary":"overlay");
		for (int j = 0; j < plane->count_formats; ++j)
		{
			uint32_t fourcc = plane->formats[j];
			dbg("sdrm: Plane[%d] %u: 4cc %.4s", i, plane->plane_id, (char *)&fourcc);
			if (plane->formats[j] == disp->format->fourcc)
			{
				found = 1;
				break;
			}
		}
		*plane_id = plane->plane_id;
		drmModeFreePlane(plane);
		if (found)
			break;
	}
	drmModeFreePlaneResources(planes);
	return 0;
}

static int sdrm_buffer(Display_t *disp, uint32_t width, uint32_t height, uint32_t size, uint32_t fourcc, DisplayBuffer_t *buffer)
{
	buffer->size = size;
	buffer->pitch = size / height;

#ifdef HAVE_LIBKMS
	unsigned attr[] = {
		KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
		KMS_WIDTH, width,
		KMS_HEIGHT, height,
		KMS_TERMINATE_PROP_LIST
	};

	if (kms_bo_create(disp->kms, attr, &buffer->bo))
	{
		err("sdrm: kms bo error");
		return -1;
	}
	if (kms_bo_get_prop(buffer->bo, KMS_HANDLE, &buffer->bo_handle))
	{
		err("sdrm: kms bo handle error");
		return -1;
	}
	if (kms_bo_get_prop(buffer->bo, KMS_PITCH, &buffer->pitch))
	{
		err("sdrm: kms bo pitch error");
		return -1;
	}
	if (kms_bo_map(buffer->bo, &buffer->memory))
	{
		err("sdrm: kms bo map error");
		return -1;
	}
#else
	struct drm_mode_create_dumb gem = {
		.width = width,
		.height = height,
		.size = size,
		.bpp = 32,
	};
	if (drmIoctl(disp->fd, DRM_IOCTL_MODE_CREATE_DUMB, &gem) == -1)
	{
		err("sdrm: dumb allocation error %m");
		return -1;
	}

	buffer->bo_handle = gem.handle;

	struct drm_mode_map_dumb map = {
		.handle = gem.handle,
	};
	if (drmIoctl(disp->fd, DRM_IOCTL_MODE_MAP_DUMB, &map) == -1)
	{
		err("sdrm: dumb map error %m");
		return -1;
	}
#endif

	uint32_t offsets[4] = { 0 };
	uint32_t pitches[4] = { buffer->pitch };
	uint32_t bo_handles[4] = { buffer->bo_handle };

#if 0
	struct drm_prime_handle prime = {0};
	prime.handle = buffer->bo_handle;

	if (ioctl(disp->fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime))
	{
		err("sdrm: dmabuf not allowed %m");
	}
	buffer->dma_fd = prime.fd;
#else
	if (drmPrimeHandleToFD(disp->fd, buffer->bo_handle, DRM_CLOEXEC, &buffer->dma_fd))
	{
		err("sdrm: dmabuf not allowed %m");
	}
#endif
	
	if (drmModeAddFB2(disp->fd, width, height, disp->format->fourcc, bo_handles,
		pitches, offsets, &buffer->fb_id, 0) == -1)
	{
		err("sdrm: Frame buffer unavailable %m");
		return -1;
	}
	dbg("sdrm: buffer for width %u height %u size %u buffer %u dma %u", width, height, size, buffer->fb_id, buffer->dma_fd);
	return 0;
}

static void sdrm_freebuffer(Display_t *disp, DisplayBuffer_t *buffer)
{
	drmModeRmFB(disp->fd, buffer->fb_id);
#ifdef HAVE_LIBKMS
	kms_bo_unmap(buffer->bo);
	kms_bo_destroy(buffer->bo);
#else
	struct drm_mode_destroy_dumb dumb = {
		.handle = buffer->bo_handle,
	};
	drmIoctl(disp->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dumb);
#endif
}

Display_t *sdrm_create(const char *device, uint32_t fourcc, uint32_t width, uint32_t height)
{
	int fd = 0;
	if (!access(device, R_OK | W_OK))
		fd = open(device, O_RDWR);
	else
		fd = drmOpen(device, NULL);
	if (fd < 0)
	{
		err("glmotor: device (%s) bad argument %m", device);
		return NULL;
	}
	if (drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1))
		err("glmotor: Universal plane not supported %m");

	Display_t *disp = calloc(1, sizeof(*disp));
	disp->fd = fd;
	disp->format = &g_formats[0];
	disp->type = DRM_PLANE_TYPE_PRIMARY;

	disp->mode.hdisplay = width;
	disp->mode.vdisplay = height;
	if (sdrm_ids(disp, &disp->connector_id, &disp->encoder_id, &disp->crtc_id, &disp->mode) == -1)
	{
		free(disp);
		return NULL;
	}
	if (sdrm_plane(disp, &disp->plane_id) == -1)
	{
		free(disp);
		return NULL;
	}
#ifdef HAVE_LIBKMS
	if (kms_create(fd, &disp->kms))
		err("sdrm: kms create erro");
#endif
#if 0
	size_t size = disp->mode.hdisplay * disp->mode.vdisplay * disp->format->depth;
	for (int i = 0; i < MAX_BUFFERS; i++, disp->nbuffers ++)
	{
		if (sdrm_buffer(disp,  disp->mode.hdisplay, disp->mode.vdisplay, size, disp->format->fourcc, &disp->buffers[i]) == -1)
		{
			err("sdrm: buffer %d allocation error", i);
			break;
		}
	}
	disp->crtc = drmModeGetCrtc(disp->fd, disp->crtc_id);
	if (drmModeSetCrtc(disp->fd, disp->crtc_id, disp->buffers[0].fb_id, 0, 0, &disp->connector_id, 1, &disp->mode))
	{
		err("srdm: Crtc setting error %m");
		for (int j = 0; j < MAX_BUFFERS; j++)
			sdrm_freebuffer(disp, &disp->buffers[j]);
		free(disp);
		return NULL;
	}
	drmModePageFlip(disp->fd, disp->crtc_id, disp->buffers[0].fb_id, DRM_MODE_PAGE_FLIP_EVENT, disp);
#endif
	return disp;
}

void *sdrm_getdma(Display_t *disp, int *fd, size_t *size)
{
	disp->buf_id++;
	disp->buf_id %= disp->nbuffers;
	if (disp->buffers[disp->buf_id].dma_fd == 0)
	{
		size_t size = disp->mode.hdisplay * disp->mode.vdisplay * disp->format->depth;
		if (sdrm_buffer(disp,  disp->mode.hdisplay, disp->mode.vdisplay, size, disp->format->fourcc, &disp->buffers[disp->buf_id]) == -1)
		{
			err("sdrm: buffer %d allocation error", disp->buf_id);
			return NULL;
		}
		if (disp->crtc == NULL)
		{
			disp->crtc = drmModeGetCrtc(disp->fd, disp->crtc_id);
			if (drmModeSetCrtc(disp->fd, disp->crtc_id, disp->buffers[disp->buf_id].fb_id, 0, 0, &disp->connector_id, 1, &disp->mode))
			{
				err("srdm: Crtc setting error %m");
				return NULL;
			}
		}
	}
	*fd = disp->buffers[disp->buf_id].dma_fd;
	*size = disp->buffers[disp->buf_id].size;
	return (void*)disp->buf_id;
}

int sdrm_flushbuf(Display_t *disp, void* id)
{
	drmModePageFlip(disp->fd, disp->crtc_id, disp->buffers[(int)id].fb_id, DRM_MODE_PAGE_FLIP_EVENT, disp);
	return 0;
}

void sdrm_destroy(Display_t *disp)
{
	drmModeFreeCrtc(disp->crtc);
	for (int j = 0; j < MAX_BUFFERS; j++)
		sdrm_freebuffer(disp, &disp->buffers[j]);
	close(disp->fd);
	free(disp);
}

Display_t *g_disp = NULL;

static EGLNativeDisplayType native_display()
{
	GLuint width = 640;
	GLuint height = 480;

	g_disp = sdrm_create("/dev/dri/card0", DRM_FORMAT_ARGB8888, width, height);

#ifdef HAVE_GBM
	struct gbm_device *dev;
	dev = gbm_create_device(g_disp->fd);
	if (!dev)
		return NULL;
#endif

	return (EGLNativeDisplayType)dev;
}

static GLboolean native_running(EGLNativeWindowType native_win)
{
#ifdef HAVE_GBM
	static struct gbm_bo *old_bo = NULL;
	struct gbm_surface *surface = (struct gbm_surface *)native_win;

	struct gbm_bo *bo;
	bo = gbm_surface_lock_front_buffer(surface);

	void * dma_id = gbm_bo_get_user_data(bo);
	if (dma_id == 0)
	{
		int fd;
		size_t size;
		dma_id = sdrm_getdma(g_disp, &fd, &size);
		gbm_bo_set_user_data(bo, dma_id + 1, NULL);
	}
	dma_id--;
	sdrm_flushbuf(g_disp, dma_id);
	if (old_bo)
		gbm_surface_release_buffer(surface, old_bo);
	old_bo = bo;
#else
	void * dma_id = 0;
	sdrm_flushbuf(g_disp, dma_id);
#endif

	return running;
}

static EGLNativeWindowType native_createwindow(EGLNativeDisplayType display, GLuint width, GLuint height, const GLchar *name)
{
	struct gbm_device *dev = (struct gbm_device *)display;

	struct gbm_surface *surface = NULL;
	surface = gbm_surface_create(dev, width, height, GBM_FORMAT_XRGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!surface)
		err("glmotor: DRM surface error %m");

	return (EGLNativeWindowType) surface;
}

static void native_destroy(EGLNativeDisplayType native_display)
{
}

struct eglnative_motor_s *eglnative_drm = &(struct eglnative_motor_s)
{
	.name = "drm",
	.display = native_display,
	.createwindow = native_createwindow,
	.running = native_running,
	.destroy = native_destroy,
};
