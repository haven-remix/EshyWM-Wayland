
#pragma once

#include "Server.h"
#include "Shared.h"

enum EshyWMWindowType
{
	WT_XDGShell,
	WT_LayerShell,
	WT_X11Managed,
	WT_X11Unmanaged
};

enum BORDER_SIDE
{
	BS_TOP 		= 0,
	BS_BOTTOM 	= 1,
	BS_LEFT 	= 2,
	BS_RIGHT 	= 3
};

extern EshyWMWindowBase* DesktopWindowAt(double lx, double ly, struct wlr_surface** surface, double* sx, double* sy);

class EshyWMWindowBase
{
public:

	EshyWMWindowBase()
		: WindowState(ESHYWM_WINDOW_STATE_NORMAL)
		, SavedGeo({0, 0, 0, 0})
	{}

	struct wlr_scene_tree* Scene;
	struct wlr_scene_tree* SceneTree;
	struct wlr_scene_rect* Border[4];
	
	struct wl_listener MapListener;
	struct wl_listener UnmapListener;
	struct wl_listener DestroyListener;
	struct wl_listener RequestMoveListener;
	struct wl_listener RequestResizeListener;
	struct wl_listener RequestMaximizeListener;
	struct wl_listener RequestFullscreenListener;
	struct wl_listener SetTitleListener;

	EEshyWMWindowState WindowState;
	wlr_box SavedGeo;
	wlr_box WindowGeometry;

	EshyWMWindowType WindowType;

    virtual void FocusWindow();
	virtual void UnfocusWindow();

    virtual void BeginInteractive(enum EshyWMCursorMode mode, uint32_t edges) {}
    virtual void ProcessCursorMove(uint32_t time) {}
    virtual void ProcessCursorResize(uint32_t time) {}

	void CreateBorder();
	void DestroyBorder();
	void UpdateBorder();

	virtual void UpdateWindowGeometry() {}

	virtual void MinimizeWindow(bool b_minimize) {}
	virtual void MaximizeWindow(bool b_maximize) {}
	virtual void FullscreenWindow(bool b_fullscreen) {}
	void ToggleMinimize() {MinimizeWindow(!(WindowState == ESHYWM_WINDOW_STATE_MINIMIZED));}
	void ToggleMaximize() {MaximizeWindow(!(WindowState == ESHYWM_WINDOW_STATE_MAXIMIZED));}
	void ToggleFullscreen() {FullscreenWindow(!(WindowState == ESHYWM_WINDOW_STATE_FULLSCREEN));}
};

class EshyWMWindow : public EshyWMWindowBase
{
public:

	EshyWMWindow(struct wlr_xdg_surface* XdgSurface);

	struct wlr_xdg_toplevel* XdgToplevel;

	struct wl_listener SetAppIdListener;

    virtual void FocusWindow() override;
	virtual void UnfocusWindow() override;

	virtual void UpdateWindowGeometry() override;

	virtual void BeginInteractive(enum EshyWMCursorMode mode, uint32_t edges) override;
	virtual void ProcessCursorMove(uint32_t time) override;
    virtual void ProcessCursorResize(uint32_t time) override;

	virtual void MinimizeWindow(bool b_minimize) override;
	virtual void MaximizeWindow(bool b_maximize) override;
	virtual void FullscreenWindow(bool b_fullscreen) override;
};

class EshyWMXWindow : public EshyWMWindowBase
{
public:

	EshyWMXWindow(struct wlr_xwayland_surface* XSurface);

	struct wlr_xwayland_surface* XWaylandSurface;

	struct wl_listener XAssociateListener;
	struct wl_listener XDissociateListener;
	struct wl_listener XCommitListener;
	struct wl_listener XActivateListener;
	struct wl_listener XConfigureListener;
	struct wl_listener XSetHintsListener;

	virtual void FocusWindow() override;
	virtual void UnfocusWindow() override;

	virtual void UpdateWindowGeometry() override;

	virtual void BeginInteractive(enum EshyWMCursorMode mode, uint32_t edges) override;
	virtual void ProcessCursorMove(uint32_t time) override;
    virtual void ProcessCursorResize(uint32_t time) override;
};