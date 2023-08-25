
#include "Keyboard.h"
#include "Server.h"
#include "Window.h"
#include "Config.h"

#define static

extern "C"
{
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
}

#undef static

#include <iostream>

void KeyboardHandleModifiers(struct wl_listener* listener, void* data)
{
	const EshyWMKeyboard* keyboard = wl_container_of(listener, keyboard, ModifiersListener);

	wlr_seat_set_keyboard(Server->Seat, keyboard->WlrKeyboard);
	wlr_seat_keyboard_notify_modifiers(Server->Seat, &keyboard->WlrKeyboard->modifiers);

	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->WlrKeyboard);

	//Window modifier
	Server->bWindowModifierKeyPressed = modifiers & WLR_MODIFIER_LOGO ? Server->bWindowModifierKeyPressed : false;

	if(!Server->bWindowModifierKeyPressed)
		Server->ResetCursorMode();
}

bool HandleKeybinding(xkb_keysym_t sym)
{
	//This function assumes super is held down
	if (sym == XKB_KEY_Escape)
	{
		wl_display_terminate(Server->WlDisplay);
	}
	else if (sym == EshyWMConfig::ESHYWM_BINDING_MINIMIZE)
	{
		if (Server->FocusedWindow)
			Server->FocusedWindow->ToggleMinimize();
	}
	else if (sym == EshyWMConfig::ESHYWM_BINDING_MAXIMIZE)
	{
		if (Server->FocusedWindow)
			Server->FocusedWindow->ToggleMaximize();
	}
	else if (sym == EshyWMConfig::ESHYWM_BINDING_FULLSCREEN)
	{
		if (Server->FocusedWindow)
			Server->FocusedWindow->ToggleFullscreen();
	}
	else if (sym == EshyWMConfig::ESHYWM_BINDING_CLOSE_WINDOW)
	{
		if (Server->FocusedWindow)
			Server->CloseWindow(Server->FocusedWindow);
	}
	else if (EshyWMConfig::GetKeyboundCommands().find(sym) != EshyWMConfig::GetKeyboundCommands().end())
	{
		system(EshyWMConfig::GetKeyboundCommands()[sym].c_str());
	}
	else
		return false;

	return true;
}

void KeyboardHandleKey(struct wl_listener* listener, void* data)
{
	const EshyWMKeyboard* keyboard = wl_container_of(listener, keyboard, KeyListener);
	const struct wlr_keyboard_key_event* event = (wlr_keyboard_key_event* )data;
	struct wlr_seat* seat = Server->Seat;

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
				Server->bWindowModifierKeyPressed = true;
				handled = true;
				break;
			}
			else if (syms[i] == XKB_KEY_Control_L && event->state == WL_KEYBOARD_KEY_STATE_RELEASED)
			{
				Server->bWindowModifierKeyPressed = false;
				handled = true;
				break;
			}

	//Window switching
	if (!handled && (modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
	{
		for (int i = 0; i < nsyms; i++)
			if (syms[i] == XKB_KEY_Tab)
			{				
				Server->NextWindowIndex++;
				if (Server->NextWindowIndex > Server->WindowList.size() - 1)
					Server->NextWindowIndex = 0;

				handled = true;
				break;
			}
	}
	else if (!handled && event->state == WL_KEYBOARD_KEY_STATE_RELEASED)
	{
		for (int i = 0; i < nsyms; i++)
			if (syms[i] == XKB_KEY_Alt_L)
			{				
				EshyWMWindowBase* next_window = Server->WindowList[Server->NextWindowIndex];
				next_window->FocusWindow();
				Server->NextWindowIndex = 0;
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