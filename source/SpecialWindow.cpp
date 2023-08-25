
#include "SpecialWindow.h"

#define static

extern "C"
{
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
}

#define static

EshyWMSpecialWindow::EshyWMSpecialWindow(struct wlr_xdg_surface* xdg_surface)
{
    XdgToplevel = xdg_surface->toplevel;
    SceneTree = wlr_scene_xdg_surface_create(&Server->Scene->tree, XdgToplevel->base);
    SceneTree->node.data = this;
    xdg_surface->data = SceneTree;
}