
#pragma once

#include "server.h"

class eshywm_window
{
public:

	struct wlr_xdg_toplevel* xdg_toplevel;
	struct wlr_scene_tree* scene_tree;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;

	void initialize(struct wlr_xdg_surface* xdg_surface);
    void focus_window(struct wlr_surface* surface);

    void begin_interactive(enum eshywm_cursor_mode mode, uint32_t edges);
    void process_cursor_move(uint32_t time);
    void process_cursor_resize(uint32_t time);
};