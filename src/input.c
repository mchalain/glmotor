#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <math.h>

#include <libinput.h>

#include "glmotor.h"
#include "input.h"
#include "log.h"

struct GLMotor_Input_s
{
	GLMotor_Scene_t *scene;
	struct libinput *li;
	struct udev *udev;
	pthread_t thread;
	GLfloat l;
};

static int open_restricted(const char *path, int flags, void *user_data)
{
	int fd = open(path, flags);
	return fd;
}

static void close_restricted(int fd, void *user_data)
{
	close(fd);
}

const static struct libinput_interface _interface = {
	.open_restricted = open_restricted,
	.close_restricted = close_restricted,
};

static void *_input_thread(void *arg)
{
	GLMotor_Input_t *input = (GLMotor_Input_t *)arg;
	struct libinput_event *event;
	GLMotor_Object_t *obj = NULL;

	while (1)
	{
		struct pollfd fd = {libinput_get_fd(input->li), POLLIN, 0};
		poll (&fd, 1, -1);
		libinput_dispatch(input->li);
		while ((event = libinput_get_event(input->li)) != NULL)
		{
			int type = libinput_event_get_type(event);
#ifdef DEBUG
			if (type == LIBINPUT_EVENT_DEVICE_ADDED)
			{
				struct libinput_device *dev = libinput_event_get_device(event);
				warn("glmotor: new input device %s", libinput_device_get_name(dev));
				if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_POINTER))
					warn("glmotor: pointer device");
			}
#endif
			if (type == LIBINPUT_EVENT_POINTER_BUTTON)
			{
				struct libinput_event_pointer *pointer_event;
				pointer_event = libinput_event_get_pointer_event(event);
				uint32_t button = libinput_event_pointer_get_button(pointer_event);
				if (button == 0x110) /// Button_Left
				{
					enum libinput_button_state state;
					state = libinput_event_pointer_get_button_state(pointer_event);
					if (obj == NULL && state == LIBINPUT_BUTTON_STATE_PRESSED)
					{
						obj = scene_getobject(input->scene, "object");
					}
					if (state == LIBINPUT_BUTTON_STATE_RELEASED)
						obj = NULL;
				}
			}
			if (type == LIBINPUT_EVENT_POINTER_MOTION)
			{
				struct libinput_event_pointer *pointer_event;
				pointer_event = libinput_event_get_pointer_event(event);
#if 0
				double dx = libinput_event_pointer_get_dx(pointer_event);
				double dy = libinput_event_pointer_get_dy(pointer_event);
#else
				double dx = libinput_event_pointer_get_dx_unaccelerated(pointer_event);
				double dy = libinput_event_pointer_get_dy_unaccelerated(pointer_event);
#endif
				if (obj)
				{
					GLMotor_Translate_t tr = {0};
					tr.coord.L = input->l;
					tr.coord.X = (GLfloat) dx;
					tr.coord.Y = (GLfloat) -dy;
					object_appendkinematic(obj, &tr, NULL, 0);
				}
			}
			libinput_event_destroy(event);
			libinput_dispatch(input->li);
		}
	}
	return NULL;
}

GLMOTOR_EXPORT GLMotor_Input_t *input_create(GLMotor_Scene_t *scene)
{
	GLMotor_Input_t *input = calloc(1, sizeof(*input));
	input->scene = scene;
	input->udev = udev_new ();
	input->li = libinput_udev_create_context(&_interface, NULL, input->udev);
	libinput_udev_assign_seat(input->li, "seat0");
	pthread_create(&input->thread, NULL, _input_thread, input);
	/**
	 * as the mapping on a OpenGL space is a cube  (-1.0 / 1.0) * 3
	 * an d the deplacement is in pixels' screen.
	 * It is impossible to have the good correction
	 */
	input->l = sqrt((scene_width(scene) * scene_width(scene)) + (scene_height(scene) * scene_height(scene)));
	input->l = 1 / input->l;
	return input;
}

GLMOTOR_EXPORT void input_destroy(GLMotor_Input_t *input)
{
	libinput_unref(input->li);
	udev_unref(input->udev);
	free(input);
}
