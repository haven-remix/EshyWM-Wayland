
#include "eshywm.h"
#include "server.h"
#include "util.h"

eshywm_server* server = nullptr;

eshywm_window* desktop_window_at(double lx, double ly, struct wlr_surface* *surface, double* sx, double* sy)
{
	/*This returns the topmost node in the scene at the given layout coords.
	*  we only care about surface nodes as we are specifically looking for a
	*  surface in the surface tree of a eshywm_window.*/
	struct wlr_scene_node* node = wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
	if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
		return NULL;

	struct wlr_scene_buffer* scene_buffer = wlr_scene_buffer_from_node(node);
	struct wlr_scene_surface* scene_surface = wlr_scene_surface_try_from_buffer(scene_buffer);

	if (!scene_surface)
		return NULL;

	*surface = scene_surface->surface;
	/*Find the node corresponding to the eshywm_window at the root of this
	*  surface tree, it is the only one for which we set the data field.*/
	struct wlr_scene_tree* tree = node->parent;
	while (tree != NULL && tree->node.data == NULL)
		tree = tree->node.parent;

	return (eshywm_window*)tree->node.data;
}


void output_frame(struct wl_listener* listener, void* data)
{
	/*This function is called every time an output is ready to display a frame,
	*  generally at the output's refresh rate (e.g. 60Hz).*/
	class eshywm_output* output = wl_container_of(listener, output, frame);
	struct wlr_scene* scene = server->scene;

	struct wlr_scene_output* scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);

	/*Render the scene if needed and commit the output*/
	wlr_scene_output_commit(scene_output);

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now);
}

void output_request_state(struct wl_listener* listener, void* data)
{
	/*This function is called when the backend requests a new state for
	*  the output. For example, Wayland and X11 backends request a new mode
	*  when the output window is resized.*/
	class eshywm_output* output = wl_container_of(listener, output, request_state);
	const struct wlr_output_event_request_state* event = (wlr_output_event_request_state*)data;
	wlr_output_commit_state(output->wlr_output, event->state);
}

void output_destroy(struct wl_listener* listener, void* data)
{
	class eshywm_output* output = wl_container_of(listener, output, destroy);

	wl_list_remove(&output->frame.link);
	wl_list_remove(&output->request_state.link);
	wl_list_remove(&output->destroy.link);
	free(output);
}


void keyboard_handle_modifiers(struct wl_listener* listener, void* data)
{
	/*This event is raised when a modifier key, such as shift or alt, is
	*  pressed. We simply communicate this to the client.*/
	class eshywm_keyboard* keyboard = wl_container_of(listener, keyboard, modifiers);
	/*
	*  A seat can only have one keyboard, but this is a limitation of the
	*  Wayland protocol - not wlroots. We assign all connected keyboards to the
	*  same seat. You can swap out the underlying wlr_keyboard like this and
	*  wlr_seat handles this transparently.
	*/
	wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);
	/*Send modifiers to the client.*/
	wlr_seat_keyboard_notify_modifiers(server->seat, &keyboard->wlr_keyboard->modifiers);
}

bool handle_keybinding(xkb_keysym_t sym)
{
	/*
	*  Here we handle compositor keybindings. This is when the compositor is
	*  processing keys, rather than passing them on to the client for its own
	*  processing.
	* 
	*  This function assumes Alt is held down.
	*/
	switch (sym)
	{
	case XKB_KEY_Escape:
		wl_display_terminate(server->wl_display);
		break;
	case XKB_KEY_F1:
	{
		/*Cycle to the next window*/
		// if (wl_list_length(&server->windows) < 2)
		// {
		// 	break;
		// }
		// class eshywm_window* next_window = (eshywm_window*)wl_container_of(server->windows.prev, next_window, link);
		// focus_window(next_window, next_window->xdg_toplevel->base->surface);
		break;
	}
	default:
		return false;
	}
	return true;
}

void keyboard_handle_key(struct wl_listener* listener, void* data)
{
	/*This event is raised when a key is pressed or released.*/
	class eshywm_keyboard* keyboard = wl_container_of(listener, keyboard, key);
	struct wlr_keyboard_key_event* event = (wlr_keyboard_key_event* )data;
	struct wlr_seat* seat = server->seat;

	/*Translate libinput keycode -> xkbcommon*/
	uint32_t keycode = event->keycode + 8;
	/*Get a list of keysyms based on the keymap for this keyboard*/
	const xkb_keysym_t* syms;
	int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

	bool handled = false;
	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
	if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		/*If alt is held down and this button was _pressed_, we attempt to
		*  process it as a compositor keybinding.*/
		for (int i = 0; i < nsyms; i++)
		{
			handled = handle_keybinding(syms[i]);
		}
	}

	if (!handled)
	{
		/*Otherwise, we pass it along to the client.*/
		wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
		wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
	}
}

void keyboard_handle_destroy(struct wl_listener* listener, void* data)
{
	/*This event is raised by the keyboard base wlr_input_device to signal
	*  the destruction of the wlr_keyboard. It will no longer receive events
	*  and should be destroyed.
	*/
	class eshywm_keyboard* keyboard = wl_container_of(listener, keyboard, destroy);
	wl_list_remove(&keyboard->modifiers.link);
	wl_list_remove(&keyboard->key.link);
	wl_list_remove(&keyboard->destroy.link);
	free(keyboard);
}


int main(int argc, char* argv[])
{
	wlr_log_init(WLR_DEBUG, NULL);
	char* startup_cmd = NULL;

	int c;
	while ((c = getopt(argc, argv, "s:h")) != -1)
	{
		switch (c)
		{
		case 's':
			startup_cmd = optarg;
			break;
		default:
			printf("Usage: %s [-s startup command]\n", argv[0]);
			return 0;
		}
	}
	if (optind < argc)
	{
		printf("Usage: %s [-s startup command]\n", argv[0]);
		return 0;
	}

	server = new eshywm_server;
	server->initialize();
	server->run_display(startup_cmd);
	server->shutdown();
	return 0;
}
