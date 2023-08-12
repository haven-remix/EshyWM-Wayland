
#pragma once

#define ACTION_ADD_WINDOW           "ADD_WINDOW"
#define ACTION_REMOVE_WINDOW        "REMOVE_WINDOW"
#define ACTION_FOCUS_WINDOW         "FOCUS_WINDOW"
#define ACTION_UNFOCUS_WINDOW       "UNFOCUS_WINDOW"
#define ACTION_MINIMIZE_WINDOW      "MINIMIZE_WINDOW"
#define ACTION_INIT_ESHYBAR         "INIT_ESHYBAR"
#define ACTION_CONFIGURE_ESHYBAR    "CONFIGURE_ESHYBAR"

#define CLIENT_COMPOSITOR           "EshyWM"
#define CLIENT_ESHYBAR              "Eshybar"

enum EEshyWMWindowState
{
	ESHYWM_WINDOW_STATE_NORMAL,
	ESHYWM_WINDOW_STATE_MINIMIZED,
	ESHYWM_WINDOW_STATE_MAXIMIZED,
	ESHYWM_WINDOW_STATE_FULLSCREEN
};