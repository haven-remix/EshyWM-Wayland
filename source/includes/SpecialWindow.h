
#pragma once

#include "Server.h"

class EshyWMSpecialWindow
{
public:

    EshyWMSpecialWindow(struct wlr_xdg_surface* xdg_surface);
    struct wlr_xdg_toplevel* XdgToplevel;
	struct wlr_scene_tree* SceneTree;
};