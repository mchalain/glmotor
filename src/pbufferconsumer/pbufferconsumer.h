#ifndef __PBUFFERCONSUMER_H__
#define __PBUFFERCONSUMER_H__

#ifdef __cplusplus
extern "C" {
#endif
typedef struct pbufferconsumer_metadata_s pbufferconsumer_metadata_t;
struct pbufferconsumer_metadata_s
{
	EGLint target;
	EGLint type;
};

#ifndef __cplusplus
typedef struct GLMotor_s GLMotor_t;
typedef struct consumer_dmabuf_s consumer_dmabuf_t;
struct consumer_dmabuf_s
{
	GLMotor_t *motor;
	void *private;
};

int _dmabuf_init(consumer_dmabuf_t *dmabuf, uint32_t width, uint32_t height);
int _dmabuf_send(consumer_dmabuf_t *dmabuf, int dma_buf, uint32_t fourcc, EGLint stride, EGLint offset);
void _dmabuf_uninit(consumer_dmabuf_t *dmabuf);
#endif

#ifdef __cplusplus
}
#endif

#endif
