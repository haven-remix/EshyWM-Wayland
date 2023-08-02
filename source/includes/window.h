
#pragma once

#include "server.h"

enum eshywm_window_state
{
	ESHYWM_WINDOW_STATE_NORMAL,
	ESHYWM_WINDOW_STATE_MINIMIZED,
	ESHYWM_WINDOW_STATE_MAXIMIZED,
	ESHYWM_WINDOW_STATE_FULLSCREEN
};

class eshywm_window
{
public:

	eshywm_window()
		: window_state(ESHYWM_WINDOW_STATE_NORMAL)
		, saved_geo({0, 0, 0, 0})
	{}

	struct wlr_xdg_toplevel* xdg_toplevel;
	struct wlr_scene_tree* scene_tree;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;

	eshywm_window_state window_state;
	wlr_box saved_geo;

	void initialize(struct wlr_xdg_surface* xdg_surface);
    void focus_window(struct wlr_surface* surface);

    void begin_interactive(enum eshywm_cursor_mode mode, uint32_t edges);
    void process_cursor_move(uint32_t time);
    void process_cursor_resize(uint32_t time);

	void maximize_window(bool b_maximize);
	void fullscreen_window(bool b_fullscreen);
	void toggle_maximize() {maximize_window(!(window_state == ESHYWM_WINDOW_STATE_MAXIMIZED));}
	void toggle_fullscreen() {fullscreen_window(!(window_state == ESHYWM_WINDOW_STATE_FULLSCREEN));}
};