#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#ifdef HAVE_XKBCOMMON
#include <xkbcommon/xkbcommon.h>
#endif
#ifdef HAVE_XDG_SHELL
#include "xdg-shell-client-protocol.h"
#endif

#include "glmotor.h"
#include "log.h"
#include "eglnative.h"

static struct wl_display *display;
static struct wl_compositor *compositor = NULL;
static struct wl_shell *shell = NULL;
static struct wl_seat *seat = NULL;
#ifdef HAVE_XKBCOMMON
static struct xkb_keymap *keymap = NULL;
static struct xkb_state *xkb_state = NULL;
#endif
#ifdef HAVE_XDG_SHELL
static struct xdg_wm_base *xdg_wm_base = NULL;
static struct xdg_toplevel *xdg_toplevel = NULL;
#endif
static char running = 1;

static struct GLMotor_EGLWL_s
{
	GLuint width;
	GLuint height;
	EGLContext egl_context;
	struct wl_surface *surface;
#ifdef HAVE_XDG_SHELL
	struct xdg_surface *xdg_surface;
#endif
#ifdef HAVE_XKBCOMMON
	struct xkb_context *xkb_context;
#endif
	struct wl_shell_surface *shell_surface;
	struct wl_egl_window *egl_window;
	EGLSurface egl_surface;
	GLMotor_t *motor;
} *window = NULL;

#ifdef HAVE_XKBCOMMON
static char key_name[64];

// listeners
static void keyboard_keymap (void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
	struct GLMotor_EGLWL_s *window = data;
	char *keymap_string = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (keymap_string == NULL)
		return;
	xkb_keymap_unref (keymap);
	if (window->xkb_context == NULL)
		return;
	keymap = xkb_keymap_new_from_string(window->xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap (keymap_string, size);
	close (fd);
	xkb_state_unref (xkb_state);
	xkb_state = xkb_state_new (keymap);
}
static void keyboard_enter (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {

}
static void keyboard_leave (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {

}
static void keyboard_key (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
	struct GLMotor_EGLWL_s *window = data;
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		xkb_keysym_t keysym = xkb_state_key_get_one_sym (xkb_state, key+8);
		uint32_t utf32 = xkb_keysym_to_utf32 (keysym);
		xkb_keysym_get_name (keysym, key_name, 64);
		if (utf32) {
			if (utf32 >= 0x1b)
				running = 0;
			if (utf32 >= 0x21 && utf32 <= 0x7E) {
				dbg("the key %c was pressed", (char)utf32);
				if (utf32 == 'q')
					running = 0;
				else
				{
				}
			}
			else {
				dbg("the key U+%04X was pressed", utf32);
			}
		}
		else {
			int keycode = 0;
			switch (keysym)
			{
				case XKB_KEY_Left:
					keycode = 0x72;
				break;
				case XKB_KEY_Right:
					keycode = 0x71;
				break;
				case XKB_KEY_Up:
					keycode = 0x6f;
				break;
				case XKB_KEY_Down:
					keycode = 0x74;
				break;
			}
			GLMotor_Event_t evt = {
				.type = EVT_KEY,
				.data = {
					.key = {
						.mode = 0,
						.code = keycode,
						.value = 0,
					},
				},
			};
			glmotor_newevent(window->motor, evt);
			dbg("the key %s was pressed %#x", key_name, keysym);
		}
	}
}
static void keyboard_modifiers (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	xkb_state_update_mask (xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}
static struct wl_keyboard_listener keyboard_listener = {
	&keyboard_keymap,
	&keyboard_enter,
	&keyboard_leave,
	&keyboard_key,
	&keyboard_modifiers
};

static void seat_capabilities (void *data, struct wl_seat *seat, uint32_t capabilities) {
	struct GLMotor_EGLWL_s *window = data;
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		window->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
		struct wl_keyboard *keyboard = wl_seat_get_keyboard (seat);
		wl_keyboard_add_listener (keyboard, &keyboard_listener, window);
	}
}
static struct wl_seat_listener seat_listener = {&seat_capabilities};
#endif

static void registry_add_object (void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
	//warn("interface %s", interface);
	if (!strcmp(interface, wl_compositor_interface.name)) {
		warn("Compositor");
		compositor = wl_registry_bind (registry, name, &wl_compositor_interface, 1);
	}
	else if (!strcmp(interface, wl_shell_interface.name)) {
		warn("shell");
		shell = wl_registry_bind (registry, name, &wl_shell_interface, 1);
	}
#ifdef HAVE_XKBCOMMON
	else if (!strcmp(interface, wl_seat_interface.name)) {
		warn("seat");
		seat = wl_registry_bind (registry, name, &wl_seat_interface, 1);
	}
#endif
#ifdef HAVE_XDG_SHELL
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		warn("xdg_wm_base");
		xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
#endif
}

static void registry_remove_object (void *data, struct wl_registry *registry, uint32_t name) {

}
static struct wl_registry_listener registry_listener = {&registry_add_object, &registry_remove_object};

#ifdef HAVE_XDG_SHELL
static void xdg_wm_base_handle_ping(void *data,
		struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	// The compositor will send us a ping event to check that we're responsive.
	// We need to send back a pong request immediately.
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_handle_ping,
};
/**
 * xdg_shell xdg_toplevel
 */
static void noop() {
	// This space intentionally left blank
}

static void xdg_surface_handle_configure(void *data,
		struct xdg_surface *xdg_surface, uint32_t serial) {
	// The compositor configures our surface, acknowledge the configure event
	xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_handle_configure,
};

static void xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *xdg_toplevel,
				int32_t width, int32_t height, struct wl_array *states)
{
	struct GLMotor_EGLWL_s *window = data;

	if(!width && !height)
		return;

	if(window->width != width || window->height != height)
	{
		window->width = width;
		window->height = height;

		wl_egl_window_resize(window->egl_window, width, height, 0, 0);
		wl_surface_commit(window->surface);
	}
}

static void xdg_toplevel_handle_close(void *data,
		struct xdg_toplevel *xdg_toplevel) {
	// Stop running if the user requests to close the toplevel
	running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_handle_configure,
	.close = xdg_toplevel_handle_close,
};
#endif

/**
 * wl_shell
 */
static void shell_surface_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
	wl_shell_surface_pong (shell_surface, serial);
}
static void shell_surface_configure (void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
	struct GLMotor_EGLWL_s *window = data;

	if(!width && !height)
		return;

	if(window->width != width || window->height != height)
	{
		window->width = width;
		window->height = height;

		wl_egl_window_resize (window->egl_window, width, height, 0, 0);
	}
}
static void shell_surface_popup_done (void *data, struct wl_shell_surface *shell_surface) {

}
static struct wl_shell_surface_listener shell_surface_listener = {&shell_surface_ping, &shell_surface_configure, &shell_surface_popup_done};

static EGLNativeWindowType native_createwindow(EGLNativeDisplayType dislay, GLuint width, GLuint height, const GLchar *name)
{
	/** environment management */
	if (display == NULL)
	{
		display = wl_display_connect(NULL);
	}
	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);

	window = calloc(1, sizeof(*window));
	window->width = width;
	window->height = height;
	window->surface =
		wl_compositor_create_surface(compositor);
	if (window->surface == NULL)
	{
		err("glmotor: surface creation error %m");
		return NULL;
	}
	if (seat)
		wl_seat_add_listener (seat, &seat_listener, window);

#ifdef HAVE_XDG_SHELL
	if (xdg_wm_base != NULL)
	{
		xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, window);
		window->xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, window->surface);
		xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener, window);

		xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
		wl_surface_commit(window->surface);
		xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, window);
	}
	else
#endif
	if (shell != NULL)
	{
		window->shell_surface =
			wl_shell_get_shell_surface(shell, window->surface);
		wl_shell_surface_add_listener(window->shell_surface, &shell_surface_listener, window);
		wl_shell_surface_set_toplevel(window->shell_surface);
	}

	window->egl_window =
		wl_egl_window_create(window->surface, width, height);
	return window->egl_window;
}

static EGLNativeDisplayType native_display()
{
	if (display == NULL)
		/** environment management */
		display = wl_display_connect(NULL);
	return (EGLNativeDisplayType)display;
}

static GLboolean native_running(EGLNativeWindowType native_win, GLMotor_t *motor)
{
	struct wl_egl_window *egl_window = (struct wl_egl_window *)native_win;
	window->motor = motor;
	wl_display_dispatch_pending(display);
	return running;
}

static void native_destroy(EGLNativeDisplayType native_display)
{
	struct wl_display *display = (struct wl_display *)native_display;
	wl_egl_window_destroy(window->egl_window);
	wl_shell_surface_destroy(window->shell_surface);
	wl_surface_destroy(window->surface);
	wl_display_disconnect(display);
}

struct eglnative_motor_s *eglnative_wl = &(struct eglnative_motor_s)
{
	.name = "wl",
	.display = native_display,
	.createwindow = native_createwindow,
	.running = native_running,
	.destroy = native_destroy,
};
