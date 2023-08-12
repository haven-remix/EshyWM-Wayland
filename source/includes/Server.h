
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

#include <nlohmann/json.hpp>

extern class EshyWMServer* Server;

enum EshyWMCursorMode
{
	ESHYWM_CURSOR_PASSTHROUGH,
	ESHYWM_CURSOR_MOVE,
	ESHYWM_CURSOR_RESIZE,
};

class EshyWMServer
{
public:

	EshyWMServer()
		: b_window_modifier_key_pressed(false)
		, b_window_switch_key_pressed(false)
		, next_window_index(0)
	{}

	struct wl_display* wl_display;
	struct wlr_backend* backend;
	struct wlr_renderer* renderer;
	struct wlr_allocator* allocator;
	struct wlr_scene* scene;

	struct wlr_xdg_shell* xdg_shell;
	struct wl_listener new_xdg_surface;
	std::vector<class EshyWMWindow*> window_list;

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
	std::vector<class EshyWMKeyboard*> keyboard_list;
	enum EshyWMCursorMode cursor_mode;
	class EshyWMWindow* focused_window;
	double grab_x;
	double grab_y;
	struct wlr_box grab_geobox;
	uint32_t resize_edges;
	bool b_window_modifier_key_pressed;
	bool b_window_switch_key_pressed;
	uint next_window_index;

	struct wlr_output_layout* output_layout;
	struct wl_listener output_change;
	std::vector<class EshyWMOutput*> output_list;
	struct wl_listener new_output;

	class EshyWMSpecialWindow* Eshybar;

    void Initialize();
    void BeginEventLoop(char* startup_cmd);
    void Shutdown();

	void CloseWindow(EshyWMWindow* window);

    void ResetCursorMode();
};