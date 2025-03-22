#include <stdlib.h>

#include <pthread.h>

#include <EGL/egl.h>

#include "opencv2/core/utility.hpp"
#include "opencv2/core/opengl.hpp"
#include <opencv2/highgui.hpp>
#include <iostream>

#include "glmotor.h"
#include "pbufferconsumer/pbufferconsumer.h"

typedef struct consumer_ocv_s consumer_ocv_t;
struct consumer_ocv_s
{
	uint32_t width;
	uint32_t height;
	pthread_t viewthread;
	pthread_cond_t viewcond;
	pthread_mutex_t viewmutex;
	cv::Mat image;
	int run;
};

static void * _module_viewthread(void *data);

extern "C" void *pbufferconsumer_create(GLMotor_t *motor, uint32_t width, uint32_t height)
{
//dbg("%s %d", __FILE__, __LINE__);
	consumer_ocv_t *ctx = (consumer_ocv_t *)calloc(1, sizeof(*ctx));
	ctx->width = width;
	ctx->height = height;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutex_init(&ctx->viewmutex, &mutexattr);

	pthread_cond_init(&ctx->viewcond, NULL);

	ctx->run = 1;
	pthread_create(&ctx->viewthread, NULL, _module_viewthread, ctx);
	sched_yield();

	return ctx;
}

static void * _module_viewthread(void *data)
{
	consumer_ocv_t *ctx = (consumer_ocv_t *)data;
	while (ctx->run)
	{
		pthread_mutex_lock(&ctx->viewmutex);
		while (ctx->image.empty())
			pthread_cond_wait(&ctx->viewcond, &ctx->viewmutex);
		pthread_mutex_unlock(&ctx->viewmutex);
		cv::imshow("view", ctx->image);
		cv::waitKey(1);
		free(ctx->image.data);
		ctx->image.release();
	}
	return NULL;
}

extern "C" void pbufferconsumer_queue(void * arg, GLuint client, pbufferconsumer_metadata_t metadata)
{
	consumer_ocv_t *ctx = (consumer_ocv_t *)arg;
	pthread_mutex_lock(&ctx->viewmutex);
	try {
#if 0
		cv::ogl::Texture2D::Format format = cv::ogl::Texture2D::RGBA;
		cv::ogl::Texture2D texture = cv::ogl::Texture2D(ctx->width, ctx->height, format, client);
		cv::ogl::convertFromGLTexture2D(texture, ctx->image);
#else
		unsigned char *input = (unsigned char *)malloc(ctx->width * ctx->height * 4);
		glReadPixels(0, 0, ctx->width, ctx->height, GL_RGBA, GL_UNSIGNED_BYTE, input);
		ctx->image = cv::Mat(ctx->height, ctx->width, CV_8UC4, input);
#endif
	}
	catch( cv::Exception exp) {
		std::cerr << "OpenCV exception " << exp.msg << "\n";
	}
	pthread_mutex_unlock(&ctx->viewmutex);
	pthread_cond_broadcast(&ctx->viewcond);
}

extern "C" void pbufferconsumer_destroy(void * arg)
{
	consumer_ocv_t *ctx = (consumer_ocv_t *)arg;
	ctx->run = 0;
	pthread_cond_broadcast(&ctx->viewcond);
	pthread_join(ctx->viewthread, NULL);
	free(arg);
}
