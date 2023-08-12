
#include "eshybar.h"

#include "EshyUI.h"
#include "Entity.h"
#include "Renderer.h"

#include "EshyIPC.h"
#include "Shared.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <map>
#include <fstream>

static float StartingLocationX = 5.0f;
static float StartingLocationY = 5.0f;
static float BackgroundWidth = 40.0f;
static float BackgroundHeight = 40.0f;
static float ImageWidth = 30.0f;
static float ImageHeight = 30.0f;
static float Padding = 5.0f;
static float InternalPadding = 10.0f;

static int SHMID = 0;
static bool bLeftButtonPressed = false;

euiRenderer* renderer;

struct EshyWMWindowRef
{
	EshyWMWindowRef(euiSolidEntity* _Background, euiImageEntity* _Image)
		: Background(_Background)
		, Image(_Image)
		, WindowState(ESHYWM_WINDOW_STATE_NORMAL)
		, bWindowFocused(false)
	{}

	euiSolidEntity* Background;
	euiImageEntity* Image;

	EEshyWMWindowState WindowState;
	bool bWindowFocused;

	uint64_t WindowID;
};

static std::map<uint64_t, EshyWMWindowRef*> EWMWindows;

static void SendDataToCompositor(const char* Action, uint64_t Data)
{
	nlohmann::json SendInfo;
	SendInfo["action"] = Action;
	SendInfo["sender_client"] = CLIENT_ESHYBAR;
	SendInfo["window_id"] = Data;
	EshyIPC::InsertIntoMemoryUnprotected(SHMID, SendInfo.dump());
}

static void NotifyPointerEnter(euiEntity* Entity, void* Data)
{
	
}

static void NotifyPointerExit(euiEntity* Entity, void* Data)
{
	
}

static void NotifyPointerHover(euiEntity* Entity, void* Data)
{
	
}

static void NotifyPointerPressed(euiEntity* Entity, void* Data)
{
	EshyWMWindowRef* pointer = (EshyWMWindowRef*)Data;

	if(!pointer)
		return;

	if(!pointer->bWindowFocused && (pointer->WindowState == ESHYWM_WINDOW_STATE_NORMAL || pointer->WindowState == ESHYWM_WINDOW_STATE_MINIMIZED))
	{
		SendDataToCompositor(ACTION_FOCUS_WINDOW, pointer->WindowID);
		pointer->bWindowFocused = true;
		pointer->WindowState = ESHYWM_WINDOW_STATE_NORMAL;
	}
	else if(pointer->bWindowFocused && pointer->WindowState == ESHYWM_WINDOW_STATE_NORMAL)
	{
		SendDataToCompositor(ACTION_MINIMIZE_WINDOW, pointer->WindowID);
		pointer->bWindowFocused = false;
		pointer->WindowState = ESHYWM_WINDOW_STATE_MINIMIZED;
	}
}

static void NotifyPointerUnpressed(euiEntity* Entity, void* Data)
{
	
}

static void NotifyPointerHeld(euiEntity* Entity, void* Data)
{
	
}

static EshyWMWindowRef* AddIcon(int Index, const std::string& ImagePath, uint64_t WindowID)
{
	euiSolidEntity* Background = new euiSolidEntity(StartingLocationX + (BackgroundWidth * Index) + (Padding * Index), StartingLocationY, BackgroundWidth, BackgroundHeight, EUI_ANCHOR_LEFT_BOTTOM);
	euiImageEntity* Image = new euiImageEntity(StartingLocationX + (BackgroundWidth * Index) + (Padding * Index) + (InternalPadding / 2), StartingLocationY + (InternalPadding / 2.0f), ImageWidth, ImageHeight, EUI_ANCHOR_LEFT_BOTTOM, renderer, ImagePath);

	Background->SetNormalColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
	Background->SetHoveredColor(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
	Background->SetPressedColor(glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));

	Background->NotifyPointerEnterCallback = &NotifyPointerEnter;
	Background->NotifyPointerExitCallback = &NotifyPointerExit;
	Background->NotifyPointerHoverCallback = &NotifyPointerHover;
	Background->NotifyPointerPressedCallback = &NotifyPointerPressed;
	Background->NotifyPointerUnpressedCallback = &NotifyPointerUnpressed;
	Background->NotifyPointerHeldCallback = &NotifyPointerHover;

	EshyWMWindowRef* WindowRef = new EshyWMWindowRef(Background, Image);
	WindowRef->WindowID = WindowID;
	Background->CallbackData = (void*)(uint64_t)WindowRef;
	return WindowRef;
}


static void MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods)
{
	bLeftButtonPressed = Action == GLFW_PRESS;
}

static std::string RetrieveIconFilePath(const std::string& AppId)
{
	std::ifstream DesktopFile("/usr/share/applications/" + AppId + ".desktop");

	if(!DesktopFile.is_open())
		return "/usr/share/icons/hicolor/256x256/apps/qemu.png";

	std::string Line;
	std::string IconName;

	while(getline(DesktopFile, Line))
	{
		if(Line.find("Icon") != std::string::npos)
		{
			size_t Del = Line.find("=");
			IconName = Line.substr(Del + 1);
			break;
		}
	}

	DesktopFile.close();
	return "/usr/share/icons/hicolor/256x256/apps/" + IconName + ".png";
}

static void SharedMemoryChanged(const std::string& CurrentShm)
{
	nlohmann::json Data = nlohmann::json::parse(CurrentShm);
	if (Data["sender_client"] == CLIENT_COMPOSITOR)
	{
		if (Data["action"] == ACTION_ADD_WINDOW)
		{
			//Make icons for any new windows
			EshyWMWindowRef* WindowRef = AddIcon((int)EWMWindows.size(), RetrieveIconFilePath(Data["app_id"]), Data["window_id"]);
			auto result = EWMWindows.emplace(Data["window_id"], WindowRef);
		}
		else if (Data["action"] == ACTION_REMOVE_WINDOW)
		{
			//Remove icons for any removed window
			EWMWindows.erase(Data["window_id"]);
		}
		else if (Data["action"] == ACTION_CONFIGURE_ESHYBAR)
		{
			renderer->UpdateWindowSize((float)Data["width"], 50.0f);
		}
	}
}

int main(int argc, char* argv[])
{
	const int ScreenWidth = atoi(argv[2]);
	const int ScreenHeight = atoi(argv[3]);

	GLFWwindow* window = InitEshyUI("Eshybar", ScreenWidth, 50);
	renderer = new euiRenderer(window);

	glfwSetMouseButtonCallback(window, MouseButtonCallback);

	SHMID = atoi(argv[1]);
	eipcSharedMemory SharedMemory;
	std::string CurrentShm;

    while (!glfwWindowShouldClose(window))
    {
		renderer->Clear();
		renderer->SetBackgroundColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));

		static double PointerXPos;
		static double PointerYPos;
		glfwGetCursorPos(window, &PointerXPos, &PointerYPos);
		PointerYPos = renderer->GetWindowHeight() - PointerYPos;

		for (auto [key, Icon] : EWMWindows)
		{
			Icon->Background->CheckPointerStatus(PointerXPos, PointerYPos, bLeftButtonPressed);
			Icon->Background->Draw(*renderer);
			Icon->Image->Draw(*renderer);
		}

		//Only runs when shared memory changes
		SharedMemory = EshyIPC::AttachSharedMemoryBlock(SHMID);
		if(CurrentShm != SharedMemory.Block)
		{
			CurrentShm = SharedMemory.Block;
			SharedMemoryChanged(CurrentShm);
		}

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	Shutdown();
	return 0;
}