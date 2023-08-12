
#pragma once

#include <wayland-server-core.h>

extern void OutputFrame(struct wl_listener* listener, void* data);
extern void OutputRequestState(struct wl_listener* listener, void* data);
extern void OutputDestroy(struct wl_listener* listener, void* data);

class EshyWMOutput
{
public:

	struct wlr_output* WlrOutput;
	struct wl_listener FrameListener;
	struct wl_listener RequestStateListener;
	struct wl_listener DestroyListener;
};