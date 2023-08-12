
#include "Window.h"
#include "EshyWM.h"
#include "Output.h"
#include "Util.h"

#include "EshyIPC.h"

static void XdgToplevelMap(struct wl_listener* listener, void* data);
static void XdgToplevelUnmap(struct wl_listener* listener, void* data);
static void XdgToplevelDestroy(struct wl_listener* listener, void* data);
static void XdgToplevelRequestMove(struct wl_listener* listener, void* data);
static void XdgToplevelRequestResize(struct wl_listener* listener, void* data);
static void XdgToplevelRequestMaximize(struct wl_listener* listener, void* data);
static void XdgToplevelRequestFullscreen(struct wl_listener* listener, void* data);
static void XdgToplevelSetTitle(struct wl_listener* listener, void* data);
static void XdgToplevelSetAppId(struct wl_listener* listener, void* data);

EshyWMWindow* DesktopWindowAt(double lx, double ly, struct wlr_surface** surface, double* sx, double* sy)
{
	/*This returns the topmost node in the scene at the given layout coords.
	*  we only care about surface nodes as we are specifically looking for a
	*  surface in the surface tree of a eshywm_window.*/
	struct wlr_scene_node* node = wlr_scene_node_at(&Server->scene->tree.node, lx, ly, sx, sy);
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

	return (EshyWMWindow*)tree->node.data;
}

void EshyWMWindow::Initialize(wlr_xdg_surface *xdg_surface)
{
	XdgToplevel = xdg_surface->toplevel;
	SceneTree = wlr_scene_xdg_surface_create(&Server->scene->tree, XdgToplevel->base);
	SceneTree->node.data = this;
	xdg_surface->data = SceneTree;

	/*Listen to the various events it can emit*/
	add_listener(&MapListener, XdgToplevelMap, &xdg_surface->surface->events.map);
	add_listener(&UnmapListener, XdgToplevelUnmap, &xdg_surface->surface->events.unmap);
	add_listener(&DestroyListener, XdgToplevelDestroy, &xdg_surface->events.destroy);

	struct wlr_xdg_toplevel* toplevel = xdg_surface->toplevel;
	add_listener(&RequestMoveListener, XdgToplevelRequestMove, &toplevel->events.request_move);
	add_listener(&RequestResizeListener, XdgToplevelRequestResize, &toplevel->events.request_resize);
	add_listener(&RequestMaximizeListener, XdgToplevelRequestMaximize, &toplevel->events.request_maximize);
	add_listener(&RequestFullscreenListener, XdgToplevelRequestFullscreen, &toplevel->events.request_fullscreen);
	add_listener(&SetTitleListener, XdgToplevelSetTitle, &toplevel->events.set_title);
	add_listener(&SetAppIdListener, XdgToplevelSetAppId, &toplevel->events.set_app_id);
}

void EshyWMWindow::FocusWindow(struct wlr_surface *surface)
{
	//Note: this function only deals with keyboard focus
	struct wlr_seat* seat = Server->seat;
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

	//MinimizeWindow(false);

	Server->focused_window = this;
	
	//Move the window to the front
	wlr_scene_node_raise_to_top(&SceneTree->node);

	auto pos = std::find(Server->window_list.begin(), Server->window_list.end(), this);
	if(pos != Server->window_list.end())
	{
		auto it = Server->window_list.begin() + std::distance(Server->window_list.begin(), pos);
    	std::rotate(Server->window_list.begin(), it, it + 1);
	}

	wlr_xdg_toplevel_set_activated(XdgToplevel, true);
	
	if (keyboard != NULL)
		wlr_seat_keyboard_notify_enter(seat, XdgToplevel->base->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void EshyWMWindow::BeginInteractive(enum EshyWMCursorMode mode, uint32_t edges)
{
	struct wlr_surface* focused_surface = Server->seat->pointer_state.focused_surface;

	//Deny move/resize requests from unfocused clients
	if (XdgToplevel->base->surface != wlr_surface_get_root_surface(focused_surface))
		return;

	Server->focused_window = this;
	Server->cursor_mode = mode;

	if (mode == ESHYWM_CURSOR_MOVE)
	{
		Server->grab_x = Server->cursor->x - SceneTree->node.x;
		Server->grab_y = Server->cursor->y - SceneTree->node.y;
	}
	else
	{
		struct wlr_box geo_box;
		wlr_xdg_surface_get_geometry(XdgToplevel->base, &geo_box);

		double border_x = (SceneTree->node.x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
		double border_y = (SceneTree->node.y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);
		Server->grab_x = Server->cursor->x - (edges == 0 ? 0 : border_x);
		Server->grab_y = Server->cursor->y - (edges == 0 ? 0 : border_y);

		Server->grab_geobox = geo_box;
		Server->grab_geobox.x += SceneTree->node.x;
		Server->grab_geobox.y += SceneTree->node.y;

		Server->resize_edges = edges;
	}

	FullscreenWindow(false);
	MaximizeWindow(false);
}

void EshyWMWindow::ProcessCursorMove(uint32_t time)
{
	wlr_scene_node_set_position(&SceneTree->node, Server->cursor->x - Server->grab_x, Server->cursor->y - Server->grab_y);
}

void EshyWMWindow::ProvessCursorResize(uint32_t time)
{
	if(Server->resize_edges == 0)
	{
		const int new_width = Server->grab_geobox.width + (Server->cursor->x - Server->grab_x);
		const int new_height = Server->grab_geobox.height + (Server->cursor->y - Server->grab_y);
		wlr_xdg_toplevel_set_size(XdgToplevel, new_width, new_height);
		return;
	}

	double border_x = Server->cursor->x - Server->grab_x;
	double border_y = Server->cursor->y - Server->grab_y;
	int new_left = Server->grab_geobox.x;
	int new_right = Server->grab_geobox.x + Server->grab_geobox.width;
	int new_top = Server->grab_geobox.y;
	int new_bottom = Server->grab_geobox.y + Server->grab_geobox.height;

	if (Server->resize_edges & WLR_EDGE_TOP)
	{
		new_top = border_y;
		if (new_top >= new_bottom)
			new_top = new_bottom - 1;
	}
	else if (Server->resize_edges & WLR_EDGE_BOTTOM)
	{
		new_bottom = border_y;
		if (new_bottom <= new_top)
			new_bottom = new_top + 1;
	}
	if (Server->resize_edges & WLR_EDGE_LEFT)
	{
		new_left = border_x;
		if (new_left >= new_right)
			new_left = new_right - 1;
	}
	else if (Server->resize_edges & WLR_EDGE_RIGHT)
	{
		new_right = border_x;
		if (new_right <= new_left)
			new_right = new_left + 1;
	}

	struct wlr_box geo_box;
	wlr_xdg_surface_get_geometry(XdgToplevel->base, &geo_box);
	wlr_scene_node_set_position(&SceneTree->node, new_left - geo_box.x, new_top - geo_box.y);

	int new_width = new_right - new_left;
	int new_height = new_bottom - new_top;
	wlr_xdg_toplevel_set_size(XdgToplevel, new_width, new_height);
}

void EshyWMWindow::FullscreenWindow(bool b_fullscreen)
{
	if (b_fullscreen)
	{
		if(WindowState != ESHYWM_WINDOW_STATE_MAXIMIZED && WindowState != ESHYWM_WINDOW_STATE_FULLSCREEN)
		{
			struct wlr_box geo_box;
			wlr_xdg_surface_get_geometry(XdgToplevel->base, &geo_box);
			SavedGeo = geo_box;
			SavedGeo.x += SceneTree->node.x;
			SavedGeo.y += SceneTree->node.y;
		}
		
		int width;
		int height;
		wlr_output_effective_resolution(Server->output_list[0]->WlrOutput, &width, &height);

		wlr_xdg_toplevel_set_size(XdgToplevel, width, height);
		wlr_scene_node_set_position(&SceneTree->node, 0, 0);

		WindowState = ESHYWM_WINDOW_STATE_FULLSCREEN;
	}
	else if (WindowState == ESHYWM_WINDOW_STATE_FULLSCREEN)
	{
		wlr_xdg_toplevel_set_size(XdgToplevel, SavedGeo.width, SavedGeo.height);
		wlr_scene_node_set_position(&SceneTree->node, SavedGeo.x, SavedGeo.y);

		WindowState = ESHYWM_WINDOW_STATE_NORMAL;
	}
}

void EshyWMWindow::MaximizeWindow(bool b_maximize)
{
	if (b_maximize)
	{
		if(WindowState != ESHYWM_WINDOW_STATE_MAXIMIZED && WindowState != ESHYWM_WINDOW_STATE_FULLSCREEN)
		{
			struct wlr_box geo_box;
			wlr_xdg_surface_get_geometry(XdgToplevel->base, &geo_box);
			SavedGeo = geo_box;
			SavedGeo.x += SceneTree->node.x;
			SavedGeo.y += SceneTree->node.y;
		}

		int width;
		int height;
		wlr_output_effective_resolution(Server->output_list[0]->WlrOutput, &width, &height);

		wlr_xdg_toplevel_set_size(XdgToplevel, width, height);
		wlr_scene_node_set_position(&SceneTree->node, 0, 0);

		WindowState = ESHYWM_WINDOW_STATE_MAXIMIZED;
	}
	else if (WindowState == ESHYWM_WINDOW_STATE_MAXIMIZED)
	{
		wlr_xdg_toplevel_set_size(XdgToplevel, SavedGeo.width, SavedGeo.height);
		wlr_scene_node_set_position(&SceneTree->node, SavedGeo.x, SavedGeo.y);

		WindowState = ESHYWM_WINDOW_STATE_NORMAL;
	}
}
static int i = 0;
void EshyWMWindow::MinimizeWindow(bool b_minimize)
{
	i++;
	//std::cout << "Minimize window: " << b_minimize << " " << i << std::endl;

	if(b_minimize)
	{
		wlr_surface_unmap(XdgToplevel->base->surface);
	}
	else
		wlr_surface_map(XdgToplevel->base->surface);
}

void XdgToplevelMap(struct wl_listener* listener, void* data)
{
	/*Called when the surface is mapped, or ready to display on-screen.*/
	class EshyWMWindow* window = wl_container_of(listener, window, MapListener);
	window->FocusWindow(window->XdgToplevel->base->surface);
}

void XdgToplevelUnmap(struct wl_listener* listener, void* data)
{
	/*Called when the surface is unmapped, and should no longer be shown.*/
	class EshyWMWindow* window = wl_container_of(listener, window, UnmapListener);

	/*Reset the cursor mode if the grabbed window was unmapped.*/
	if (window == Server->focused_window)
	{
		Server->ResetCursorMode();
		Server->focused_window = nullptr;
	}
}

void XdgToplevelDestroy(struct wl_listener* listener, void* data)
{
	/*Called when the surface is destroyed and should never be shown again.*/
	class EshyWMWindow* window = wl_container_of(listener, window, DestroyListener);

	nlohmann::json WindowRemoveInfo;
	WindowRemoveInfo["action"] = ACTION_REMOVE_WINDOW;
	WindowRemoveInfo["sender_client"] = CLIENT_COMPOSITOR;
	WindowRemoveInfo["window_id"] = std::to_string((uint64_t)window);
	EshyIPC::InsertIntoMemory(EshybarShmID, WindowRemoveInfo.dump());

	wl_list_remove(&window->MapListener.link);
	wl_list_remove(&window->UnmapListener.link);
	wl_list_remove(&window->DestroyListener.link);
	wl_list_remove(&window->RequestMoveListener.link);
	wl_list_remove(&window->RequestResizeListener.link);
	wl_list_remove(&window->RequestMaximizeListener.link);
	wl_list_remove(&window->RequestFullscreenListener.link);
	wl_list_remove(&window->SetTitleListener.link);
	wl_list_remove(&window->SetAppIdListener.link);
	free(window);

	auto pos = std::find(Server->window_list.begin(), Server->window_list.end(), window);
	if(pos != Server->window_list.end())
	{
		Server->window_list.erase(Server->window_list.begin() + std::distance(Server->window_list.begin(), pos));
	}
}

void XdgToplevelRequestMove(struct wl_listener* listener, void* data)
{
	/*This event is raised when a client would like to begin an interactive
	*  move, typically because the user clicked on their client-side
	*  decorations. Note that a more sophisticated compositor should check the
	*  provided serial against a list of button press serials sent to this
	*  client, to prevent the client from requesting this whenever they want.*/
	class EshyWMWindow* window = wl_container_of(listener, window, RequestMoveListener);
	window->BeginInteractive(ESHYWM_CURSOR_MOVE, 0);
}

void XdgToplevelRequestResize(struct wl_listener* listener, void* data)
{
	/*This event is raised when a client would like to begin an interactive
	*  resize, typically because the user clicked on their client-side
	*  decorations. Note that a more sophisticated compositor should check the
	*  provided serial against a list of button press serials sent to this
	*  client, to prevent the client from requesting this whenever they want.*/
	struct wlr_xdg_toplevel_resize_event* event = (wlr_xdg_toplevel_resize_event*)data;
	class EshyWMWindow* window = wl_container_of(listener, window, RequestResizeListener);
	window->BeginInteractive(ESHYWM_CURSOR_RESIZE, event->edges);
}

void XdgToplevelRequestMaximize(struct wl_listener* listener, void* data)
{
	/*This event is raised when a client would like to maximize itself,
	*  typically because the user clicked on the maximize button on
	*  client-side decorations. eshywm doesn't support maximization, but
	*  to conform to xdg-shell protocol we still must send a configure.
	*  wlr_xdg_surface_schedule_configure() is used to send an empty reply.*/
	class EshyWMWindow* window = wl_container_of(listener, window, RequestMaximizeListener);
	wlr_xdg_surface_schedule_configure(window->XdgToplevel->base);
}

void XdgToplevelRequestFullscreen(struct wl_listener* listener, void* data)
{
	/*Just as with request_maximize, we must send a configure here.*/
	class EshyWMWindow* window = wl_container_of(listener, window, RequestFullscreenListener);
	wlr_xdg_surface_schedule_configure(window->XdgToplevel->base);
}

void XdgToplevelSetTitle(struct wl_listener* listener, void* data)
{
	//Update information in EshyBar shared memory
	//class EshyWMWindow* window = wl_container_of(listener, window, MapListener);
	//Server->ToplevelsInfoList[std::to_string((uint64_t)window)]["title"] = (char*)data;
	//EshyIPC::InsertIntoMemory(EshybarShmID, Server->ToplevelsInfoList.dump());
}

void XdgToplevelSetAppId(struct wl_listener* listener, void* data)
{
	//Update information in EshyBar shared memory
	//class EshyWMWindow* window = wl_container_of(listener, window, MapListener);
	//Server->ToplevelsInfoList[std::to_string((uint64_t)window)]["app-id"] = (char*)data;
	//EshyIPC::InsertIntoMemory(EshybarShmID, Server->ToplevelsInfoList.dump());
}
