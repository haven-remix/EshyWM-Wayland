
#pragma once

#include <wayland-server-core.h>

extern void KeyboardHandleModifiers(struct wl_listener* listener, void* data);
extern void KeyboardHandleKey(struct wl_listener* listener, void* data);
extern void KeyboardHandleDestroy(struct wl_listener* listener, void* data);

class EshyWMKeyboard
{
public:

	EshyWMKeyboard() {}
	EshyWMKeyboard(struct wlr_keyboard* _wlr_keyboard)
		: WlrKeyboard(_wlr_keyboard)
	{}

	struct wlr_keyboard* WlrKeyboard;

	struct wl_listener ModifiersListener;
	struct wl_listener KeyListener;
	struct wl_listener DestroyListener;
};