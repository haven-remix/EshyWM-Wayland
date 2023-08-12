
#include "Server.h"
#include "EshyWM.h"
#include "Window.h"
#include "SpecialWindow.h"
#include "Keyboard.h"
#include "Output.h"
#include "Util.h"

#include "EshyIPC.h"
#include "Shared.h"

#include <GLFW/glfw3.h>

#include <wayland-client.h>

#include <linux/input-event-codes.h>

EshyWMServer* Server = nullptr;

static void ServerNewKeyboard(struct wlr_input_device* device);
static void ServerNewInput(struct wl_listener* listener, void* data);
static void ServerCursorMotion(struct wl_listener* listener, void* data);
static void ServerCursorMotionAbsolute(struct wl_listener* listener, void* data);
static void ServerCursorButton(struct wl_listener* listener, void* data);
static void ServerCursorAxis(struct wl_listener* listener, void* data);
static void ServerCursorFrame(struct wl_listener* listener, void* data);
static void ServerNewOutput(struct wl_listener* listener, void* data);
static void ServerOutputChange(struct wl_listener* listener, void* data);
static void ServerNewXdgSurface(struct wl_listener* listener, void* data);

static void ProcessCursorMotion(uint32_t time);

static void SeatRequestCursor(struct wl_listener* listener, void* data);
static void SeatRequestSetSelection(struct wl_listener* listener, void* data);

static void SharedMemoryUpdated(const std::string& CurrentShm)
{
	nlohmann::json Data = nlohmann::json::parse(CurrentShm);
	if (Data["sender_client"] == CLIENT_ESHYBAR)
	{
		if (Data["action"] == ACTION_FOCUS_WINDOW)
		{
			if(EshyWMWindow* pointer = (EshyWMWindow*)((uint64_t)Data["window_id"]))
				pointer->FocusWindow(pointer->XdgToplevel->base->surface);
		}
		else if (Data["action"] == ACTION_UNFOCUS_WINDOW)
		{
			if(EshyWMWindow* pointer = (EshyWMWindow*)((uint64_t)Data["window_id"]))
				pointer->FocusWindow(pointer->XdgToplevel->base->surface);
		}
		else if (Data["action"] == ACTION_MINIMIZE_WINDOW)
		{
			if(EshyWMWindow* pointer = (EshyWMWindow*)((uint64_t)Data["window_id"]))
				pointer->MinimizeWindow(true);
		}
	}
}

void EshyWMServer::Initialize()
{
    wl_display = wl_display_create();
	backend = wlr_backend_autocreate(wl_display, NULL);
    check(backend, "Failed to create wlr_backend");
	renderer = wlr_renderer_autocreate(backend);
    check(renderer, "Failed to create wlr_renderer");
	wlr_renderer_init_wl_display(renderer, wl_display);
	allocator = wlr_allocator_autocreate(backend, renderer);
    check(allocator, "Failed to create wlr_allocator");

	wlr_compositor_create(wl_display, 5, renderer);
	wlr_subcompositor_create(wl_display);
	wlr_data_device_manager_create(wl_display);

	output_layout = wlr_output_layout_create();
	add_listener(&new_output, ServerNewOutput, &backend->events.new_output);
	add_listener(&output_change, ServerOutputChange, &output_layout->events.change);

	scene = wlr_scene_create();
	wlr_scene_attach_output_layout(scene, output_layout);

	xdg_shell = wlr_xdg_shell_create(wl_display, 3);
	add_listener(&new_xdg_surface, ServerNewXdgSurface, &xdg_shell->events.new_surface);

	cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(cursor, output_layout);

	cursor_mgr = wlr_xcursor_manager_create(NULL, 24);

	cursor_mode = ESHYWM_CURSOR_PASSTHROUGH;
	add_listener(&cursor_motion, ServerCursorMotion, &cursor->events.motion);
	add_listener(&cursor_motion_absolute, ServerCursorMotionAbsolute, &cursor->events.motion_absolute);
	add_listener(&cursor_button, ServerCursorButton, &cursor->events.button);
	add_listener(&cursor_axis, ServerCursorAxis, &cursor->events.axis);
	add_listener(&cursor_frame, ServerCursorFrame, &cursor->events.frame);

	add_listener(&new_input, ServerNewInput, &backend->events.new_input);
	seat = wlr_seat_create(wl_display, "seat0");
	add_listener(&request_cursor, SeatRequestCursor, &seat->events.request_set_cursor);
	add_listener(&request_set_selection, SeatRequestSetSelection, &seat->events.request_set_selection);
}

void EshyWMServer::BeginEventLoop(char* startup_cmd)
{
    //Add a Unix socket to the Wayland display
	const char* socket = wl_display_add_socket_auto(wl_display);
	if (!socket)
	{
		wlr_backend_destroy(backend);
		return;
	}

	if (!wlr_backend_start(backend))
	{
		wlr_backend_destroy(backend);
		wl_display_destroy(wl_display);
		return;
	}

	setenv("WAYLAND_DISPLAY", socket, true);
	if (startup_cmd && fork() == 0)
		execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void*)NULL);

	if(fork() == 0)
	{
		int width;
		int height;
		wlr_output_effective_resolution(Server->output_list[0]->WlrOutput, &width, &height);

		execl("eshybar", "eshybar", std::to_string(EshybarShmID).c_str(), std::to_string(width).c_str(), std::to_string(height).c_str(), (void*)NULL);
	}

	wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s", socket);

	wl_event_loop* wl_event_loop = wl_display_get_event_loop(wl_display);

	eipcSharedMemory SharedMemory;
	std::string CurrentShm;

	while (true)
	{
		//Only runs when shared memory changes
		SharedMemory = EshyIPC::AttachSharedMemoryBlock(EshybarShmID);
		if(CurrentShm != SharedMemory.Block)
		{
			CurrentShm = SharedMemory.Block;
			SharedMemoryUpdated(CurrentShm);
		}

		wl_event_loop_dispatch(wl_event_loop, 1);
		wl_display_flush_clients(wl_display);
	}
}

void EshyWMServer::Shutdown()
{
    wl_display_destroy_clients(wl_display);
	wlr_scene_node_destroy(&scene->tree.node);
	wlr_xcursor_manager_destroy(cursor_mgr);
	wlr_output_layout_destroy(output_layout);
	wl_display_destroy(wl_display);
}


void EshyWMServer::CloseWindow(EshyWMWindow* window)
{
	wlr_xdg_toplevel_send_close(window->XdgToplevel);
}


void EshyWMServer::ResetCursorMode()
{
	cursor_mode = ESHYWM_CURSOR_PASSTHROUGH;
}


void ServerNewKeyboard(struct wlr_input_device* device)
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

	wlr_seat_set_keyboard(Server->seat, keyboard->WlrKeyboard);

	/*And add the keyboard to our list of keyboards*/
	Server->keyboard_list.push_back(keyboard);
}

void ServerNewPointer(struct wlr_input_device* device)
{
	/*We don't do anything special with pointers. All of our pointer handling
	*  is proxied through wlr_cursor. On another compositor, you might take this
	*  opportunity to do libinput configuration on the device to set
	*  acceleration, etc.*/
	wlr_cursor_attach_input_device(Server->cursor, device);
}

void ServerNewInput(struct wl_listener* listener, void* data)
{
	/*This event is raised by the backend when a new input device becomes
	*  available.*/
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
	/*We need to let the wlr_seat know what our capabilities are, which is
	*  communiciated to the client. In eshywm we always have a cursor, even if
	*  there are no pointer devices, so we always include that capability.*/
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (Server->keyboard_list.size() > 0)
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;

	wlr_seat_set_capabilities(Server->seat, caps);
}


void ServerCursorMotion(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_motion_event* event = (wlr_pointer_motion_event*)data;
	wlr_cursor_move(Server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
	ProcessCursorMotion(event->time_msec);
}

void ServerCursorMotionAbsolute(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_motion_absolute_event* event = (wlr_pointer_motion_absolute_event*)data;
	wlr_cursor_warp_absolute(Server->cursor, &event->pointer->base, event->x, event->y);
	ProcessCursorMotion(event->time_msec);
}

void ServerCursorButton(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_button_event* event = (wlr_pointer_button_event*)data;

	wlr_seat_pointer_notify_button(Server->seat, event->time_msec, event->button, event->state);
	double sx;
	double sy;
	struct wlr_surface* surface = NULL;
	EshyWMWindow* window = DesktopWindowAt(Server->cursor->x, Server->cursor->y, &surface, &sx, &sy);

	if (event->state == WLR_BUTTON_RELEASED)
		Server->ResetCursorMode();
	else
		window->FocusWindow(surface);

	if(Server->b_window_modifier_key_pressed && event->state == WLR_BUTTON_PRESSED)
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
}

void ServerCursorAxis(struct wl_listener* listener, void* data)
{
	struct wlr_pointer_axis_event* event = (wlr_pointer_axis_event*)data;
	wlr_seat_pointer_notify_axis(Server->seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source);
}

void ServerCursorFrame(struct wl_listener* listener, void* data)
{
	wlr_seat_pointer_notify_frame(Server->seat);
}

void ServerNewOutput(struct wl_listener* listener, void* data)
{
	/*This event is raised by the backend when a new output (aka a display or
	*  monitor) becomes available.*/
	struct wlr_output* wlr_output = (struct wlr_output*)data;

	/*Configures the output created by the backend to use our allocator
	*  and our renderer. Must be done once, before commiting the output*/
	wlr_output_init_render(wlr_output, Server->allocator, Server->renderer);

	/*The output may be disabled, switch it on.*/
	struct wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, true);

	/*Some backends don't have modes. DRM+KMS does, and we need to set a mode
	*  before we can use the output. The mode is a tuple of (width, height,
	*  refresh rate), and each monitor supports only a specific set of modes. We
	*  just pick the monitor's preferred mode, a more sophisticated compositor
	*  would let the user configure it.*/
	struct wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output);
	if (mode != NULL)
	{
		wlr_output_state_set_mode(&state, mode);
	}

	/*Atomically applies the new output state.*/
	wlr_output_commit_state(wlr_output, &state);
	wlr_output_state_finish(&state);

	/*Allocates and configures our state for this output*/
	EshyWMOutput* output = new EshyWMOutput;
	output->WlrOutput = wlr_output;

	add_listener(&output->FrameListener, OutputFrame, &wlr_output->events.frame);
	add_listener(&output->RequestStateListener, OutputRequestState, &wlr_output->events.request_state);
	add_listener(&output->DestroyListener, OutputDestroy, &wlr_output->events.destroy);

	Server->output_list.push_back(output);

	/*Adds this to the output layout. The add_auto function arranges outputs
	*  from left-to-right in the order they appear. A more sophisticated
	*  compositor would let the user configure the arrangement of outputs in the
	*  layout.
	* 
	*  The output layout utility automatically adds a wl_output global to the
	*  display, which Wayland clients can see to find out information about the
	*  output (such as DPI, scale factor, manufacturer, etc).
	*/
	wlr_output_layout_add_auto(Server->output_layout, wlr_output);
}

void ServerOutputChange(struct wl_listener* listener, void* data)
{
	struct wlr_output_layout_output* event = (wlr_output_layout_output*)data;

	if(!Server->Eshybar)
		return;

	int Width;
	int Height;
	wlr_output_effective_resolution(Server->output_list[0]->WlrOutput, &Width, &Height);
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
		assert(parent != NULL);
		struct wlr_scene_tree* parent_tree = (wlr_scene_tree*)parent->data;
		xdg_surface->data = wlr_scene_xdg_surface_create(parent_tree, xdg_surface);
	}
	else if (std::string(xdg_surface->toplevel->title) == std::string("Eshybar"))
	{
		EshyWMSpecialWindow* Eshybar = new EshyWMSpecialWindow;
		Eshybar->Initialize(xdg_surface);
		Eshybar->XdgToplevel->app_id = (char*)"eshybar";

		Server->Eshybar = Eshybar;

		int width;
		int height;
		wlr_output_effective_resolution(Server->output_list[0]->WlrOutput, &width, &height);
		wlr_scene_node_set_position(&Eshybar->SceneTree->node, 0, height - 50);

		//Send a message for all windows that exist at the time the bar is created
		for (EshyWMWindow* Window : Server->window_list)
		{
			nlohmann::json WindowAddInfo;
			WindowAddInfo["action"] = ACTION_ADD_WINDOW;
			WindowAddInfo["sender_client"] = CLIENT_COMPOSITOR;
			WindowAddInfo["window_id"] = (uint64_t)Window;
			WindowAddInfo["app_id"] = Window->XdgToplevel->app_id ? Window->XdgToplevel->app_id : "NO_APP_ID";
			WindowAddInfo["title"] = Window->XdgToplevel->title ? Window->XdgToplevel->title : "NO_TITLE";
			EshyIPC::InsertIntoMemory(EshybarShmID, WindowAddInfo.dump());
		}
	}
	else
	{
		EshyWMWindow* Window = new EshyWMWindow;
		Window->Initialize(xdg_surface);
		Server->window_list.push_back(Window);

		nlohmann::json WindowAddInfo;
		WindowAddInfo["action"] = ACTION_ADD_WINDOW;
		WindowAddInfo["sender_client"] = CLIENT_COMPOSITOR;
		WindowAddInfo["window_id"] = (uint64_t)Window;
		WindowAddInfo["app_id"] = Window->XdgToplevel->app_id ? Window->XdgToplevel->app_id : "NO_APP_ID";
		WindowAddInfo["title"] = Window->XdgToplevel->title ? Window->XdgToplevel->title : "NO_TITLE";
		EshyIPC::InsertIntoMemory(EshybarShmID, WindowAddInfo.dump());
	}
}


void ProcessCursorMotion(uint32_t time)
{
	/*If the mode is non-passthrough, delegate to those functions.*/
	if (Server->cursor_mode == ESHYWM_CURSOR_MOVE)
	{
		Server->focused_window->ProcessCursorMove(time);
		return;
	}
	else if (Server->cursor_mode == ESHYWM_CURSOR_RESIZE)
	{
		Server->focused_window->ProvessCursorResize(time);
		return;
	}

	/*Otherwise, find the window under the pointer and send the event along.*/
	double sx, sy;
	struct wlr_seat* seat = Server->seat;
	struct wlr_surface* surface = NULL;
	EshyWMWindow* window = DesktopWindowAt(Server->cursor->x, Server->cursor->y, &surface, &sx, &sy);

	if (!window)
	{
		/*If there's no window under the cursor, set the cursor image to a
		*  default. This is what makes the cursor image appear when you move it
		*  around the screen, not over any windows.*/
		wlr_cursor_set_xcursor(Server->cursor, Server->cursor_mgr, "default");
	}

	if (surface)
	{
		/*
		*  Send pointer enter and motion events.
		* 
		*  The enter event gives the surface "pointer focus", which is distinct
		*  from keyboard focus. You get pointer focus by moving the pointer over
		*  a window.
		* 
		*  Note that wlroots will avoid sending duplicate enter/motion events if
		*  the surface has already has pointer focus or if the client is already
		*  aware of the coordinates passed.
		*/
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	}
	else
	{
		/*Clear pointer focus so future button events and such are not sent to
		*  the last client to have the cursor over it.*/
		wlr_seat_pointer_clear_focus(seat);
	}
}

void SeatRequestCursor(struct wl_listener* listener, void* data)
{
	class EshyWMServer* server = wl_container_of(listener, server, request_cursor);
	/*This event is raised by the seat when a client provides a cursor image*/
	struct wlr_seat_pointer_request_set_cursor_event* event = (wlr_seat_pointer_request_set_cursor_event*)data;
	struct wlr_seat_client* focused_client = server->seat->pointer_state.focused_client;
	/*This can be sent by any client, so we check to make sure this one is
	*  actually has pointer focus first.*/
	if (focused_client == event->seat_client)
	{
		/*Once we've vetted the client, we can tell the cursor to use the
		*  provided surface as the cursor image. It will set the hardware cursor
		*  on the output that it's currently on and continue to do so as the
		*  cursor moves between outputs.*/
		wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x, event->hotspot_y);
	}
}

void SeatRequestSetSelection(struct wl_listener* listener, void* data)
{
	/*This event is raised by the seat when a client wants to set the selection,
	*  usually when the user copies something. wlroots allows compositors to
	*  ignore such requests if they so choose, but in eshywm we always honor
	*/
	class EshyWMServer* server = wl_container_of(listener, server, request_set_selection);
	struct wlr_seat_request_set_selection_event* event = (wlr_seat_request_set_selection_event*)data;
	wlr_seat_set_selection(server->seat, event->source, event->serial);
}