
#pragma once

#include <iostream>

#include <wayland-server-core.h>

#define check(condition, message)        if (!(condition)) {std::cout << message << std::endl; abort();}

static void add_listener(struct wl_listener* listener, wl_notify_func_t callback, struct wl_signal* signal)
{
    listener->notify = callback;
    wl_signal_add(signal, listener);
}