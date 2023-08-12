
#include "Keyboard.h"
#include "Server.h"
#include "Window.h"

void KeyboardHandleModifiers(struct wl_listener* listener, void* data)
{
	const EshyWMKeyboard* keyboard = wl_container_of(listener, keyboard, ModifiersListener);

	wlr_seat_set_keyboard(Server->seat, keyboard->WlrKeyboard);
	wlr_seat_keyboard_notify_modifiers(Server->seat, &keyboard->WlrKeyboard->modifiers);

	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->WlrKeyboard);

	//Window modifier
	Server->b_window_modifier_key_pressed = modifiers & WLR_MODIFIER_LOGO ? Server->b_window_modifier_key_pressed : false;

	if(!Server->b_window_modifier_key_pressed)
		Server->ResetCursorMode();
}

bool HandleKeybinding(xkb_keysym_t sym)
{
	//This function assumes super is held down
	switch (sym)
	{
	case XKB_KEY_Escape:
		wl_display_terminate(Server->wl_display);
		break;
	case XKB_KEY_h:
	{
		if (Server->focused_window)
			Server->focused_window->ToggleMinimize();
		break;
	}
	case XKB_KEY_j:
	{
		if (Server->focused_window)
			Server->focused_window->ToggleMaximize();
		break;
	}
	case XKB_KEY_k:
	{
		if (Server->focused_window)
			Server->focused_window->ToggleFullscreen();
		break;
	}
	case XKB_KEY_n:
	{
		if (Server->focused_window)
			Server->CloseWindow(Server->focused_window);
		break;
	}
	case XKB_KEY_r:
		system("wofi --show drun --fork --allow-images");
		break;
	default:
		return false;
	}
	return true;
}

void KeyboardHandleKey(struct wl_listener* listener, void* data)
{
	const EshyWMKeyboard* keyboard = wl_container_of(listener, keyboard, KeyListener);
	const struct wlr_keyboard_key_event* event = (wlr_keyboard_key_event* )data;
	struct wlr_seat* seat = Server->seat;

	//Translate libinput keycode -> xkbcommon
	uint32_t keycode = event->keycode + 8;
	//Get a list of keysyms based on the keymap for this keyboard
	const xkb_keysym_t* syms;
	int nsyms = xkb_state_key_get_syms(keyboard->WlrKeyboard->xkb_state, keycode, &syms);

	bool handled = false;
	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->WlrKeyboard);
	if ((modifiers & WLR_MODIFIER_LOGO) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
		for (int i = 0; i < nsyms; i++)
			handled = HandleKeybinding(syms[i]);
	
	//Window modifier
	if (!handled && modifiers & WLR_MODIFIER_LOGO)
		for (int i = 0; i < nsyms; i++)
			if (syms[i] == XKB_KEY_Control_L && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
			{
				Server->b_window_modifier_key_pressed = true;
				handled = true;
				break;
			}
			else if (syms[i] == XKB_KEY_Control_L && event->state == WL_KEYBOARD_KEY_STATE_RELEASED)
			{
				Server->b_window_modifier_key_pressed = false;
				handled = true;
				break;
			}

	//Window switching
	if (!handled && (modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		for (int i = 0; i < nsyms; i++)
			if (syms[i] == XKB_KEY_Tab)
			{				
				Server->next_window_index++;
				if (Server->next_window_index > Server->window_list.size() - 1)
					Server->next_window_index = 0;

				handled = true;
				break;
			}
	}
	else if (!handled && event->state == WL_KEYBOARD_KEY_STATE_RELEASED)
	{
		for (int i = 0; i < nsyms; i++)
			if (syms[i] == XKB_KEY_Alt_L)
			{				
				EshyWMWindow* next_window = Server->window_list[Server->next_window_index];
				next_window->FocusWindow(next_window->XdgToplevel->base->surface);
				Server->next_window_index = 0;
				handled = true;
				break;
			}
	}

	if (!handled)
	{
		wlr_seat_set_keyboard(seat, keyboard->WlrKeyboard);
		wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
	}
}

void KeyboardHandleDestroy(struct wl_listener* listener, void* data)
{
	class EshyWMKeyboard* keyboard = wl_container_of(listener, keyboard, DestroyListener);
	wl_list_remove(&keyboard->ModifiersListener.link);
	wl_list_remove(&keyboard->KeyListener.link);
	wl_list_remove(&keyboard->DestroyListener.link);
	free(keyboard);
}