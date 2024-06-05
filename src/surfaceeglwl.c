#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <xkbcommon/xkbcommon.h>

#define GLMOTOR_SURFACE_S struct GLMotor_Surface_s
#include "glmotor.h"
#include "log.h"

static struct wl_display *display;
static struct wl_compositor *compositor = NULL;
static struct wl_shell *shell = NULL;
static struct xdg_wm_base *xdg_wm_base = NULL;
static EGLDisplay egl_display;
static struct wl_seat *seat = NULL;
static struct xkb_context *xkb_context;
static struct xkb_keymap *keymap = NULL;
static struct xkb_state *xkb_state = NULL;
static struct xdg_toplevel *xdg_toplevel = NULL;
static char running = 1;

struct GLMotor_Surface_s
{
	EGLContext egl_context;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct wl_shell_surface *shell_surface;
	struct wl_egl_window *egl_window;
	EGLSurface egl_surface;
};

// listeners
static void keyboard_keymap (void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
	char *keymap_string = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	xkb_keymap_unref (keymap);
	keymap = xkb_keymap_new_from_string (xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
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
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		xkb_keysym_t keysym = xkb_state_key_get_one_sym (xkb_state, key+8);
		uint32_t utf32 = xkb_keysym_to_utf32 (keysym);
		if (utf32) {
			if (utf32 >= 0x21 && utf32 <= 0x7E) {
				printf ("the key %c was pressed\n", (char)utf32);
				if (utf32 == 'q') running = 0;
			}
			else {
				printf ("the key U+%04X was pressed\n", utf32);
			}
		}
		else {
			char name[64];
			xkb_keysym_get_name (keysym, name, 64);
			printf ("the key %s was pressed\n", name);
		}
	}
}
static void keyboard_modifiers (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
	xkb_state_update_mask (xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}
static struct wl_keyboard_listener keyboard_listener = {&keyboard_keymap, &keyboard_enter, &keyboard_leave, &keyboard_key, &keyboard_modifiers};

static void seat_capabilities (void *data, struct wl_seat *seat, uint32_t capabilities) {
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		struct wl_keyboard *keyboard = wl_seat_get_keyboard (seat);
		wl_keyboard_add_listener (keyboard, &keyboard_listener, NULL);
	}
}
static struct wl_seat_listener seat_listener = {&seat_capabilities};

static void xdg_wm_base_handle_ping(void *data,
		struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	// The compositor will send us a ping event to check that we're responsive.
	// We need to send back a pong request immediately.
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_handle_ping,
};

static void registry_add_object (void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
	warn("interface %s", interface);
	if (!strcmp(interface, wl_compositor_interface.name)) {
		warn("Compositor");
		compositor = wl_registry_bind (registry, name, &wl_compositor_interface, 1);
	}
	else if (!strcmp(interface, wl_shell_interface.name)) {
		warn("shell");
		shell = wl_registry_bind (registry, name, &wl_shell_interface, 1);
	}
	else if (!strcmp(interface, wl_seat_interface.name)) {
		warn("seat");
		seat = wl_registry_bind (registry, name, &wl_seat_interface, 1);
		wl_seat_add_listener (seat, &seat_listener, NULL);
	}
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
	}
}
static void registry_remove_object (void *data, struct wl_registry *registry, uint32_t name) {
	
}
static struct wl_registry_listener registry_listener = {&registry_add_object, &registry_remove_object};

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

static void xdg_toplevel_handle_close(void *data,
		struct xdg_toplevel *xdg_toplevel) {
	// Stop running if the user requests to close the toplevel
	running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = noop,
	.close = xdg_toplevel_handle_close,
};

/**
 * wl_shell
 */
static void shell_surface_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial) {
	wl_shell_surface_pong (shell_surface, serial);
}
static void shell_surface_configure (void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {
	struct GLMotor_Surface_s *window = data;
	wl_egl_window_resize (window->egl_window, width, height, 0, 0);
}
static void shell_surface_popup_done (void *data, struct wl_shell_surface *shell_surface) {
	
}
static struct wl_shell_surface_listener shell_surface_listener = {&shell_surface_ping, &shell_surface_configure, &shell_surface_popup_done};

GLMOTOR_EXPORT GLMotor_t *glmotor_create(int argc, char** argv)
{
	GLuint width = 640;
	GLuint height = 480;
	GLchar *name = "GLMotor";

	/** environment management */
	display = wl_display_connect(NULL);
	if (display == NULL)
	{
		err("glmotor: wayland connection error %m");
		return NULL;
	}
	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);
	
	egl_display = eglGetDisplay(display);
	eglInitialize (egl_display, NULL, NULL);

	/** window creation */
	eglBindAPI(EGL_OPENGL_API);
	EGLint attributes[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
	EGL_NONE};
	EGLConfig config;
	EGLint num_config;
	eglChooseConfig(egl_display, attributes, &config, 1, &num_config);

	GLMotor_Surface_t *window = calloc(1, sizeof(*window));

	window->egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, NULL);

	window->surface =
		wl_compositor_create_surface(compositor);
	if (window->surface == NULL)
	{
		err("glmotor: surface creation error %m");
		return NULL;
	}
	
	if (shell != NULL)
	{
		window->shell_surface =
			wl_shell_get_shell_surface(shell, window->surface);
		wl_shell_surface_add_listener(window->shell_surface, &shell_surface_listener, window);
		wl_shell_surface_set_toplevel(window->shell_surface);
	}
	if (xdg_wm_base != NULL)
	{
		struct xdg_surface *xdg_surface =
			xdg_wm_base_get_xdg_surface(xdg_wm_base, window->surface);
		xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);

		xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
		wl_surface_commit(window->surface);
		xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);
	}

	window->egl_window =
		wl_egl_window_create(window->surface, width, height);
	window->egl_surface =
		eglCreateWindowSurface(egl_display, config, window->egl_window, NULL);

	GLMotor_t *motor = calloc(1, sizeof(*motor));
	motor->width = width;
	motor->height = height;
	motor->surf = window;

	eglMakeCurrent(egl_display, window->egl_surface, window->egl_surface, window->egl_context);

	return motor;
}

GLMOTOR_EXPORT GLuint glmotor_run(GLMotor_t *motor, GLMotor_Draw_func_t draw, void *drawdata)
{
	while (running)
	{
		wl_display_dispatch_pending (display);
		draw(drawdata);
		eglSwapBuffers(egl_display, motor->surf->egl_surface);
	}
	return 0;
}

GLMOTOR_EXPORT void glmotor_destroy(GLMotor_t *motor)
{
	GLMotor_Surface_t *window = motor->surf;

	eglDestroySurface(egl_display, window->egl_surface);
	wl_egl_window_destroy(window->egl_window);
	wl_shell_surface_destroy(window->shell_surface);
	wl_surface_destroy(window->surface);
	eglDestroyContext(egl_display, window->egl_context);

	free(window);
	free(motor);

	eglTerminate(egl_display);
	wl_display_disconnect(display);
}
