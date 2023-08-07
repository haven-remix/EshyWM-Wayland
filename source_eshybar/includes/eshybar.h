#pragma once

#include <string>

class TaskbarIcon
{
public:

	TaskbarIcon(class euiRenderer* renderer, const std::string& IconImagePath);

	void Draw();

private:

	class euiRenderer* renderer;
	class euiSolidEntity* background;
	class euiImageEntity* image;
};

class Eshybar
{
public:

    Eshybar();
	~Eshybar();
};