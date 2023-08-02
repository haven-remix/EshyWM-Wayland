
#include "server.h"
#include "eshywm.h"
#include "window.h"
#include "util.h"

#include <wayland-client.h>

#include <linux/input-event-codes.h>

static void server_new_keyboard(struct wlr_input_device* device);
static void server_new_input(struct wl_listener* listener, void* data);
static void server_cursor_motion(struct wl_listener* listener, void* data);
static void server_cursor_motion_absolute(struct wl_listener* listener, void* data);
static void server_cursor_button(struct wl_listener* listener, void* data);
static void server_cursor_axis(struct wl_listener* listener, void* data);
static void server_cursor_frame(struct wl_listener* listener, void* data);
static void server_new_output(struct wl_listener* listener, void* data);
static void server_new_xdg_surface(struct wl_listener* listener, void* data);

static void process_cursor_motion(uint32_t time);

static void seat_request_cursor(struct wl_listener* listener, void* data);
static void seat_request_set_selection(struct wl_listener* listener, void* data);

void eshywm_server::initialize()
{
    wl_display = wl_display_create();
	backend = wlr_backend_autocreate(wl_display, NULL);
    check(backend, "Failed to create wlr_backend");
	renderer = wlr_renderer_autocreate(backend);
    check(renderer, "Failed to create wlr_renderer");
	wlr_renderer_init_wl_display(renderer, wl_display);
	allocator = wlr_allocator_autocreate(backend, renderer);
    check(allocator, "Failed to create wlr_allocator");

	wlr_compositor_create(wl_display, 5, renderer);
	wlr_subcompositor_create(wl_display);
	wlr_data_device_manager_create(wl_display);

	output_layout = wlr_output_layout_create();
	add_listener(&new_output, server_new_output, &backend->events.new_output);

	scene = wlr_scene_create();
	wlr_scene_attach_output_layout(scene, output_layout);

	xdg_shell = wlr_xdg_shell_create(wl_display, 3);
	add_listener(&new_xdg_surface, server_new_xdg_surface, &xdg_shell->events.new_surface);

	cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(cursor, output_layout);

	cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

	cursor_mode = ESHYWM_CURSOR_PASSTHROUGH;
	add_listener(&cursor_motion, server_cursor_motion, &cursor->events.motion);
	add_listener(&cursor_motion_absolute, server_cursor_motion_absolute, &cursor->events.motion_absolute);
	add_listener(&cursor_button, server_cursor_button, &cursor->events.button);
	add_listener(&cursor_axis, server_cursor_axis, &cursor->events.axis);
	add_listener(&cursor_frame, server_cursor_frame, &cursor->events.frame);

	add_listener(&new_input, server_new_input, &backend->events.new_input);
	seat = wlr_seat_create(wl_display, "seat0");
	add_listener(&request_cursor, seat_request_cursor, &seat->events.request_set_cursor);
	add_listener(&request_set_selection, seat_request_set_selection, &seat->events.request_set_selection);
}

void eshywm_server::run_display(char* startup_cmd)
{
    /*Add a Unix socket to the Wayland display.*/
	const char* socket = wl_display_add_socket_auto(wl_display);
	if (!socket)
	{
		wlr_backend_destroy(backend);
		return;
	}

	if (!wlr_backend_start(backend))
	{
		wlr_backend_destroy(backend);
		wl_display_destroy(wl_display);
		return;
	}

	setenv("WAYLAND_DISPLAY", socket, true);
	if (startup_cmd && fork() == 0)
		execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void* )NULL);
	
	wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);
	wl_display_run(wl_display);
}

void eshywm_server::shutdown()
{
    wl_display_destroy_clients(wl_display);
	wlr_scene_node_destroy(&scene->tree.node);
	wlr_xcursor_manager_destroy(cursor_mgr);
	wlr_output_layout_destroy(output_layout);
	wl_display_destroy(wl_display);
}


void eshywm_server::close_window(eshywm_window* window)
{
	wlr_xdg_toplevel_send_close(window->xdg_toplevel);
}


void eshywm_server::reset_cursor_mode()
{
	cursor_mode = ESHYWM_CURSOR_PASSTHROUGH;
}


void server_new_keyboard(struct wlr_input_device* device)
{
	struct wlr_keyboard* wlr_keyboard = wlr_keyboard_from_input_device(device);

	eshywm_keyboard* keyboard = new eshywm_keyboard(wlr_keyboard);

	/*We need to prepare an XKB keymap and assign it to the keyboard. This
	*  assumes the defaults (e.g. layout = "us").*/
	struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap* keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(wlr_keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

	/*Here we set up listeners for keyboard events.*/
	add_listener(&keyboard->modifiers, keyboard_handle_modifiers, &wlr_keyboard->events.modifiers);
	add_listener(&keyboard->key, keyboard_handle_key, &wlr_keyboard->events.key);
	add_listener(&keyboard->destroy, keyboard_handle_destroy, &device->events.destroy);

	wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);

	/*And add the keyboard to our list of keyboards*/
	server->keyboard_list.push_back(keyboard);
}

void server_new_pointer(struct wlr_input_device* device)
{
	/*We don't do anything special with pointers. All of our pointer handling
	*  is proxied through wlr_cursor. On another compositor, you might take this
	*  opportunity to do libinput configuration on the device to set
	*  acceleration, etc.*/
	wlr_cursor_attach_input_device(server->cursor, device);
}

void server_new_input(struct wl_listener* listener, void* data)
{
	/*This event is raised by the backend when a new input device becomes
	*  available.*/
	struct wlr_input_device* device = (wlr_input_device*)data;
	switch (device->type)
	{
	case WLR_INPUT_DEVICE_KEYBOARD:
		server_new_keyboard(device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
		server_new_pointer(device);
		break;
	default:
		break;
	}
	/*We need to let the wlr_seat know what our capabilities are, which is
	*  communiciated to the client. In eshywm we always have a cursor, even if
	*  there are no pointer devices, so we always include that capability.*/
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (server->keyboard_list.size() > 0)
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;

	wlr_seat_set_capabilities(server->seat, caps);
}


void server_cursor_motion(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_motion_event* event = (wlr_pointer_motion_event*)data;
	wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
	process_cursor_motion(event->time_msec);
}

void server_cursor_motion_absolute(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_motion_absolute_event* event = (wlr_pointer_motion_absolute_event*)data;
	wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);
	process_cursor_motion(event->time_msec);
}

void server_cursor_button(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_button_event* event = (wlr_pointer_button_event*)data;

	wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
	double sx;
	double sy;
	struct wlr_surface* surface = NULL;
	eshywm_window* window = desktop_window_at(server->cursor->x, server->cursor->y, &surface, &sx, &sy);

	if (event->state == WLR_BUTTON_RELEASED)
		server->reset_cursor_mode();
	else
		window->focus_window(surface);

	if(server->b_window_modifier_key_pressed && event->state == WLR_BUTTON_PRESSED)
	{
		if (event->button == BTN_LEFT)
		{
			window->begin_interactive(ESHYWM_CURSOR_MOVE, 0);
			return;
		}
		else if (event->button == BTN_RIGHT)
		{
			window->begin_interactive(ESHYWM_CURSOR_RESIZE, 0);
			return;
		}
	}
}

void server_cursor_axis(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_axis_event* event = (wlr_pointer_axis_event*)data;
	wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source);
}

void server_cursor_frame(struct wl_listener* listener, void* data)
{
	wlr_seat_pointer_notify_frame(server->seat);
}

void server_new_output(struct wl_listener* listener, void* data)
{
	/*This event is raised by the backend when a new output (aka a display or
	*  monitor) becomes available.*/
	struct wlr_output* wlr_output = (struct wlr_output*)data;

	/*Configures the output created by the backend to use our allocator
	*  and our renderer. Must be done once, before commiting the output*/
	wlr_output_init_render(wlr_output, server->allocator, server->renderer);

	/*The output may be disabled, switch it on.*/
	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, true);

	/*Some backends don't have modes. DRM+KMS does, and we need to set a mode
	*  before we can use the output. The mode is a tuple of (width, height,
	*  refresh rate), and each monitor supports only a specific set of modes. We
	*  just pick the monitor's preferred mode, a more sophisticated compositor
	*  would let the user configure it.*/
	struct wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output);
	if (mode != NULL)
	{
		wlr_output_state_set_mode(&state, mode);
	}

	/*Atomically applies the new output state.*/
	wlr_output_commit_state(wlr_output, &state);
	wlr_output_state_finish(&state);

	/*Allocates and configures our state for this output*/
	eshywm_output* output = new eshywm_output;
	output->wlr_output = wlr_output;

	add_listener(&output->frame, output_frame, &wlr_output->events.frame);
	add_listener(&output->request_state, output_request_state, &wlr_output->events.request_state);
	add_listener(&output->destroy, output_destroy, &wlr_output->events.destroy);

	server->output_list.push_back(output);

	/*Adds this to the output layout. The add_auto function arranges outputs
	*  from left-to-right in the order they appear. A more sophisticated
	*  compositor would let the user configure the arrangement of outputs in the
	*  layout.
	* 
	*  The output layout utility automatically adds a wl_output global to the
	*  display, which Wayland clients can see to find out information about the
	*  output (such as DPI, scale factor, manufacturer, etc).
	*/
	wlr_output_layout_add_auto(server->output_layout, wlr_output);
}

void server_new_xdg_surface(struct wl_listener* listener, void* data)
{
	struct wlr_xdg_surface* xdg_surface = (wlr_xdg_surface*)data;

	if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP)
	{
		struct wlr_xdg_surface* parent = wlr_xdg_surface_try_from_wlr_surface(xdg_surface->popup->parent);
		assert(parent != NULL);
		struct wlr_scene_tree* parent_tree = (wlr_scene_tree*)parent->data;
		xdg_surface->data = wlr_scene_xdg_surface_create(parent_tree, xdg_surface);
	}
	else
	{
		eshywm_window* window = new eshywm_window;
		window->initialize(xdg_surface);
	}
}


void process_cursor_motion(uint32_t time)
{
	/*If the mode is non-passthrough, delegate to those functions.*/
	if (server->cursor_mode == ESHYWM_CURSOR_MOVE)
	{
		server->focused_window->process_cursor_move(time);
		return;
	}
	else if (server->cursor_mode == ESHYWM_CURSOR_RESIZE)
	{
		server->focused_window->process_cursor_resize(time);
		return;
	}

	/*Otherwise, find the window under the pointer and send the event along.*/
	double sx, sy;
	struct wlr_seat* seat = server->seat;
	struct wlr_surface* surface = NULL;
	eshywm_window* window = desktop_window_at(server->cursor->x, server->cursor->y, &surface, &sx, &sy);

	if (!window)
	{
		/*If there's no window under the cursor, set the cursor image to a
		*  default. This is what makes the cursor image appear when you move it
		*  around the screen, not over any windows.*/
		wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
	}

	if (surface)
	{
		/*
		*  Send pointer enter and motion events.
		* 
		*  The enter event gives the surface "pointer focus", which is distinct
		*  from keyboard focus. You get pointer focus by moving the pointer over
		*  a window.
		* 
		*  Note that wlroots will avoid sending duplicate enter/motion events if
		*  the surface has already has pointer focus or if the client is already
		*  aware of the coordinates passed.
		*/
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	}
	else
	{
		/*Clear pointer focus so future button events and such are not sent to
		*  the last client to have the cursor over it.*/
		wlr_seat_pointer_clear_focus(seat);
	}
}

void seat_request_cursor(struct wl_listener* listener, void* data)
{
	class eshywm_server* server = wl_container_of(listener, server, request_cursor);
	/*This event is raised by the seat when a client provides a cursor image*/
	struct wlr_seat_pointer_request_set_cursor_event* event = (wlr_seat_pointer_request_set_cursor_event*)data;
	struct wlr_seat_client* focused_client = server->seat->pointer_state.focused_client;
	/*This can be sent by any client, so we check to make sure this one is
	*  actually has pointer focus first.*/
	if (focused_client == event->seat_client)
	{
		/*Once we've vetted the client, we can tell the cursor to use the
		*  provided surface as the cursor image. It will set the hardware cursor
		*  on the output that it's currently on and continue to do so as the
		*  cursor moves between outputs.*/
		wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
	}
}

void seat_request_set_selection(struct wl_listener* listener, void* data)
{
	/*This event is raised by the seat when a client wants to set the selection,
	*  usually when the user copies something. wlroots allows compositors to
	*  ignore such requests if they so choose, but in eshywm we always honor
	*/
	class eshywm_server* server = wl_container_of(listener, server, request_set_selection);
	struct wlr_seat_request_set_selection_event* event = (wlr_seat_request_set_selection_event*)data;
	wlr_seat_set_selection(server->seat, event->source, event->serial);
}