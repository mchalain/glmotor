#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <math.h>

#include <libinput.h>
#ifdef HAVE_XKBCOMMON
#include <xkbcommon/xkbcommon.h>
#endif

#include "glmotor.h"
#include "input.h"
#include "log.h"

#define INPUT_MOVECAMERA 0x0001
#define INPUT_ROTATEOBJECT 0x0002

struct GLMotor_Input_s
{
	GLMotor_Scene_t *scene;
	struct libinput *li;
	struct udev *udev;
	pthread_t thread;
	GLfloat l;
	GLfloat camera[3];
#ifdef HAVE_XKBCOMMON
	struct xkb_context *xkb_context;
	struct xkb_keymap *keymap;
	struct xkb_state *xkb_state;
#endif
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
	int mode = 0;

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
#ifdef HAVE_XKBCOMMON
			if (type == LIBINPUT_EVENT_KEYBOARD_KEY)
			{
				struct libinput_event_keyboard *keyboard_event;
				keyboard_event = libinput_event_get_keyboard_event(event);
				uint32_t key = libinput_event_keyboard_get_key(keyboard_event);
				int state = libinput_event_keyboard_get_key_state(keyboard_event);
				if (state == LIBINPUT_KEY_STATE_PRESSED)
				{
					//xkb_state_update_key(input->xkb_state, key+8, state);
					xkb_keysym_t keysym = xkb_state_key_get_one_sym(input->xkb_state, key+8);
					if (keysym == XKB_KEY_space)
					{
						obj = scene_getobject(input->scene, "object");
						scene_reset(input->scene);
						object_movereset(obj);
						obj = NULL;
					}
				}
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
				if (button == 0x111) /// Button_rIGHT
				{
					enum libinput_button_state state;
					state = libinput_event_pointer_get_button_state(pointer_event);
					if (state == LIBINPUT_BUTTON_STATE_PRESSED)
					{
						mode |= INPUT_ROTATEOBJECT;
						obj = scene_getobject(input->scene, "object");
					}
					if (state == LIBINPUT_BUTTON_STATE_RELEASED)
					{
						mode &= ~INPUT_ROTATEOBJECT;
						obj = NULL;
					}
				}
			}
			if (type == LIBINPUT_EVENT_POINTER_SCROLL_CONTINUOUS ||
				type == LIBINPUT_EVENT_POINTER_SCROLL_WHEEL ||
				type == LIBINPUT_EVENT_POINTER_SCROLL_FINGER)
			{
				struct libinput_event_pointer *pointer_event;
				pointer_event = libinput_event_get_pointer_event(event);
				if (! libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
				{
					continue;
				}
				double dz = libinput_event_pointer_get_scroll_value(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
				if (obj && (mode & INPUT_ROTATEOBJECT))
				{
					GLMotor_Rotate_t rot = {0};
					rot.ra.A = dz;
					rot.ra.Z = 1.0;
					object_appendkinematic(obj, NULL, &rot, 0);
				}
				else if (obj)
				{
					GLMotor_Translate_t tr = {0};
					tr.coord.L = input->l;
					tr.coord.Z = (GLfloat) dz;
					object_appendkinematic(obj, &tr, NULL, 0);
				}
				else
				{
					GLfloat zoom = 0.001f;
					if (dz < 0)
						zoom *= -1.0;
					scene_zoom(input->scene, zoom);
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
				if (obj && (mode & INPUT_ROTATEOBJECT))
				{
					GLMotor_Rotate_t rot = {0};
					rot.ra.A = 0.1;
					if (dx > dy)
					{
						rot.ra.Y = 1.0;
						rot.ra.A *= dx;
					}
					else
					{
						rot.ra.X = 1.0;
						rot.ra.A *= dy;
					}
					object_appendkinematic(obj, NULL, &rot, 0);
				}
				else if (obj)
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
	if (input->li == NULL)
	{
		err("glmotor: input not available");
		return NULL;
	}
	libinput_udev_assign_seat(input->li, "seat0");
	pthread_create(&input->thread, NULL, _input_thread, input);
	/**
	 * as the mapping on a OpenGL space is a cube  (-1.0 / 1.0) * 3
	 * an d the deplacement is in pixels' screen.
	 * It is impossible to have the good correction
	 */
	input->l = sqrt((scene_width(scene) * scene_width(scene)) + (scene_height(scene) * scene_height(scene)));
	input->l = 1 / input->l;
#ifdef HAVE_XKBCOMMON
	input->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	input->keymap = xkb_keymap_new_from_names(input->xkb_context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
	input->xkb_state = xkb_state_new(input->keymap);
	return input;
#endif
}

GLMOTOR_EXPORT void input_destroy(GLMotor_Input_t *input)
{
	libinput_unref(input->li);
	udev_unref(input->udev);
	free(input);
}
