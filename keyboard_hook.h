#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>

// إعدادات البرنامج العامة
struct AppSettings {
    bool EnableRemap = true;
    bool EnableAutoClicker = false;
    bool RandomDelay = false;
    int MinDelay = 50;
    int MaxDelay = 100;
    int ClickMouseButton = 0;      // 0=Left, 1=Right, 2=Middle
    int ToggleKey = VK_F8;
    bool PlaySounds = true;
    bool PanicMode = false;
    int PanicKey = VK_END;
    bool AlwaysOnTop = false;
};

// متغيرات عامة
extern AppSettings g_Settings;
extern std::map<int, int> g_key_mappings;
extern bool g_HookActive;
extern bool g_AutoClickerRunning; // <--- تمت إضافة هذا السطر لحل المشكلة
extern int g_CurrentCPS;
extern std::vector<std::string> g_Logs;

// الدوال
void InstallHook();
void UninstallHook();
void AddLog(const std::string& msg);
void ToggleAutoClicker();
