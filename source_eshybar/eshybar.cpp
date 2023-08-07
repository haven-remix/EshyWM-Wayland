
#include "eshybar.h"

#include "EshyUI.h"
#include "Entity.h"
#include "Renderer.h"

#include <vector>

static float StartingLocationX = 5.0f;
static float StartingLocationY = 5.0f;
static float BackgroundWidth = 40.0f;
static float BackgroundHeight = 40.0f;
static float ImageWidth = 30.0f;
static float ImageHeight = 30.0f;
static float Padding = 5.0f;
static float InternalPadding = 10.0f;

struct Icon
{
	euiSolidEntity* Background;
	euiImageEntity* Image;
};

static void NotifyPointerEnter(euiEntity* Entity)
{
	
}

static void NotifyPointerExit(euiEntity* Entity)
{
	
}

static void NotifyPointerPressed(euiEntity* Entity)
{
	
}

static void NotifyPointerUnpressed(euiEntity* Entity)
{
	
}

static Icon AddIcon(int Index, euiRenderer* Renderer, const std::string& ImagePath)
{
	euiSolidEntity* Background = new euiSolidEntity(StartingLocationX + (BackgroundWidth * Index) + (Padding * Index), StartingLocationY, BackgroundWidth, BackgroundHeight, EUI_ANCHOR_LEFT_BOTTOM);
	euiImageEntity* Image = new euiImageEntity(StartingLocationX + (BackgroundWidth * Index) + (Padding * Index) + (InternalPadding / 2), StartingLocationY + (InternalPadding / 2.0f), ImageWidth, ImageHeight, EUI_ANCHOR_LEFT_BOTTOM, Renderer, ImagePath);

	Background->SetNormalColor(glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));
	Background->SetHoveredColor(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
	Background->SetPressedColor(glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));

	Background->NotifyPointerEnterCallback = &NotifyPointerEnter;
	Background->NotifyPointerExitCallback = &NotifyPointerExit;
	Background->NotifyPointerPressedCallback = &NotifyPointerPressed;
	Background->NotifyPointerUnpressedCallback = &NotifyPointerUnpressed;

	return {Background, Image};
}


int main()
{
	GLFWwindow* window = InitEshyUI("Eshybar", 800, 600);
	euiRenderer renderer(window);

	std::vector<Icon> Icons;
	Icons.push_back(AddIcon((int)Icons.size(), &renderer, "./Resources/image1.png"));
	Icons.push_back(AddIcon((int)Icons.size(), &renderer, "./Resources/image2.png"));
	Icons.push_back(AddIcon((int)Icons.size(), &renderer, "./Resources/image3.png"));
	Icons.push_back(AddIcon((int)Icons.size(), &renderer, "./Resources/image4.png"));
	Icons.push_back(AddIcon((int)Icons.size(), &renderer, "./Resources/image5.png"));
	Icons.push_back(AddIcon((int)Icons.size(), &renderer, "./Resources/image1.png"));
	Icons.push_back(AddIcon((int)Icons.size(), &renderer, "./Resources/image2.png"));
	Icons.push_back(AddIcon((int)Icons.size(), &renderer, "./Resources/image3.png"));

    while (!glfwWindowShouldClose(window))
    {
		renderer.Clear();
		renderer.SetBackgroundColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));

		static double PointerXPos;
		static double PointerYPos;
		glfwGetCursorPos(window, &PointerXPos, &PointerYPos);
		PointerYPos = renderer.GetWindowHeight() - PointerYPos;

		static int bLeftButtonPressedState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

		for (const Icon& Icon : Icons)
		{			
			Icon.Background->CheckPointerStatus(PointerXPos, PointerYPos, bLeftButtonPressedState);
			Icon.Background->Draw(renderer);
			Icon.Image->Draw(renderer);
		}

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	Shutdown();
	return 0;
}