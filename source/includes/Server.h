
#pragma once

#include <string>
#include <vector>

#include <wayland-server-core.h>

#define static

extern "C"
{
#include <wlr/util/box.h>
}

#undef static

extern class EshyWMServer* Server;

enum EshyWMSceneLayer
{
	L_Background,
	L_Bottom,
	L_Tile,
	L_Float,
	L_Top,
	L_Overlay,
	L_NUM_LAYERS
};

enum EshyWMCursorMode
{
	ESHYWM_CURSOR_PASSTHROUGH,
	ESHYWM_CURSOR_MOVE,
	ESHYWM_CURSOR_RESIZE,
};

extern void SharedMemoryUpdated(const std::string& CurrentShm);

class EshyWMServer
{
public:

	EshyWMServer();

	struct wl_display* WlDisplay;
	struct wlr_backend* Backend;
	struct wlr_renderer* Renderer;
	struct wlr_allocator* Allocator;
	struct wlr_scene* Scene;
	struct wlr_scene_output_layout* SceneLayout;
	struct wlr_xwayland* XWayland;

	struct wlr_xdg_shell* XdgShell;
	struct wlr_layer_shell* LayerShell;
	struct wl_listener NewXdgSurfaceListener;
	struct wl_listener NewXWaylandSurfaceListener;
	struct wl_listener XWaylandReadyListener;
	std::vector<class EshyWMWindowBase*> WindowList;

	struct wlr_cursor* Cursor;
	struct wlr_xcursor_manager* CursorMgr;
	struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;
	enum EshyWMCursorMode CursorMode;

	struct wlr_seat* Seat;
	struct wl_listener new_input;
	struct wl_listener request_cursor;
	struct wl_listener request_set_selection;
	std::vector<class EshyWMKeyboard*> KeyboardList;

	class EshyWMWindowBase* FocusedWindow;
	double grab_x;
	double grab_y;
	struct wlr_box GrabGeobox;
	uint32_t ResizeEdges;
	bool bWindowModifierKeyPressed;
	uint NextWindowIndex;

	struct wlr_scene_tree* Layers[L_NUM_LAYERS];

	struct wlr_output_layout* OutputLayout;
	struct wl_listener NewOutput;
	struct wl_listener OutputChange;
	std::vector<class EshyWMOutput*> OutputList;

	class EshyWMSpecialWindow* Eshybar;

    void BeginEventLoop();
    void Shutdown();

	void CloseWindow(EshyWMWindowBase* window);

    void ResetCursorMode();
};