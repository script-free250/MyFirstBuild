#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>

// Global structures needed for settings
struct AppSettings {
    bool EnableRemap = true;
    bool EnableAutoClicker = false;
    bool RandomDelay = false;
    int MinDelay = 50;
    int MaxDelay = 100;
    int ClickCount = 0; // 0 = infinite
    int ClickMouseButton = 0; // 0=Left, 1=Right, 2=Middle
    int ToggleKey = VK_F8;
    bool PlaySounds = true;
    bool PanicMode = false;
    int PanicKey = VK_END;
};

extern AppSettings g_Settings;
extern std::map<int, int> g_key_mappings;
extern bool g_HookActive;
extern int g_CurrentCPS;
extern std::vector<std::string> g_Logs;

// Function declarations
void InstallHook();
void UninstallHook();
void AddLog(const std::string& msg);
void ToggleAutoClicker();
