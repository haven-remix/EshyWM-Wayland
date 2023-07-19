
#include "window.h"
#include "eshywm.h"

static void xdg_toplevel_map(struct wl_listener* listener, void* data);
static void xdg_toplevel_unmap(struct wl_listener* listener, void* data);
static void xdg_toplevel_destroy(struct wl_listener* listener, void* data);
static void xdg_toplevel_request_move(struct wl_listener* listener, void* data);
static void xdg_toplevel_request_resize(struct wl_listener* listener, void* data);
static void xdg_toplevel_request_maximize(struct wl_listener* listener, void* data);
static void xdg_toplevel_request_fullscreen(struct wl_listener* listener, void* data);

void eshywm_window::initialize(wlr_xdg_surface *xdg_surface)
{
	xdg_toplevel = xdg_surface->toplevel;
	scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree, xdg_toplevel->base);
	scene_tree->node.data = this;
	xdg_surface->data = scene_tree;

	/*Listen to the various events it can emit*/
	map.notify = xdg_toplevel_map;
	wl_signal_add(&xdg_surface->surface->events.map, &map);
	unmap.notify = xdg_toplevel_unmap;
	wl_signal_add(&xdg_surface->surface->events.unmap, &unmap);
	destroy.notify = xdg_toplevel_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &destroy);

	/*cotd*/
	struct wlr_xdg_toplevel* toplevel = xdg_surface->toplevel;
	request_move.notify = xdg_toplevel_request_move;
	wl_signal_add(&toplevel->events.request_move, &request_move);
	request_resize.notify = xdg_toplevel_request_resize;
	wl_signal_add(&toplevel->events.request_resize, &request_resize);
	request_maximize.notify = xdg_toplevel_request_maximize;
	wl_signal_add(&toplevel->events.request_maximize, &request_maximize);
	request_fullscreen.notify = xdg_toplevel_request_fullscreen;
	wl_signal_add(&toplevel->events.request_fullscreen, &request_fullscreen);
}

void eshywm_window::focus_window(struct wlr_surface *surface)
{
	//Note: this function only deals with keyboard focus
	struct wlr_seat* seat = server->seat;
	struct wlr_surface* prev_surface = seat->keyboard_state.focused_surface;
	
	//Don't re-focus an already focused surface
	if (prev_surface == surface)
		return;

	if (prev_surface)
	{
		struct wlr_xdg_surface* previous = wlr_xdg_surface_try_from_wlr_surface(seat->keyboard_state.focused_surface);
		assert(previous != NULL && previous->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL);
		wlr_xdg_toplevel_set_activated(previous->toplevel, false);
	}

	struct wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
	
	//Move the window to the front
	wlr_scene_node_raise_to_top(&scene_tree->node);

	auto pos = std::find(server->window_list.begin(), server->window_list.end(), this);
	if(pos != server->window_list.end())
	{
		auto it = server->window_list.begin() + std::distance(server->window_list.begin(), pos);
    	std::rotate(server->window_list.begin(), it, it + 1);
	}

	wlr_xdg_toplevel_set_activated(xdg_toplevel, true);
	
	if (keyboard != NULL)
		wlr_seat_keyboard_notify_enter(seat, xdg_toplevel->base->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void eshywm_window::begin_interactive(enum eshywm_cursor_mode mode, uint32_t edges)
{
	struct wlr_surface* focused_surface = server->seat->pointer_state.focused_surface;

	//Deny move/resize requests from unfocused clients
	if (xdg_toplevel->base->surface != wlr_surface_get_root_surface(focused_surface))
		return;

	server->grabbed_window = this;
	server->cursor_mode = mode;

	if (mode == ESHYWM_CURSOR_MOVE)
	{
		server->grab_x = server->cursor->x - scene_tree->node.x;
		server->grab_y = server->cursor->y - scene_tree->node.y;
	}
	else
	{
		struct wlr_box geo_box;
		wlr_xdg_surface_get_geometry(xdg_toplevel->base, &geo_box);

		double border_x = (scene_tree->node.x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
		double border_y = (scene_tree->node.y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);
		server->grab_x = server->cursor->x - border_x;
		server->grab_y = server->cursor->y - border_y;

		server->grab_geobox = geo_box;
		server->grab_geobox.x += scene_tree->node.x;
		server->grab_geobox.y += scene_tree->node.y;

		server->resize_edges = edges;
	}
}

void eshywm_window::process_cursor_move(uint32_t time)
{
	/*Move the grabbed window to the new position.*/
	wlr_scene_node_set_position(&scene_tree->node, server->cursor->x - server->grab_x, server->cursor->y - server->grab_y);
}

void eshywm_window::process_cursor_resize(uint32_t time)
{
	/*
	*  Resizing the grabbed window can be a little bit complicated, because we
	*  could be resizing from any corner or edge. This not only resizes the window
	*  on one or two axes, but can also move the window if you resize from the top
	*  or left edges (or top-left corner).
	* 
	*  Note that I took some shortcuts here. In a more fleshed-out compositor,
	*  you'd wait for the client to prepare a buffer at the new size, then
	*  commit any movement that was prepared.
	*/
	double border_x = server->cursor->x - server->grab_x;
	double border_y = server->cursor->y - server->grab_y;
	int new_left = server->grab_geobox.x;
	int new_right = server->grab_geobox.x + server->grab_geobox.width;
	int new_top = server->grab_geobox.y;
	int new_bottom = server->grab_geobox.y + server->grab_geobox.height;

	if (server->resize_edges & WLR_EDGE_TOP)
	{
		new_top = border_y;
		if (new_top >= new_bottom)
		{
			new_top = new_bottom - 1;
		}
	}
	else if (server->resize_edges & WLR_EDGE_BOTTOM)
	{
		new_bottom = border_y;
		if (new_bottom <= new_top)
		{
			new_bottom = new_top + 1;
		}
	}
	if (server->resize_edges & WLR_EDGE_LEFT)
	{
		new_left = border_x;
		if (new_left >= new_right)
		{
			new_left = new_right - 1;
		}
	}
	else if (server->resize_edges & WLR_EDGE_RIGHT)
	{
		new_right = border_x;
		if (new_right <= new_left)
		{
			new_right = new_left + 1;
		}
	}

	struct wlr_box geo_box;
	wlr_xdg_surface_get_geometry(xdg_toplevel->base, &geo_box);
	wlr_scene_node_set_position(&scene_tree->node, new_left - geo_box.x, new_top - geo_box.y);

	int new_width = new_right - new_left;
	int new_height = new_bottom - new_top;
	wlr_xdg_toplevel_set_size(xdg_toplevel, new_width, new_height);
}

void xdg_toplevel_map(struct wl_listener* listener, void* data)
{
	/*Called when the surface is mapped, or ready to display on-screen.*/
	class eshywm_window* window = wl_container_of(listener, window, map);

	server->window_list.push_back(window);

	window->focus_window(window->xdg_toplevel->base->surface);
}

void xdg_toplevel_unmap(struct wl_listener* listener, void* data)
{
	/*Called when the surface is unmapped, and should no longer be shown.*/
	class eshywm_window* window = wl_container_of(listener, window, unmap);

	/*Reset the cursor mode if the grabbed window was unmapped.*/
	if (window == server->grabbed_window)
	{
		server->reset_cursor_mode();
	}

	//auto pos = std::find(server->window_list.begin(), server->window_list.end(), window);
	//if(pos != server->window_list.end())
	//{
	//	//server->window_list.erase(server->window_list.begin() + std::distance(server->window_list.begin(), pos));
	//}
}

void xdg_toplevel_destroy(struct wl_listener* listener, void* data)
{
	/*Called when the surface is destroyed and should never be shown again.*/
	class eshywm_window* window = wl_container_of(listener, window, destroy);

	wl_list_remove(&window->map.link);
	wl_list_remove(&window->unmap.link);
	wl_list_remove(&window->destroy.link);
	wl_list_remove(&window->request_move.link);
	wl_list_remove(&window->request_resize.link);
	wl_list_remove(&window->request_maximize.link);
	wl_list_remove(&window->request_fullscreen.link);
	free(window);
}

void xdg_toplevel_request_move(struct wl_listener* listener, void* data)
{
	/*This event is raised when a client would like to begin an interactive
	*  move, typically because the user clicked on their client-side
	*  decorations. Note that a more sophisticated compositor should check the
	*  provided serial against a list of button press serials sent to this
	*  client, to prevent the client from requesting this whenever they want.*/
	class eshywm_window* window = wl_container_of(listener, window, request_move);
	window->begin_interactive(ESHYWM_CURSOR_MOVE, 0);
}

void xdg_toplevel_request_resize(struct wl_listener* listener, void* data)
{
	/*This event is raised when a client would like to begin an interactive
	*  resize, typically because the user clicked on their client-side
	*  decorations. Note that a more sophisticated compositor should check the
	*  provided serial against a list of button press serials sent to this
	*  client, to prevent the client from requesting this whenever they want.*/
	struct wlr_xdg_toplevel_resize_event* event = (wlr_xdg_toplevel_resize_event*)data;
	class eshywm_window* window = wl_container_of(listener, window, request_resize);
	window->begin_interactive(ESHYWM_CURSOR_RESIZE, event->edges);
}

void xdg_toplevel_request_maximize(struct wl_listener* listener, void* data)
{
	/*This event is raised when a client would like to maximize itself,
	*  typically because the user clicked on the maximize button on
	*  client-side decorations. eshywm doesn't support maximization, but
	*  to conform to xdg-shell protocol we still must send a configure.
	*  wlr_xdg_surface_schedule_configure() is used to send an empty reply.*/
	class eshywm_window* window = wl_container_of(listener, window, request_maximize);
	wlr_xdg_surface_schedule_configure(window->xdg_toplevel->base);
}

void xdg_toplevel_request_fullscreen(struct wl_listener* listener, void* data)
{
	/*Just as with request_maximize, we must send a configure here.*/
	class eshywm_window* window = wl_container_of(listener, window, request_fullscreen);
	wlr_xdg_surface_schedule_configure(window->xdg_toplevel->base);
}