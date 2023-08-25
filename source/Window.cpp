
#include "Window.h"
#include "EshyWM.h"
#include "Output.h"
#include "Config.h"
#include "Util.h"

#include "EshyIPC.h"

#define static
#define class wlr

extern "C"
{
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/edges.h>

#include <wlr/xwayland.h>
}

#undef static
#undef class

#include <nlohmann/json.hpp>

#include <algorithm>
#include <assert.h>

static void WindowMap(struct wl_listener* listener, void* data);
static void WindowUnmap(struct wl_listener* listener, void* data);
static void WindowRequestMove(struct wl_listener* listener, void* data);
static void WindowRequestResize(struct wl_listener* listener, void* data);
static void WindowRequestMaximize(struct wl_listener* listener, void* data);
static void WindowRequestFullscreen(struct wl_listener* listener, void* data);
static void WindowSetTitle(struct wl_listener* listener, void* data);

static void XdgToplevelDestroy(struct wl_listener* listener, void* data);
static void XdgToplevelSetAppId(struct wl_listener* listener, void* data);

static void XWindowAssociate(struct wl_listener* listener, void* data);
static void XWindowDissociate(struct wl_listener* listener, void* data);
static void XWindowMap(struct wl_listener* listener, void* data);
static void XWindowUnmap(struct wl_listener* listener, void* data);
static void XWaylandDestroy(struct wl_listener* listener, void* data);
static void XWindowCommit(struct wl_listener* listener, void* data);
static void XRequestActivateWindow(struct wl_listener* listener, void* data);
static void XRequestConfigureWindow(struct wl_listener* listener, void* data);
static void XSetHints(struct wl_listener* listener, void* data);

EshyWMWindowBase* DesktopWindowAt(double lx, double ly, struct wlr_surface** surface, double* sx, double* sy)
{
	/*This returns the topmost node in the scene at the given layout coords.
	*  we only care about surface nodes as we are specifically looking for a
	*  surface in the surface tree of a eshywm_window.*/
	struct wlr_scene_node* node = wlr_scene_node_at(&Server->Scene->tree.node, lx, ly, sx, sy);
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

	return (EshyWMWindowBase*)tree->node.data;
}


void EshyWMWindowBase::FocusWindow()
{
	//Don't re-focus an already focused surface
	if (Server->FocusedWindow == this)
		return;

	if (Server->FocusedWindow)
		Server->FocusedWindow->UnfocusWindow();

	Server->FocusedWindow = this;

	auto pos = std::find(Server->WindowList.begin(), Server->WindowList.end(), this);
	if(pos != Server->WindowList.end())
	{
		auto it = Server->WindowList.begin() + std::distance(Server->WindowList.begin(), pos);
    	std::rotate(Server->WindowList.begin(), it, it + 1);
	}

	for(int i = 0; i < 4; ++i)
		wlr_scene_rect_set_color(Border[i], EshyWMConfig::ESHYWM_COLOR_BORDER_FOCUSED);

	//Move the window to the front
	wlr_scene_node_raise_to_top(&Scene->node);
}

void EshyWMWindowBase::UnfocusWindow()
{
	for(int i = 0; i < 4; ++i)
		wlr_scene_rect_set_color(Border[i], EshyWMConfig::ESHYWM_COLOR_BORDER_NORMAL);
}

void EshyWMWindowBase::CreateBorder()
{
	UpdateWindowGeometry();

	for(int i = 0; i < 4; ++i)
	{
		Border[i] = wlr_scene_rect_create(Scene, 0, 0, EshyWMConfig::ESHYWM_COLOR_BORDER_NORMAL);
		Border[i]->node.data = this;
	}

	UpdateBorder();
}

void EshyWMWindowBase::DestroyBorder()
{
	for(int i = 0; i < 4; ++i)
		wlr_scene_node_destroy(&Border[i]->node);
}

void EshyWMWindowBase::UpdateBorder()
{
	wlr_scene_rect_set_size(Border[BS_TOP], WindowGeometry.width, EshyWMConfig::ESHYWM_BORDER_WIDTH);
	wlr_scene_rect_set_size(Border[BS_BOTTOM], WindowGeometry.width, EshyWMConfig::ESHYWM_BORDER_WIDTH);
	wlr_scene_rect_set_size(Border[BS_LEFT], EshyWMConfig::ESHYWM_BORDER_WIDTH, WindowGeometry.height);
	wlr_scene_rect_set_size(Border[BS_RIGHT], EshyWMConfig::ESHYWM_BORDER_WIDTH, WindowGeometry.height + EshyWMConfig::ESHYWM_BORDER_WIDTH);

	wlr_scene_node_set_position(&Border[BS_BOTTOM]->node, 0, WindowGeometry.height);
	wlr_scene_node_set_position(&Border[BS_LEFT]->node, 0, 0);
	wlr_scene_node_set_position(&Border[BS_RIGHT]->node, WindowGeometry.width, 0);
}


EshyWMWindow::EshyWMWindow(struct wlr_xdg_surface* XdgSurface)
	: EshyWMWindowBase()
{
	XdgToplevel = XdgSurface->toplevel;
	WindowType = WT_XDGShell;

	add_listener(&MapListener, WindowMap, &XdgSurface->surface->events.map);
	add_listener(&UnmapListener, WindowUnmap, &XdgSurface->surface->events.unmap);
	add_listener(&DestroyListener, XdgToplevelDestroy, &XdgSurface->events.destroy);

	add_listener(&RequestMoveListener, WindowRequestMove, &XdgToplevel->events.request_move);
	add_listener(&RequestResizeListener, WindowRequestResize, &XdgToplevel->events.request_resize);
	add_listener(&RequestMaximizeListener, WindowRequestMaximize, &XdgToplevel->events.request_maximize);
	add_listener(&RequestFullscreenListener, WindowRequestFullscreen, &XdgToplevel->events.request_fullscreen);
	add_listener(&SetTitleListener, WindowSetTitle, &XdgToplevel->events.set_title);

	add_listener(&SetAppIdListener, XdgToplevelSetAppId, &XdgToplevel->events.set_app_id);
}

void EshyWMWindow::FocusWindow()
{
	//Don't re-focus an already focused surface
	if (Server->FocusedWindow == this)
		return;

	EshyWMWindowBase::FocusWindow();
	
	wlr_xdg_toplevel_set_activated(XdgToplevel, true);
	
	if (const wlr_keyboard* keyboard = wlr_seat_get_keyboard(Server->Seat))
		wlr_seat_keyboard_notify_enter(Server->Seat, XdgToplevel->base->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
}

void EshyWMWindow::UnfocusWindow()
{
	EshyWMWindowBase::UnfocusWindow();
	wlr_xdg_toplevel_set_activated(XdgToplevel->base->toplevel, false);
}

void EshyWMWindow::UpdateWindowGeometry()
{
	wlr_xdg_surface_get_geometry(XdgToplevel->base, &WindowGeometry);
}

void EshyWMWindow::BeginInteractive(enum EshyWMCursorMode mode, uint32_t edges)
{
	struct wlr_surface* focused_surface = Server->Seat->pointer_state.focused_surface;

	//Deny move/resize requests from unfocused clients
	if (XdgToplevel->base->surface != wlr_surface_get_root_surface(focused_surface))
		return;

	Server->FocusedWindow = this;
	Server->CursorMode = mode;

	if (mode == ESHYWM_CURSOR_MOVE)
	{
		Server->grab_x = Server->Cursor->x - Scene->node.x;
		Server->grab_y = Server->Cursor->y - Scene->node.y;
	}
	else
	{
		struct wlr_box geo_box;
		wlr_xdg_surface_get_geometry(XdgToplevel->base, &geo_box);

		double border_x = (Scene->node.x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
		double border_y = (Scene->node.y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);
		Server->grab_x = Server->Cursor->x - (edges == 0 ? 0 : border_x);
		Server->grab_y = Server->Cursor->y - (edges == 0 ? 0 : border_y);

		Server->GrabGeobox = geo_box;
		Server->GrabGeobox.x += Scene->node.x;
		Server->GrabGeobox.y += Scene->node.y;

		Server->ResizeEdges = edges;
	}

	FullscreenWindow(false);
	MaximizeWindow(false);
}

void EshyWMWindow::ProcessCursorMove(uint32_t time)
{
	WindowGeometry.x = Server->Cursor->x - Server->grab_x;
	WindowGeometry.y = Server->Cursor->y - Server->grab_y;
	wlr_scene_node_set_position(&Scene->node, Server->Cursor->x - Server->grab_x, Server->Cursor->y - Server->grab_y);
}

void EshyWMWindow::ProcessCursorResize(uint32_t time)
{
	if(Server->ResizeEdges == 0)
	{
		WindowGeometry.width = std::max((double)100, Server->GrabGeobox.width + (Server->Cursor->x - Server->grab_x));
		WindowGeometry.height = std::max((double)100, Server->GrabGeobox.height + (Server->Cursor->y - Server->grab_y));
		wlr_xdg_toplevel_set_size(XdgToplevel, WindowGeometry.width, WindowGeometry.height);
		UpdateBorder();
		return;
	}

	double border_x = Server->Cursor->x - Server->grab_x;
	double border_y = Server->Cursor->y - Server->grab_y;
	int new_left = Server->GrabGeobox.x;
	int new_right = Server->GrabGeobox.x + Server->GrabGeobox.width;
	int new_top = Server->GrabGeobox.y;
	int new_bottom = Server->GrabGeobox.y + Server->GrabGeobox.height;

	if (Server->ResizeEdges & WLR_EDGE_TOP)
	{
		new_top = border_y;
		if (new_top >= new_bottom)
			new_top = new_bottom - 1;
	}
	else if (Server->ResizeEdges & WLR_EDGE_BOTTOM)
	{
		new_bottom = border_y;
		if (new_bottom <= new_top)
			new_bottom = new_top + 1;
	}
	if (Server->ResizeEdges & WLR_EDGE_LEFT)
	{
		new_left = border_x;
		if (new_left >= new_right)
			new_left = new_right - 1;
	}
	else if (Server->ResizeEdges & WLR_EDGE_RIGHT)
	{
		new_right = border_x;
		if (new_right <= new_left)
			new_right = new_left + 1;
	}

	struct wlr_box geo_box;
	wlr_xdg_surface_get_geometry(XdgToplevel->base, &geo_box);
	wlr_scene_node_set_position(&Scene->node, new_left - geo_box.x, new_top - geo_box.y);

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
			SavedGeo.x += Scene->node.x;
			SavedGeo.y += Scene->node.y;
		}
		
		int width;
		int height;
		wlr_output_effective_resolution(Server->OutputList[0]->WlrOutput, &width, &height);

		wlr_xdg_toplevel_set_size(XdgToplevel, width, height);
		wlr_scene_node_set_position(&Scene->node, 0, 0);

		DestroyBorder();

		WindowState = ESHYWM_WINDOW_STATE_FULLSCREEN;
	}
	else if (WindowState == ESHYWM_WINDOW_STATE_FULLSCREEN)
	{
		wlr_xdg_toplevel_set_size(XdgToplevel, SavedGeo.width, SavedGeo.height);
		wlr_scene_node_set_position(&Scene->node, SavedGeo.x, SavedGeo.y);

		CreateBorder();

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
			SavedGeo.x += Scene->node.x;
			SavedGeo.y += Scene->node.y;
		}

		int width;
		int height;
		wlr_output_effective_resolution(Server->OutputList[0]->WlrOutput, &width, &height);

		wlr_xdg_toplevel_set_size(XdgToplevel, width, height - 50.0f);
		wlr_scene_node_set_position(&Scene->node, 0, 0);

		WindowState = ESHYWM_WINDOW_STATE_MAXIMIZED;
	}
	else if (WindowState == ESHYWM_WINDOW_STATE_MAXIMIZED)
	{
		wlr_xdg_toplevel_set_size(XdgToplevel, SavedGeo.width, SavedGeo.height);
		wlr_scene_node_set_position(&Scene->node, SavedGeo.x, SavedGeo.y);

		WindowState = ESHYWM_WINDOW_STATE_NORMAL;
	}
}

void EshyWMWindow::MinimizeWindow(bool b_minimize)
{
	if (!XdgToplevel || !XdgToplevel->base || !XdgToplevel->base->surface)
		return;

	if(b_minimize)
		wlr_surface_unmap(XdgToplevel->base->surface);
	else
		wlr_surface_map(XdgToplevel->base->surface);
}


EshyWMXWindow::EshyWMXWindow(struct wlr_xwayland_surface* XSurface)
	: EshyWMWindowBase()
{
	XWaylandSurface = XSurface;
	WindowType = XSurface->override_redirect ? WT_X11Unmanaged : WT_X11Managed;

	add_listener(&XAssociateListener, XWindowAssociate, &XSurface->events.associate);
	add_listener(&XDissociateListener, XWindowDissociate, &XSurface->events.dissociate);
	add_listener(&DestroyListener, XWaylandDestroy, &XSurface->events.destroy);

	add_listener(&RequestMoveListener, WindowRequestMove, &XSurface->events.request_move);
	add_listener(&RequestResizeListener, WindowRequestResize, &XSurface->events.request_resize);
	add_listener(&RequestMaximizeListener, WindowRequestMaximize, &XSurface->events.request_maximize);
	add_listener(&RequestFullscreenListener, WindowRequestFullscreen, &XSurface->events.request_fullscreen);
	add_listener(&SetTitleListener, WindowSetTitle, &XSurface->events.set_title);

	add_listener(&XActivateListener, XRequestActivateWindow, &XSurface->events.request_activate);
	add_listener(&XConfigureListener, XRequestConfigureWindow, &XSurface->events.request_configure);
	add_listener(&XSetHintsListener, XSetHints, &XSurface->events.set_hints);
}

void EshyWMXWindow::FocusWindow()
{
	//Don't re-focus an already focused surface
	if (Server->FocusedWindow == this || WindowType != WT_X11Managed)
		return;

	EshyWMWindowBase::FocusWindow();
	
	//Only managed windows can be activated
	if(WindowType == WT_X11Managed)
	{
		wlr_xwayland_surface_activate(XWaylandSurface, true);

		if (const wlr_keyboard* keyboard = wlr_seat_get_keyboard(Server->Seat))
			wlr_seat_keyboard_notify_enter(Server->Seat, XWaylandSurface->surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
	}
}

void EshyWMXWindow::UnfocusWindow()
{
	EshyWMWindowBase::UnfocusWindow();

	if(!XWaylandSurface->override_redirect)
		wlr_xwayland_surface_activate(XWaylandSurface, false);
}

void EshyWMXWindow::UpdateWindowGeometry()
{
	WindowGeometry.x = XWaylandSurface->x;
	WindowGeometry.y = XWaylandSurface->y;
	WindowGeometry.width = XWaylandSurface->width;
	WindowGeometry.height = XWaylandSurface->height;
}

void EshyWMXWindow::BeginInteractive(enum EshyWMCursorMode mode, uint32_t edges)
{
	struct wlr_surface* focused_surface = Server->Seat->pointer_state.focused_surface;

	//Deny move/resize requests from unfocused clients
	if (XWaylandSurface->surface != wlr_surface_get_root_surface(focused_surface))
		return;

	Server->FocusedWindow = this;
	Server->CursorMode = mode;

	if (mode == ESHYWM_CURSOR_MOVE)
	{
		Server->grab_x = Server->Cursor->x - Scene->node.x;
		Server->grab_y = Server->Cursor->y - Scene->node.y;
	}
	else
	{
		struct wlr_box geo_box = {XWaylandSurface->x, XWaylandSurface->y, XWaylandSurface->width, XWaylandSurface->height};

		double border_x = (Scene->node.x + geo_box.x) + ((edges & WLR_EDGE_RIGHT) ? geo_box.width : 0);
		double border_y = (Scene->node.y + geo_box.y) + ((edges & WLR_EDGE_BOTTOM) ? geo_box.height : 0);
		Server->grab_x = Server->Cursor->x - (edges == 0 ? 0 : border_x);
		Server->grab_y = Server->Cursor->y - (edges == 0 ? 0 : border_y);

		Server->GrabGeobox = geo_box;
		Server->GrabGeobox.x += Scene->node.x;
		Server->GrabGeobox.y += Scene->node.y;

		Server->ResizeEdges = edges;
	}

	FullscreenWindow(false);
	MaximizeWindow(false);
}

void EshyWMXWindow::ProcessCursorMove(uint32_t time)
{
	WindowGeometry.x = Server->Cursor->x - Server->grab_x;
	WindowGeometry.y = Server->Cursor->y - Server->grab_y;
	wlr_scene_node_set_position(&Scene->node, WindowGeometry.x, WindowGeometry.y);
}

void EshyWMXWindow::ProcessCursorResize(uint32_t time)
{
	WindowGeometry.width = std::max((double)100, Server->GrabGeobox.width + (Server->Cursor->x - Server->grab_x));
	WindowGeometry.height = std::max((double)100, Server->GrabGeobox.height + (Server->Cursor->y - Server->grab_y));
	wlr_xwayland_surface_configure(XWaylandSurface, Server->GrabGeobox.x, Server->GrabGeobox.y, WindowGeometry.width, WindowGeometry.height);
	UpdateBorder();
}


static void WindowDestroy(EshyWMWindowBase* window)
{
	nlohmann::json WindowRemoveInfo;
	WindowRemoveInfo["action"] = ACTION_REMOVE_WINDOW;
	WindowRemoveInfo["sender_client"] = CLIENT_COMPOSITOR;
	WindowRemoveInfo["window_id"] = (uint64_t)window;
	EshyIPC::InsertIntoMemory(EshybarShmID, WindowRemoveInfo.dump());

	wl_list_remove(&window->DestroyListener.link);
	wl_list_remove(&window->RequestMoveListener.link);
	wl_list_remove(&window->RequestResizeListener.link);
	wl_list_remove(&window->RequestMaximizeListener.link);
	wl_list_remove(&window->RequestFullscreenListener.link);
	wl_list_remove(&window->SetTitleListener.link);
	free(window);

	auto pos = std::find(Server->WindowList.begin(), Server->WindowList.end(), window);
	if(pos != Server->WindowList.end())
	{
		Server->WindowList.erase(Server->WindowList.begin() + std::distance(Server->WindowList.begin(), pos));
	}
}

void WindowMap(struct wl_listener* listener, void* data)
{
	/*Called when the surface is mapped, or ready to display on-screen.*/
	EshyWMWindow* window = wl_container_of(listener, window, MapListener);

	window->Scene = wlr_scene_tree_create(Server->Layers[L_Float]);
	wlr_scene_node_set_enabled(&window->Scene->node, true);
	window->SceneTree = wlr_scene_subsurface_tree_create(window->Scene, window->XdgToplevel->base->surface);
	window->XdgToplevel->base->data = window->Scene;
	window->Scene->node.data = window->SceneTree->node.data = window;

	window->CreateBorder();
	window->FocusWindow();
}

void WindowUnmap(struct wl_listener* listener, void* data)
{
	/*Called when the surface is unmapped, and should no longer be shown.*/
	EshyWMWindow* window = wl_container_of(listener, window, UnmapListener);
	window->DestroyBorder();

	/*Reset the cursor mode if the grabbed window was unmapped.*/
	if (window == Server->FocusedWindow)
	{
		Server->ResetCursorMode();
		Server->FocusedWindow = nullptr;
	}
}

void WindowRequestMove(struct wl_listener* listener, void* data)
{
	EshyWMWindowBase* window = wl_container_of(listener, window, RequestMoveListener);
	window->BeginInteractive(ESHYWM_CURSOR_MOVE, 0);
}

void WindowRequestResize(struct wl_listener* listener, void* data)
{
	struct wlr_xdg_toplevel_resize_event* event = (wlr_xdg_toplevel_resize_event*)data;
	EshyWMWindowBase* window = wl_container_of(listener, window, RequestResizeListener);
	window->BeginInteractive(ESHYWM_CURSOR_RESIZE, event->edges);
}

void WindowRequestMaximize(struct wl_listener* listener, void* data)
{
	EshyWMWindowBase* window = wl_container_of(listener, window, RequestMaximizeListener);
	window->MaximizeWindow(true);
}

void WindowRequestFullscreen(struct wl_listener* listener, void* data)
{
	EshyWMWindow* window = wl_container_of(listener, window, RequestFullscreenListener);
	window->FullscreenWindow(true);
}

void WindowSetTitle(struct wl_listener* listener, void* data)
{
	
}


void XdgToplevelDestroy(struct wl_listener* listener, void* data)
{
	/*Called when the surface is destroyed and should never be shown again.*/
	EshyWMWindow* window = wl_container_of(listener, window, DestroyListener);
	wl_list_remove(&window->SetAppIdListener.link);
	wl_list_remove(&window->MapListener.link);
	wl_list_remove(&window->UnmapListener.link);
	WindowDestroy(window);
}

void XdgToplevelSetAppId(struct wl_listener* listener, void* data)
{
	
}


void XWindowAssociate(struct wl_listener* listener, void* data)
{
	EshyWMXWindow* window = wl_container_of(listener, window, XAssociateListener);
	add_listener(&window->MapListener, XWindowMap, &window->XWaylandSurface->surface->events.map);
	add_listener(&window->UnmapListener, XWindowUnmap, &window->XWaylandSurface->surface->events.unmap);
}

void XWindowDissociate(struct wl_listener* listener, void* data)
{
	EshyWMXWindow* window = wl_container_of(listener, window, XDissociateListener);
	wl_list_remove(&window->MapListener.link);
	wl_list_remove(&window->UnmapListener.link);
}

void XWindowMap(struct wl_listener* listener, void* data)
{
	EshyWMXWindow* window = wl_container_of(listener, window, MapListener);

	window->Scene = wlr_scene_tree_create(Server->Layers[L_Float]);
	wlr_scene_node_set_enabled(&window->Scene->node, true);
	window->SceneTree = wlr_scene_subsurface_tree_create(window->Scene, window->XWaylandSurface->surface);
	window->XWaylandSurface->data = window->Scene;
	window->Scene->node.data = window->SceneTree->node.data = window;

	//Attach commit listener here because xwayland map and unmap can change the underlying wlr_surface
	add_listener(&window->XCommitListener, XWindowCommit, &window->XWaylandSurface->surface->events.commit);

	if(window->WindowType == WT_X11Managed)
	{
		window->CreateBorder();
		window->FocusWindow();
	}
	else
	{
		wlr_scene_node_reparent(&window->Scene->node, Server->Layers[L_Float]);
		window->WindowGeometry.x = Server->Cursor->x;
		window->WindowGeometry.y = Server->Cursor->y;
		wlr_scene_node_set_position(&window->Scene->node, window->WindowGeometry.x, window->WindowGeometry.y);
	}
}

void XWindowUnmap(struct wl_listener* listener, void* data)
{
	EshyWMXWindow* window = wl_container_of(listener, window, UnmapListener);

	if(window->WindowType == WT_X11Managed)
		window->DestroyBorder();

	wl_list_remove(&window->XCommitListener.link);

	/*Reset the cursor mode if the grabbed window was unmapped.*/
	if (window == Server->FocusedWindow)
	{
		Server->ResetCursorMode();
		Server->FocusedWindow = nullptr;
	}
}

void XWaylandDestroy(struct wl_listener* listener, void* data)
{
	/*Called when the surface is destroyed and should never be shown again.*/
	EshyWMXWindow* window = wl_container_of(listener, window, DestroyListener);
	wl_list_remove(&window->XActivateListener.link);
	wl_list_remove(&window->XConfigureListener.link);
	wl_list_remove(&window->XSetHintsListener.link);
	WindowDestroy(window);
}

void XWindowCommit(struct wl_listener* listener, void* data)
{
	
}

void XRequestActivateWindow(struct wl_listener* listener, void* data)
{
	EshyWMXWindow* window = wl_container_of(listener, window, XActivateListener);

	//Only managed windows can be activated
	if(window->WindowType == WT_X11Managed)
		wlr_xwayland_surface_activate(window->XWaylandSurface, true);
}

void XRequestConfigureWindow(struct wl_listener* listener, void* data)
{
	struct wlr_xwayland_surface_configure_event* event = (wlr_xwayland_surface_configure_event*)data;
	wlr_xwayland_surface_configure(event->surface, event->x, event->y, event->width, event->height);
}

void XSetHints(struct wl_listener* listener, void* data)
{
	EshyWMXWindow* window = wl_container_of(listener, window, XSetHintsListener);
}