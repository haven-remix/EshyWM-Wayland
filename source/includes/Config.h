
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct EshyWMMonitorInfo
{
    std::string Name;
    int Width;
    int Height;
    int Refresh;
    int OffsetX;
    int OffsetY;
    float Scale;
};

namespace EshyWMConfig
{
extern uint32_t ESHYWM_BINDING_FULLSCREEN;
extern uint32_t ESHYWM_BINDING_MAXIMIZE;
extern uint32_t ESHYWM_BINDING_MINIMIZE;
extern uint32_t ESHYWM_BINDING_CLOSE_WINDOW;

extern float ESHYWM_COLOR_BORDER_NORMAL[4];
extern float ESHYWM_COLOR_BORDER_FOCUSED[4];

extern int ESHYWM_BORDER_WIDTH;

void InitializeKeys();
void ReadConfigFromFile(const std::string& ConfigFilePath);

std::vector<std::string> GetStartupCommands();
std::vector<EshyWMMonitorInfo> GetMonitorInfoList();
std::unordered_map<uint32_t, std::string> GetKeyboundCommands();
}