
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

extern class eshywm_server* server;

extern class eshywm_window* desktop_window_at(double lx, double ly, struct wlr_surface* *surface, double* sx, double* sy);

extern void output_frame(struct wl_listener* listener, void* data);
extern void output_request_state(struct wl_listener* listener, void* data);
extern void output_destroy(struct wl_listener* listener, void* data);

class eshywm_output
{
public:

	struct wlr_output* wlr_output;
	struct wl_listener frame;
	struct wl_listener request_state;
	struct wl_listener destroy;
};

extern void keyboard_handle_modifiers(struct wl_listener* listener, void* data);
extern void keyboard_handle_key(struct wl_listener* listener, void* data);
extern void keyboard_handle_destroy(struct wl_listener* listener, void* data);

class eshywm_keyboard
{
public:

	eshywm_keyboard() {}
	eshywm_keyboard(struct wlr_keyboard* _wlr_keyboard)
		: wlr_keyboard(_wlr_keyboard)
	{}

	struct wlr_keyboard* wlr_keyboard;

	struct wl_listener modifiers;
	struct wl_listener key;
	struct wl_listener destroy;
};