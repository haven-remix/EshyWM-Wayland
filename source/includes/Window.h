
#pragma once

#include "Server.h"
#include "Shared.h"

extern EshyWMWindow* DesktopWindowAt(double lx, double ly, struct wlr_surface** surface, double* sx, double* sy);

class EshyWMWindow
{
public:

	EshyWMWindow()
		: WindowState(ESHYWM_WINDOW_STATE_NORMAL)
		, SavedGeo({0, 0, 0, 0})
	{}

	struct wlr_xdg_toplevel* XdgToplevel;
	struct wlr_scene_tree* SceneTree;
	struct wl_listener MapListener;
	struct wl_listener UnmapListener;
	struct wl_listener DestroyListener;
	struct wl_listener RequestMoveListener;
	struct wl_listener RequestResizeListener;
	struct wl_listener RequestMaximizeListener;
	struct wl_listener RequestFullscreenListener;
	struct wl_listener SetTitleListener;
	struct wl_listener SetAppIdListener;

	EEshyWMWindowState WindowState;
	wlr_box SavedGeo;

	void Initialize(struct wlr_xdg_surface* xdg_surface);
    void FocusWindow(struct wlr_surface* surface);

    void BeginInteractive(enum EshyWMCursorMode mode, uint32_t edges);
    void ProcessCursorMove(uint32_t time);
    void ProvessCursorResize(uint32_t time);

	void MinimizeWindow(bool b_minimize);
	void MaximizeWindow(bool b_maximize);
	void FullscreenWindow(bool b_fullscreen);
	void ToggleMinimize() {MinimizeWindow(!(WindowState == ESHYWM_WINDOW_STATE_MINIMIZED));}
	void ToggleMaximize() {MaximizeWindow(!(WindowState == ESHYWM_WINDOW_STATE_MAXIMIZED));}
	void ToggleFullscreen() {FullscreenWindow(!(WindowState == ESHYWM_WINDOW_STATE_FULLSCREEN));}
};