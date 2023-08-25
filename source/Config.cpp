
#include "Config.h"

#include <fstream>
#include <iostream>
#include <cstring>

#include <xkbcommon/xkbcommon.h>

#define ESHYWM_KEY_1                             "KEY_1"
#define ESHYWM_KEY_2                             "KEY_2"
#define ESHYWM_KEY_3                             "KEY_3"
#define ESHYWM_KEY_4                             "KEY_4"
#define ESHYWM_KEY_5                             "KEY_5"
#define ESHYWM_KEY_6                             "KEY_6"
#define ESHYWM_KEY_7                             "KEY_7"
#define ESHYWM_KEY_8                             "KEY_8"
#define ESHYWM_KEY_9                             "KEY_9"
#define ESHYWM_KEY_colon                         "KEY_colon"
#define ESHYWM_KEY_semicolon                     "KEY_semicolon"
#define ESHYWM_KEY_less                          "KEY_less"
#define ESHYWM_KEY_equal                         "KEY_equal"
#define ESHYWM_KEY_greater                       "KEY_greater"
#define ESHYWM_KEY_question                      "KEY_question"
#define ESHYWM_KEY_A                             "KEY_A"
#define ESHYWM_KEY_B                             "KEY_B"
#define ESHYWM_KEY_C                             "KEY_C"
#define ESHYWM_KEY_D                             "KEY_D"
#define ESHYWM_KEY_E                             "KEY_E"
#define ESHYWM_KEY_F                             "KEY_F"
#define ESHYWM_KEY_G                             "KEY_G"
#define ESHYWM_KEY_H                             "KEY_H"
#define ESHYWM_KEY_I                             "KEY_I"
#define ESHYWM_KEY_J                             "KEY_J"
#define ESHYWM_KEY_K                             "KEY_K"
#define ESHYWM_KEY_L                             "KEY_L"
#define ESHYWM_KEY_M                             "KEY_M"
#define ESHYWM_KEY_N                             "KEY_N"
#define ESHYWM_KEY_O                             "KEY_O"
#define ESHYWM_KEY_P                             "KEY_P"
#define ESHYWM_KEY_Q                             "KEY_Q"
#define ESHYWM_KEY_R                             "KEY_R"
#define ESHYWM_KEY_S                             "KEY_S"
#define ESHYWM_KEY_T                             "KEY_T"
#define ESHYWM_KEY_U                             "KEY_U"
#define ESHYWM_KEY_V                             "KEY_V"
#define ESHYWM_KEY_W                             "KEY_W"
#define ESHYWM_KEY_X                             "KEY_X"
#define ESHYWM_KEY_Y                             "KEY_Y"
#define ESHYWM_KEY_Z                             "KEY_Z"
#define ESHYWM_KEY_a                             "KEY_a"
#define ESHYWM_KEY_b                             "KEY_b"
#define ESHYWM_KEY_c                             "KEY_c"
#define ESHYWM_KEY_d                             "KEY_d"
#define ESHYWM_KEY_e                             "KEY_e"
#define ESHYWM_KEY_f                             "KEY_f"
#define ESHYWM_KEY_g                             "KEY_g"
#define ESHYWM_KEY_h                             "KEY_h"
#define ESHYWM_KEY_i                             "KEY_i"
#define ESHYWM_KEY_j                             "KEY_j"
#define ESHYWM_KEY_k                             "KEY_k"
#define ESHYWM_KEY_l                             "KEY_l"
#define ESHYWM_KEY_m                             "KEY_m"
#define ESHYWM_KEY_n                             "KEY_n"
#define ESHYWM_KEY_o                             "KEY_o"
#define ESHYWM_KEY_p                             "KEY_p"
#define ESHYWM_KEY_q                             "KEY_q"
#define ESHYWM_KEY_r                             "KEY_r"
#define ESHYWM_KEY_s                             "KEY_s"
#define ESHYWM_KEY_t                             "KEY_t"
#define ESHYWM_KEY_u                             "KEY_u"
#define ESHYWM_KEY_v                             "KEY_v"
#define ESHYWM_KEY_w                             "KEY_w"
#define ESHYWM_KEY_x                             "KEY_x"
#define ESHYWM_KEY_y                             "KEY_y"
#define ESHYWM_KEY_z                             "KEY_z"

#define DetermineRelevantConfigSection(line, SectionIndicator, Section) \
    if(line.find(SectionIndicator) != std::string::npos) \
    { \
        CurrentConfigSection = Section; \
        continue; \
    }

static std::unordered_map<std::string, uint32_t> KeyMap;

enum EshyWMConfigSections
{
    CONFIG_NONE,
    CONFIG_STARTUP_COMMANDS,
    CONFIG_MONITOR
};

enum VarType
{
    VT_INT,
    VT_UINT,
    VT_UINT_HEX,
    VT_ULONG,
    VT_ULONG_HEX,
    VT_STRING,
    VT_KEY,
    VT_KEY_COMMAND,
    VT_COLOR
};

template<class T>
struct key_value_pair
{
    T key;
    T value;
};

static key_value_pair<std::string> split(const std::string& s, const std::string& delimiter)
{
    const size_t pos = s.find(delimiter);
    return {s.substr(0, pos).c_str(), s.substr(pos + 1, s.length() - pos).c_str()};
}

static bool parse_config_option(std::string line, VarType type, void* config_var, std::string option_name)
{
    if(line.find(option_name) != std::string::npos)
    {
        const key_value_pair kvp = split(line, "=");

        switch(type)
        {
        case VarType::VT_INT:
            *((int*)config_var) = std::stoi(kvp.value);
            break;
        case VarType::VT_UINT:
            *((uint*)config_var) = std::stoi(kvp.value);
            break;
        case VarType::VT_UINT_HEX:
            *((uint*)config_var) = std::stoi(kvp.value, 0, 16);
            break;
        case VarType::VT_ULONG:
            *((ulong*)config_var) = std::stoul(kvp.value);
            break;
        case VarType::VT_ULONG_HEX:
            *((ulong*)config_var) = std::stoul(kvp.value, 0, 16);
            break;
        case VarType::VT_STRING:
            *(std::string*)config_var = kvp.value;
            break;
        case VarType::VT_KEY:
        {
            const uint32_t KeyCode = KeyMap[std::string(kvp.value)];
            *((uint32_t*)config_var) = KeyCode;
            break;
        }
        case VarType::VT_KEY_COMMAND:
        {
            const key_value_pair kvp2 = split(kvp.value, ",");
            (*((std::unordered_map<uint32_t, std::string>*)config_var)).emplace(KeyMap[std::string(kvp2.key)], kvp2.value);
            break;
        }
        case VarType::VT_COLOR:
        {
            float color[4];
            std::string current = kvp.value;
            for(int i = 0; i < 4; ++i)
            {
                const key_value_pair kvpi = split(current, ",");
                color[i] = std::stof(kvpi.key);
                current = kvpi.value;
            }
            memcpy(config_var, color, sizeof(color));
            break;
        }
        };

        return true;
    }

    return false;
}

static std::vector<std::string> StartupCommands;
static std::vector<EshyWMMonitorInfo> MonitorInfoList;
static std::unordered_map<uint32_t, std::string> KeyboundCommands;

namespace EshyWMConfig
{
uint32_t ESHYWM_BINDING_FULLSCREEN = 0;
uint32_t ESHYWM_BINDING_MAXIMIZE = 0;
uint32_t ESHYWM_BINDING_MINIMIZE = 0;
uint32_t ESHYWM_BINDING_CLOSE_WINDOW = 0;

float ESHYWM_COLOR_BORDER_NORMAL[4] = {0.4f, 0.4f, 0.4f, 1.0f};
float ESHYWM_COLOR_BORDER_FOCUSED[4] = {0.0f, 0.5f, 0.5f, 1.0f};

int ESHYWM_BORDER_WIDTH = 1;

void InitializeKeys()
{
    KeyMap.emplace(ESHYWM_KEY_1, XKB_KEY_1);
    KeyMap.emplace(ESHYWM_KEY_2, XKB_KEY_2);
    KeyMap.emplace(ESHYWM_KEY_3, XKB_KEY_3);
    KeyMap.emplace(ESHYWM_KEY_4, XKB_KEY_4);
    KeyMap.emplace(ESHYWM_KEY_5, XKB_KEY_5);
    KeyMap.emplace(ESHYWM_KEY_6, XKB_KEY_6);
    KeyMap.emplace(ESHYWM_KEY_7, XKB_KEY_7);
    KeyMap.emplace(ESHYWM_KEY_8, XKB_KEY_8);
    KeyMap.emplace(ESHYWM_KEY_9, XKB_KEY_9);
    KeyMap.emplace(ESHYWM_KEY_colon, XKB_KEY_colon);
    KeyMap.emplace(ESHYWM_KEY_semicolon, XKB_KEY_semicolon);
    KeyMap.emplace(ESHYWM_KEY_less, XKB_KEY_less);
    KeyMap.emplace(ESHYWM_KEY_equal, XKB_KEY_equal);
    KeyMap.emplace(ESHYWM_KEY_greater, XKB_KEY_greater);
    KeyMap.emplace(ESHYWM_KEY_question, XKB_KEY_question);
    KeyMap.emplace(ESHYWM_KEY_A, XKB_KEY_A);
    KeyMap.emplace(ESHYWM_KEY_B, XKB_KEY_B);
    KeyMap.emplace(ESHYWM_KEY_C, XKB_KEY_C);
    KeyMap.emplace(ESHYWM_KEY_D, XKB_KEY_D);
    KeyMap.emplace(ESHYWM_KEY_E, XKB_KEY_E);
    KeyMap.emplace(ESHYWM_KEY_F, XKB_KEY_F);
    KeyMap.emplace(ESHYWM_KEY_G, XKB_KEY_G);
    KeyMap.emplace(ESHYWM_KEY_H, XKB_KEY_H);
    KeyMap.emplace(ESHYWM_KEY_I, XKB_KEY_I);
    KeyMap.emplace(ESHYWM_KEY_J, XKB_KEY_J);
    KeyMap.emplace(ESHYWM_KEY_K, XKB_KEY_K);
    KeyMap.emplace(ESHYWM_KEY_L, XKB_KEY_L);
    KeyMap.emplace(ESHYWM_KEY_M, XKB_KEY_M);
    KeyMap.emplace(ESHYWM_KEY_N, XKB_KEY_N);
    KeyMap.emplace(ESHYWM_KEY_O, XKB_KEY_O);
    KeyMap.emplace(ESHYWM_KEY_P, XKB_KEY_P);
    KeyMap.emplace(ESHYWM_KEY_Q, XKB_KEY_Q);
    KeyMap.emplace(ESHYWM_KEY_R, XKB_KEY_R);
    KeyMap.emplace(ESHYWM_KEY_S, XKB_KEY_S);
    KeyMap.emplace(ESHYWM_KEY_T, XKB_KEY_T);
    KeyMap.emplace(ESHYWM_KEY_U, XKB_KEY_U);
    KeyMap.emplace(ESHYWM_KEY_V, XKB_KEY_V);
    KeyMap.emplace(ESHYWM_KEY_W, XKB_KEY_W);
    KeyMap.emplace(ESHYWM_KEY_X, XKB_KEY_X);
    KeyMap.emplace(ESHYWM_KEY_Y, XKB_KEY_Y);
    KeyMap.emplace(ESHYWM_KEY_Z, XKB_KEY_Z);
    KeyMap.emplace(ESHYWM_KEY_a, XKB_KEY_a);
    KeyMap.emplace(ESHYWM_KEY_b, XKB_KEY_b);
    KeyMap.emplace(ESHYWM_KEY_c, XKB_KEY_c);
    KeyMap.emplace(ESHYWM_KEY_d, XKB_KEY_d);
    KeyMap.emplace(ESHYWM_KEY_e, XKB_KEY_e);
    KeyMap.emplace(ESHYWM_KEY_f, XKB_KEY_f);
    KeyMap.emplace(ESHYWM_KEY_g, XKB_KEY_g);
    KeyMap.emplace(ESHYWM_KEY_h, XKB_KEY_h);
    KeyMap.emplace(ESHYWM_KEY_i, XKB_KEY_i);
    KeyMap.emplace(ESHYWM_KEY_j, XKB_KEY_j);
    KeyMap.emplace(ESHYWM_KEY_k, XKB_KEY_k);
    KeyMap.emplace(ESHYWM_KEY_l, XKB_KEY_l);
    KeyMap.emplace(ESHYWM_KEY_m, XKB_KEY_m);
    KeyMap.emplace(ESHYWM_KEY_n, XKB_KEY_n);
    KeyMap.emplace(ESHYWM_KEY_o, XKB_KEY_o);
    KeyMap.emplace(ESHYWM_KEY_p, XKB_KEY_p);
    KeyMap.emplace(ESHYWM_KEY_q, XKB_KEY_q);
    KeyMap.emplace(ESHYWM_KEY_r, XKB_KEY_r);
    KeyMap.emplace(ESHYWM_KEY_s, XKB_KEY_s);
    KeyMap.emplace(ESHYWM_KEY_t, XKB_KEY_t);
    KeyMap.emplace(ESHYWM_KEY_u, XKB_KEY_u);
    KeyMap.emplace(ESHYWM_KEY_v, XKB_KEY_v);
    KeyMap.emplace(ESHYWM_KEY_w, XKB_KEY_w);
    KeyMap.emplace(ESHYWM_KEY_x, XKB_KEY_x);
    KeyMap.emplace(ESHYWM_KEY_y, XKB_KEY_y);
    KeyMap.emplace(ESHYWM_KEY_z, XKB_KEY_z);
}

void ReadConfigFromFile(const std::string& ConfigFilePath)
{
    std::ifstream ConfigFile(ConfigFilePath);
    std::string Line;

    EshyWMConfigSections CurrentConfigSection = CONFIG_NONE;
    EshyWMMonitorInfo MonitorInfo = {"", 0, 0, 0, 0, 0, 0};

    while (std::getline(ConfigFile, Line))
    {   
        if(Line.find("}") != std::string::npos)
        {
            //Check if monitor info dirty, if so, then push into vector
            if(MonitorInfo.Name != "")
            {
                MonitorInfoList.push_back(MonitorInfo);
                MonitorInfo = {"", 0, 0, 0, 0, 0, 0};
            }

            CurrentConfigSection = CONFIG_NONE;
            continue;
        }

        DetermineRelevantConfigSection(Line, "startup_commands {", CONFIG_STARTUP_COMMANDS);
        DetermineRelevantConfigSection(Line, "monitor {", CONFIG_MONITOR);

        switch (CurrentConfigSection)
        {
        case CONFIG_STARTUP_COMMANDS:
            StartupCommands.push_back(Line);
            break;
        case CONFIG_MONITOR:
        {
            parse_config_option(Line, VT_STRING, &MonitorInfo.Name, "name");
            parse_config_option(Line, VT_INT, &MonitorInfo.Width, "width");
            parse_config_option(Line, VT_INT, &MonitorInfo.Height, "height");
            parse_config_option(Line, VT_INT, &MonitorInfo.Refresh, "refresh");
            parse_config_option(Line, VT_INT, &MonitorInfo.OffsetX, "offsetx");
            parse_config_option(Line, VT_INT, &MonitorInfo.OffsetY, "offsety");
            parse_config_option(Line, VT_INT, &MonitorInfo.Scale, "scaling");
            break;
        }
        case CONFIG_NONE:
        {
            parse_config_option(Line, VT_KEY, &ESHYWM_BINDING_FULLSCREEN, "bind_fullscreen");
            parse_config_option(Line, VT_KEY, &ESHYWM_BINDING_MAXIMIZE, "bind_maximize");
            parse_config_option(Line, VT_KEY, &ESHYWM_BINDING_MINIMIZE, "bind_minimize");
            parse_config_option(Line, VT_KEY, &ESHYWM_BINDING_CLOSE_WINDOW, "bind_close_window");
            parse_config_option(Line, VT_KEY_COMMAND, &KeyboundCommands, "bind_command");

            parse_config_option(Line, VT_INT, &ESHYWM_BORDER_WIDTH, "border_width");
            parse_config_option(Line, VT_COLOR, &ESHYWM_COLOR_BORDER_NORMAL, "border_color_normal");
            parse_config_option(Line, VT_COLOR, &ESHYWM_COLOR_BORDER_FOCUSED, "border_color_focused");
            break;
        }
        default:
            break;
        }
    }

    ConfigFile.close();
}

std::vector<std::string> GetStartupCommands()
{
    return StartupCommands;
}

std::vector<EshyWMMonitorInfo> GetMonitorInfoList()
{
    return MonitorInfoList;
}

std::unordered_map<uint32_t, std::string> GetKeyboundCommands()
{
    return KeyboundCommands;
}
}