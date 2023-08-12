
#include "SpecialWindow.h"

void EshyWMSpecialWindow::Initialize(struct wlr_xdg_surface* xdg_surface)
{
    XdgToplevel = xdg_surface->toplevel;
    SceneTree = wlr_scene_xdg_surface_create(&Server->scene->tree, XdgToplevel->base);
    SceneTree->node.data = this;
    xdg_surface->data = SceneTree;
}