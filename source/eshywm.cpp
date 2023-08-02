
#include "eshywm.h"
#include "server.h"
#include "window.h"
#include "util.h"

#include <wayland-client-core.h>

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
	const eshywm_keyboard* keyboard = wl_container_of(listener, keyboard, modifiers);

	wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);
	wlr_seat_keyboard_notify_modifiers(server->seat, &keyboard->wlr_keyboard->modifiers);

	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);

	//Window modifier
	server->b_window_modifier_key_pressed = modifiers & WLR_MODIFIER_LOGO ? server->b_window_modifier_key_pressed : false;

	if(!server->b_window_modifier_key_pressed)
		server->reset_cursor_mode();
}

bool handle_keybinding(xkb_keysym_t sym)
{
	//This function assumes super is held down
	switch (sym)
	{
	case XKB_KEY_Escape:
		wl_display_terminate(server->wl_display);
		break;
	case XKB_KEY_j:
	{
		if (server->focused_window)
			server->focused_window->toggle_maximize();
		break;
	}
	case XKB_KEY_k:
	{
		if (server->focused_window)
			server->focused_window->toggle_fullscreen();
		break;
	}
	case XKB_KEY_n:
	{
		if (server->focused_window)
			server->close_window(server->focused_window);
		break;
	}
	case XKB_KEY_r:
		system("wofi --show drun --fork --allow-images");
		break;
	default:
		return false;
	}
	return true;
}

void keyboard_handle_key(struct wl_listener* listener, void* data)
{
	const eshywm_keyboard* keyboard = wl_container_of(listener, keyboard, key);
	const struct wlr_keyboard_key_event* event = (wlr_keyboard_key_event* )data;
	struct wlr_seat* seat = server->seat;

	//Translate libinput keycode -> xkbcommon
	uint32_t keycode = event->keycode + 8;
	//Get a list of keysyms based on the keymap for this keyboard
	const xkb_keysym_t* syms;
	int nsyms = xkb_state_key_get_syms(keyboard->wlr_keyboard->xkb_state, keycode, &syms);

	bool handled = false;
	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
	if ((modifiers & WLR_MODIFIER_LOGO) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
		for (int i = 0; i < nsyms; i++)
			handled = handle_keybinding(syms[i]);
	
	//Window modifier
	if (!handled && modifiers & WLR_MODIFIER_LOGO)
		for (int i = 0; i < nsyms; i++)
			if (syms[i] == XKB_KEY_Control_L && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
			{
				server->b_window_modifier_key_pressed = true;
				handled = true;
				break;
			}
			else if (syms[i] == XKB_KEY_Control_L && event->state == WL_KEYBOARD_KEY_STATE_RELEASED)
			{
				server->b_window_modifier_key_pressed = false;
				handled = true;
				break;
			}

	//Window switching
	if (!handled && (modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		for (int i = 0; i < nsyms; i++)
			if (syms[i] == XKB_KEY_Tab)
			{				
				server->next_window_index++;
				if (server->next_window_index > server->window_list.size() - 1)
					server->next_window_index = 0;

				handled = true;
				break;
			}
	}
	else if (!handled && event->state == WL_KEYBOARD_KEY_STATE_RELEASED)
	{
		for (int i = 0; i < nsyms; i++)
			if (syms[i] == XKB_KEY_Alt_L)
			{				
				eshywm_window* next_window = server->window_list[server->next_window_index];
				next_window->focus_window(next_window->xdg_toplevel->base->surface);
				server->next_window_index = 0;
				handled = true;
				break;
			}
	}

	if (!handled)
	{
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
