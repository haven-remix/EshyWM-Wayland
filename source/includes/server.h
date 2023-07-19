
#pragma once

#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <memory>
#include <vector>
#include <algorithm>

#include <wayland-server-core.h>

#define static

extern "C"
{
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#undef static

#include <xkbcommon/xkbcommon.h>

enum eshywm_cursor_mode
{
	ESHYWM_CURSOR_PASSTHROUGH,
	ESHYWM_CURSOR_MOVE,
	ESHYWM_CURSOR_RESIZE,
};

class eshywm_server
{
public:

	struct wl_display* wl_display;
	struct wlr_backend* backend;
	struct wlr_renderer* renderer;
	struct wlr_allocator* allocator;
	struct wlr_scene* scene;

	struct wlr_xdg_shell* xdg_shell;
	struct wl_listener new_xdg_surface;
	std::vector<class eshywm_window*> window_list;

	struct wlr_cursor* cursor;
	struct wlr_xcursor_manager* cursor_mgr;
	struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	struct wlr_seat* seat;
	struct wl_listener new_input;
	struct wl_listener request_cursor;
	struct wl_listener request_set_selection;
	std::vector<class eshywm_keyboard*> keyboard_list;
	enum eshywm_cursor_mode cursor_mode;
	class eshywm_window* grabbed_window;
	double grab_x, grab_y;
	struct wlr_box grab_geobox;
	uint32_t resize_edges;

	struct wlr_output_layout* output_layout;
	std::vector<class eshywm_output*> output_list;
	struct wl_listener new_output;

    void initialize();
    void run_display(char* startup_cmd);
    void shutdown();

    void reset_cursor_mode();
};