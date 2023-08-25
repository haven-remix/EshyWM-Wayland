
#include "Server.h"
#include "EshyWM.h"
#include "Window.h"
#include "SpecialWindow.h"
#include "Keyboard.h"
#include "Output.h"
#include "Config.h"
#include "Util.h"

#include "EshyIPC.h"

#include <nlohmann/json.hpp>

#include <linux/input-event-codes.h>

#define static
#define class wlr

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

#include <wlr/xwayland.h>
#include <X11/Xlib.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb.h>
}

#undef static
#undef class

EshyWMServer* Server = nullptr;

enum NetWMWindowType
{
	WT_Dialog,
	WT_Splash,
	WT_Toolbar,
	WT_Utility,
	NET_LAST
};

static Atom NetAtom[NET_LAST];

static void XWaylandReady(struct wl_listener* listener, void* data);

static void ServerNewInput(struct wl_listener* listener, void* data);
static void ServerNewOutput(struct wl_listener* listener, void* data);

static void ServerCursorMotion(struct wl_listener* listener, void* data);
static void ServerCursorMotionAbsolute(struct wl_listener* listener, void* data);
static void ServerCursorButton(struct wl_listener* listener, void* data);
static void ServerCursorAxis(struct wl_listener* listener, void* data);
static void ServerCursorFrame(struct wl_listener* listener, void* data);

static void ServerOutputChange(struct wl_listener* listener, void* data);
static void ServerNewXdgSurface(struct wl_listener* listener, void* data);
static void NewXWaylandSurface(struct wl_listener* listener, void* data);

static void SeatRequestCursor(struct wl_listener* listener, void* data);
static void SeatRequestSetSelection(struct wl_listener* listener, void* data);

void SharedMemoryUpdated(const std::string& CurrentShm)
{
	nlohmann::json Data = nlohmann::json::parse(CurrentShm);
	if (Data["sender_client"] == CLIENT_ESHYBAR)
	{
		if (Data["action"] == ACTION_FOCUS_WINDOW)
		{
			if(EshyWMWindow* pointer = (EshyWMWindow*)((uint64_t)Data["window_id"]))
				pointer->FocusWindow();
		}
		else if (Data["action"] == ACTION_UNFOCUS_WINDOW)
		{
			if(EshyWMWindow* pointer = (EshyWMWindow*)((uint64_t)Data["window_id"]))
				pointer->FocusWindow();
		}
		else if (Data["action"] == ACTION_MINIMIZE_WINDOW)
		{
			if(EshyWMWindow* pointer = (EshyWMWindow*)((uint64_t)Data["window_id"]))
				pointer->MinimizeWindow(true);
		}
	}
}

EshyWMServer::EshyWMServer()
	: bWindowModifierKeyPressed(false)
	, NextWindowIndex(0)
	, CursorMode(ESHYWM_CURSOR_PASSTHROUGH)
	, FocusedWindow(nullptr)
	, Eshybar(nullptr)
{
	WlDisplay = wl_display_create();
	Backend = wlr_backend_autocreate(WlDisplay, NULL);
    check(Backend, "Failed to create wlr_backend");
	Renderer = wlr_renderer_autocreate(Backend);
    check(Renderer, "Failed to create wlr_renderer");
	wlr_renderer_init_wl_display(Renderer, WlDisplay);
	Allocator = wlr_allocator_autocreate(Backend, Renderer);
    check(Allocator, "Failed to create wlr_allocator");

	struct wlr_compositor* WlrCompositor = wlr_compositor_create(WlDisplay, 5, Renderer);
	wlr_subcompositor_create(WlDisplay);
	wlr_data_device_manager_create(WlDisplay);

	//Setup xwayland X server
	XWayland = wlr_xwayland_create(WlDisplay, WlrCompositor, false);
	add_listener(&XWaylandReadyListener, XWaylandReady, &XWayland->events.ready);
	add_listener(&NewXWaylandSurfaceListener, NewXWaylandSurface, &XWayland->events.new_surface);
	setenv("DISPLAY", XWayland->display_name, true);

	OutputLayout = wlr_output_layout_create();
	add_listener(&NewOutput, ServerNewOutput, &Backend->events.new_output);
	add_listener(&OutputChange, ServerOutputChange, &OutputLayout->events.change);

	//Initialize the scene graph used to lay out windows
	Scene = wlr_scene_create();
	SceneLayout = wlr_scene_attach_output_layout(Scene, OutputLayout);

	for(int i = 0; i < L_NUM_LAYERS; i++)
		Layers[i] = wlr_scene_tree_create(&Scene->tree);

	XdgShell = wlr_xdg_shell_create(WlDisplay, 3);
	add_listener(&NewXdgSurfaceListener, ServerNewXdgSurface, &XdgShell->events.new_surface);

	Cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(Cursor, OutputLayout);
	CursorMgr = wlr_xcursor_manager_create(NULL, 24);
	add_listener(&cursor_motion, ServerCursorMotion, &Cursor->events.motion);
	add_listener(&cursor_motion_absolute, ServerCursorMotionAbsolute, &Cursor->events.motion_absolute);
	add_listener(&cursor_button, ServerCursorButton, &Cursor->events.button);
	add_listener(&cursor_axis, ServerCursorAxis, &Cursor->events.axis);
	add_listener(&cursor_frame, ServerCursorFrame, &Cursor->events.frame);

	add_listener(&new_input, ServerNewInput, &Backend->events.new_input);
	Seat = wlr_seat_create(WlDisplay, "seat0");
	add_listener(&request_cursor, SeatRequestCursor, &Seat->events.request_set_cursor);
	add_listener(&request_set_selection, SeatRequestSetSelection, &Seat->events.request_set_selection);
}

void EshyWMServer::BeginEventLoop()
{
    //Add a Unix socket to the Wayland display
	const char* socket = wl_display_add_socket_auto(WlDisplay);
	if (!socket)
	{
		wlr_backend_destroy(Backend);
		return;
	}

	if (!wlr_backend_start(Backend))
	{
		wlr_backend_destroy(Backend);
		wl_display_destroy(WlDisplay);
		return;
	}

	setenv("WAYLAND_DISPLAY", socket, true);

	// if(fork() == 0)
	// {
	// 	int width;
	// 	int height;
	// 	wlr_output_effective_resolution(Server->OutputList[0]->WlrOutput, &width, &height);

	// 	execl("eshybar", "eshybar", std::to_string(EshybarShmID).c_str(), std::to_string(width).c_str(), std::to_string(height).c_str(), (void*)NULL);
	// }

	//Excute startup commands
	for(const std::string& command : EshyWMConfig::GetStartupCommands())
        system(command.c_str());

	wl_display_run(WlDisplay);
}

void EshyWMServer::Shutdown()
{
	wlr_xwayland_destroy(XWayland);
    wl_display_destroy_clients(WlDisplay);
	wlr_scene_node_destroy(&Scene->tree.node);
	wlr_xcursor_manager_destroy(CursorMgr);
	wlr_output_layout_destroy(OutputLayout);
	wl_display_destroy(WlDisplay);
}


void EshyWMServer::CloseWindow(EshyWMWindowBase* window)
{
	if(window->WindowType == WT_XDGShell)
		wlr_xdg_toplevel_send_close(((EshyWMWindow*)window)->XdgToplevel);
}


void EshyWMServer::ResetCursorMode()
{
	CursorMode = ESHYWM_CURSOR_PASSTHROUGH;
}


static Atom GetAtom(xcb_connection_t* XC, const char* data)
{
	Atom Atom = 0;
	xcb_intern_atom_reply_t* Reply;
	xcb_intern_atom_cookie_t Cookie = xcb_intern_atom(XC, 0, strlen(data), data);

	if(Reply = xcb_intern_atom_reply(XC, Cookie, NULL))
		Atom = Reply->atom;

	free(Reply);
	return Atom;
}

void XWaylandReady(struct wl_listener* listener, void* data)
{
	struct wlr_xcursor* XCursor;
	xcb_connection_t* XC = xcb_connect(Server->XWayland->display_name, NULL);
	int Err = xcb_connection_has_error(XC);

	//Collect atoms we are interested in. If getatom returns 0, we will not detect that window type
	NetAtom[WT_Dialog] = GetAtom(XC, "_NET_WM_WINDOW_TYPE_DIALOG");
	NetAtom[WT_Splash] = GetAtom(XC, "_NET_WM_WINDOW_TYPE_SPLASH");
	NetAtom[WT_Toolbar] = GetAtom(XC, "_NET_WM_WINDOW_TYPE_TOOLBAR");
	NetAtom[WT_Utility] = GetAtom(XC, "_NET_WM_WINDOW_TYPE_UTILITY");

	wlr_xwayland_set_seat(Server->XWayland, Server->Seat);

	//Set the default XWayland cursor to match the rest of eshywm
	if(XCursor = wlr_xcursor_manager_get_xcursor(Server->CursorMgr, "left_ptr", 1))
		wlr_xwayland_set_cursor(Server->XWayland, XCursor->images[0]->buffer, XCursor->images[0]->width * 4, XCursor->images[0]->width, XCursor->images[0]->height, XCursor->images[0]->hotspot_x, XCursor->images[0]->hotspot_y);
	
	xcb_disconnect(XC);
}


static void ServerNewKeyboard(struct wlr_input_device* device)
{
	struct wlr_keyboard* wlr_keyboard = wlr_keyboard_from_input_device(device);

	EshyWMKeyboard* keyboard = new EshyWMKeyboard(wlr_keyboard);

	/*We need to prepare an XKB keymap and assign it to the keyboard. This
	*  assumes the defaults (e.g. layout = "us").*/
	struct xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap* keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(wlr_keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

	/*Here we set up listeners for keyboard events.*/
	add_listener(&keyboard->ModifiersListener, KeyboardHandleModifiers, &wlr_keyboard->events.modifiers);
	add_listener(&keyboard->KeyListener, KeyboardHandleKey, &wlr_keyboard->events.key);
	add_listener(&keyboard->DestroyListener, KeyboardHandleDestroy, &device->events.destroy);

	wlr_seat_set_keyboard(Server->Seat, keyboard->WlrKeyboard);

	/*And add the keyboard to our list of keyboards*/
	Server->KeyboardList.push_back(keyboard);
}

static void ServerNewPointer(struct wlr_input_device* device)
{
	/*We don't do anything special with pointers. All of our pointer handling
	*  is proxied through wlr_cursor. On another compositor, you might take this
	*  opportunity to do libinput configuration on the device to set
	*  acceleration, etc.*/
	wlr_cursor_attach_input_device(Server->Cursor, device);
}

void ServerNewInput(struct wl_listener* listener, void* data)
{
	struct wlr_input_device* device = (wlr_input_device*)data;

	switch (device->type)
	{
	case WLR_INPUT_DEVICE_KEYBOARD:
		ServerNewKeyboard(device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
		ServerNewPointer(device);
		break;
	default:
		break;
	}

	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (Server->KeyboardList.size() > 0)
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;

	wlr_seat_set_capabilities(Server->Seat, caps);
}

void ServerNewOutput(struct wl_listener* listener, void* data)
{
	struct wlr_output* wlr_output = (struct wlr_output*)data;

	//If monitor exists in configuration then retrieve data
	EshyWMMonitorInfo OutputInfo;
	for(const EshyWMMonitorInfo& Info : EshyWMConfig::GetMonitorInfoList())
		if(Info.Name == std::string(wlr_output->name))
		{
			OutputInfo = Info;
			break;
		}

	/*Configures the output created by the backend to use our allocator
	*  and our renderer. Must be done once, before commiting the output*/
	wlr_output_init_render(wlr_output, Server->Allocator, Server->Renderer);

	//The output may be disabled, switch it on
	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, true);

	//Select monitor's preferred mode
	if (struct wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output))
		wlr_output_state_set_mode(&state, mode);

	//Atomically applies the new output state
	wlr_output_commit_state(wlr_output, &state);
	wlr_output_state_finish(&state);

	//Allocates and configures our state for this output
	EshyWMOutput* output = new EshyWMOutput(wlr_output);
	add_listener(&output->FrameListener, OutputFrame, &wlr_output->events.frame);
	add_listener(&output->RequestStateListener, OutputRequestState, &wlr_output->events.request_state);
	add_listener(&output->DestroyListener, OutputDestroy, &wlr_output->events.destroy);
	Server->OutputList.push_back(output);

	//Add the output to the output layout arranged as specified in configuration. If no specification exists, then arragement is left to right.
	if(OutputInfo.Name != "")
	{
		struct wlr_output_layout_output* layout_output = wlr_output_layout_add(Server->OutputLayout, wlr_output, OutputInfo.OffsetX, OutputInfo.OffsetY);
		struct wlr_scene_output* scene_output = wlr_scene_output_create(Server->Scene, wlr_output);
		wlr_scene_output_layout_add_output(Server->SceneLayout, layout_output, scene_output);
	}
	else
	{
		int OffsetX = 0;
		for(const EshyWMOutput* Output : Server->OutputList)
			OffsetX += Output->WlrOutput->width;
		OffsetX -= Server->OutputList[Server->OutputList.size() - 1]->WlrOutput->width;

		struct wlr_output_layout_output* layout_output = wlr_output_layout_add(Server->OutputLayout, wlr_output, OffsetX, 0);
		struct wlr_scene_output* scene_output = wlr_scene_output_create(Server->Scene, wlr_output);
		wlr_scene_output_layout_add_output(Server->SceneLayout, layout_output, scene_output);
	}
}


static void ProcessCursorMotion(uint32_t time)
{
	//If the mode is non-passthrough, delegate to those functions
	if (Server->CursorMode == ESHYWM_CURSOR_MOVE)
	{
		Server->FocusedWindow->ProcessCursorMove(time);
		return;
	}
	else if (Server->CursorMode == ESHYWM_CURSOR_RESIZE)
	{
		Server->FocusedWindow->ProcessCursorResize(time);
		return;
	}

	//Otherwise, find the window under the pointer and send the event along
	double sx, sy;
	struct wlr_seat* seat = Server->Seat;
	struct wlr_surface* surface = NULL;
	EshyWMWindowBase* window = DesktopWindowAt(Server->Cursor->x, Server->Cursor->y, &surface, &sx, &sy);

	if (!window)
		wlr_cursor_set_xcursor(Server->Cursor, Server->CursorMgr, "default");

	if (surface)
	{
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	}
	else
		//Clear pointer focus so future button events and such are not sent to the last client to have the cursor over it
		wlr_seat_pointer_clear_focus(seat);
}

void ServerCursorMotion(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_motion_event* event = (wlr_pointer_motion_event*)data;
	wlr_cursor_move(Server->Cursor, &event->pointer->base, event->delta_x, event->delta_y);
	ProcessCursorMotion(event->time_msec);
}

void ServerCursorMotionAbsolute(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_motion_absolute_event* event = (wlr_pointer_motion_absolute_event*)data;
	wlr_cursor_warp_absolute(Server->Cursor, &event->pointer->base, event->x, event->y);
	ProcessCursorMotion(event->time_msec);
}

void ServerCursorButton(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_button_event* event = (wlr_pointer_button_event*)data;

	double sx;
	double sy;
	struct wlr_surface* surface = NULL;
	EshyWMWindowBase* window = DesktopWindowAt(Server->Cursor->x, Server->Cursor->y, &surface, &sx, &sy);

	if(window && window->WindowType == WT_XDGShell && ((EshyWMWindow*)window)->XdgToplevel->app_id == "eshybar")
		return;

	if (event->state == WLR_BUTTON_RELEASED)
		Server->ResetCursorMode();
	else if (window)
		window->FocusWindow();
	else if (!window && Server->FocusedWindow)
	{
		Server->FocusedWindow->UnfocusWindow();
		Server->FocusedWindow = nullptr;
	}

	if(Server->bWindowModifierKeyPressed && event->state == WLR_BUTTON_PRESSED && window)
	{
		if (event->button == BTN_LEFT)
		{
			window->BeginInteractive(ESHYWM_CURSOR_MOVE, 0);
			return;
		}
		else if (event->button == BTN_RIGHT)
		{
			window->BeginInteractive(ESHYWM_CURSOR_RESIZE, 0);
			return;
		}
	}

	wlr_seat_pointer_notify_button(Server->Seat, event->time_msec, event->button, event->state);
}

void ServerCursorAxis(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_axis_event* event = (wlr_pointer_axis_event*)data;
	wlr_seat_pointer_notify_axis(Server->Seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source);
}

void ServerCursorFrame(struct wl_listener* listener, void* data)
{
	wlr_seat_pointer_notify_frame(Server->Seat);
}


void ServerOutputChange(struct wl_listener* listener, void* data)
{
	struct wlr_output_layout_output* event = (wlr_output_layout_output*)data;

	if(!Server->Eshybar)
		return;

	int Width;
	int Height;
	wlr_output_effective_resolution(Server->OutputList[0]->WlrOutput, &Width, &Height);
	wlr_scene_node_set_position(&Server->Eshybar->SceneTree->node, 0, Height - 50);

	nlohmann::json ConfigureEshybarInfo;
	ConfigureEshybarInfo["action"] = ACTION_CONFIGURE_ESHYBAR;
	ConfigureEshybarInfo["sender_client"] = CLIENT_COMPOSITOR;
	ConfigureEshybarInfo["width"] = Width;
	ConfigureEshybarInfo["height"] = Height;
	EshyIPC::InsertIntoMemory(EshybarShmID, ConfigureEshybarInfo.dump());
}

void ServerNewXdgSurface(struct wl_listener* listener, void* data)
{
	struct wlr_xdg_surface* xdg_surface = (wlr_xdg_surface*)data;

	if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP)
	{
		struct wlr_xdg_surface* parent = wlr_xdg_surface_try_from_wlr_surface(xdg_surface->popup->parent);
		assert(parent);
		struct wlr_scene_tree* parent_tree = (wlr_scene_tree*)parent->data;
		xdg_surface->data = wlr_scene_xdg_surface_create(parent_tree, xdg_surface);

		//Center the popup
		struct wlr_box geo_box;
		wlr_xdg_surface_get_geometry(parent, &geo_box);
		const int XPosition = (Server->OutputList[0]->WlrOutput->width / 2) - (geo_box.width / 2);
		const int YPosition = (Server->OutputList[0]->WlrOutput->height / 2) - (geo_box.height / 2);
		wlr_scene_node_set_position(&parent_tree->node, XPosition, YPosition);
	}
	else if (xdg_surface->toplevel->title && std::string(xdg_surface->toplevel->title) == std::string("Eshybar"))
	{
		EshyWMSpecialWindow* Eshybar = new EshyWMSpecialWindow(xdg_surface);
		Eshybar->XdgToplevel->app_id = (char*)"eshybar";

		Server->Eshybar = Eshybar;

		int width;
		int height;
		wlr_output_effective_resolution(Server->OutputList[0]->WlrOutput, &width, &height);
		wlr_scene_node_set_position(&Eshybar->SceneTree->node, 0, height - 50);

		//Send a message for all windows that exist at the time the bar is created
		for (EshyWMWindowBase* Window : Server->WindowList)
		{
#define class wlr
			const std::string AppID = Window->WindowType == WT_XDGShell ? (((EshyWMWindow*)Window)->XdgToplevel->app_id ? ((EshyWMWindow*)Window)->XdgToplevel->app_id : "NO_APP_ID") : Window->WindowType == WT_X11Managed ? (((EshyWMXWindow*)Window)->XWaylandSurface->class ? ((EshyWMXWindow*)Window)->XWaylandSurface->class : "NO_APP_CLASS") : "NO_APP_CLASS";
			const std::string Title = Window->WindowType == WT_XDGShell ? (((EshyWMWindow*)Window)->XdgToplevel->title ? ((EshyWMWindow*)Window)->XdgToplevel->title : "NO_TITLE") : Window->WindowType == WT_X11Managed ? (((EshyWMXWindow*)Window)->XWaylandSurface->title ? ((EshyWMXWindow*)Window)->XWaylandSurface->title : "NO_TITLE") : "NO_TITLE";
#undef class
			nlohmann::json WindowAddInfo;
			WindowAddInfo["action"] = ACTION_ADD_WINDOW;
			WindowAddInfo["sender_client"] = CLIENT_COMPOSITOR;
			WindowAddInfo["window_id"] = (uint64_t)Window;
			WindowAddInfo["app_id"] = AppID;
			WindowAddInfo["title"] = Title;
			EshyIPC::InsertIntoMemory(EshybarShmID, WindowAddInfo.dump());
		}
	}
	else
	{
		EshyWMWindow* Window = new EshyWMWindow(xdg_surface);
		Server->WindowList.push_back(Window);

		nlohmann::json WindowAddInfo;
		WindowAddInfo["action"] = ACTION_ADD_WINDOW;
		WindowAddInfo["sender_client"] = CLIENT_COMPOSITOR;
		WindowAddInfo["window_id"] = (uint64_t)Window;
		WindowAddInfo["app_id"] = Window->XdgToplevel->app_id ? Window->XdgToplevel->app_id : "NO_APP_ID";
		WindowAddInfo["title"] = Window->XdgToplevel->title ? Window->XdgToplevel->title : "NO_TITLE";
		EshyIPC::InsertIntoMemory(EshybarShmID, WindowAddInfo.dump());
	}
}

void NewXWaylandSurface(struct wl_listener* listener, void* data)
{
	struct wlr_xwayland_surface* XSurface = (wlr_xwayland_surface*)data;

	EshyWMXWindow* Window = new EshyWMXWindow(XSurface);
	Server->WindowList.push_back(Window);

	nlohmann::json WindowAddInfo;
	WindowAddInfo["action"] = ACTION_ADD_WINDOW;
	WindowAddInfo["sender_client"] = CLIENT_COMPOSITOR;
	WindowAddInfo["window_id"] = (uint64_t)Window;
#define class wlr
	WindowAddInfo["app_id"] = Window->XWaylandSurface->class ? Window->XWaylandSurface->class : "NO_APP_CLASS";
#undef class
	WindowAddInfo["title"] = Window->XWaylandSurface->title ? Window->XWaylandSurface->title : "NO_TITLE";
	EshyIPC::InsertIntoMemory(EshybarShmID, WindowAddInfo.dump());
}


void SeatRequestCursor(struct wl_listener* listener, void* data)
{
	//This event is raised by the seat when a client provides a cursor image
	struct wlr_seat_pointer_request_set_cursor_event* event = (wlr_seat_pointer_request_set_cursor_event*)data;
	struct wlr_seat_client* focused_client = Server->Seat->pointer_state.focused_client;

	//This can be sent by any client, so we check to make sure this one is actually has pointer focus first
	if (focused_client == event->seat_client)
	{
		/*Once we've vetted the client, we can tell the cursor to use the
		*  provided surface as the cursor image. It will set the hardware cursor
		*  on the output that it's currently on and continue to do so as the
		*  cursor moves between outputs.*/
		wlr_cursor_set_surface(Server->Cursor, event->surface, event->hotspot_x, event->hotspot_y);
	}
}

void SeatRequestSetSelection(struct wl_listener* listener, void* data)
{
	struct wlr_seat_request_set_selection_event* event = (wlr_seat_request_set_selection_event*)data;
	wlr_seat_set_selection(Server->Seat, event->source, event->serial);
}