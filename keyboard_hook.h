#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>

// أوضاع اختيار الأزرار (Binding Modes)
enum BindMode {
    BIND_NONE = 0,
    BIND_TOGGLE_KEY,
    BIND_REMAP_FROM,
    BIND_REMAP_TO
};

struct AppSettings {
    bool EnableRemap = true;
    bool EnableAutoClicker = false;
    bool RandomDelay = false;
    int MinDelay = 50;
    int MaxDelay = 100;
    int ClickMouseButton = 0;      // 0=Left, 1=Right, 2=Middle
    int ToggleKey = VK_F8;         // الزر الافتراضي
    bool PlaySounds = true;
    bool PanicMode = false;
    int PanicKey = VK_END;
    bool AlwaysOnTop = false;
};

// متغيرات عامة
extern AppSettings g_Settings;
extern std::map<int, int> g_key_mappings;
extern bool g_HookActive;
extern bool g_AutoClickerRunning;
extern int g_CurrentCPS;
extern std::vector<std::string> g_Logs;

// متغيرات نظام الـ Binding
extern BindMode g_CurrentBindMode;     // الوضع الحالي (هل ننتظر ضغط زر؟)
extern int g_TempKeyFrom;              // تخزين مؤقت للزر المراد تغييره

// الدوال
void InstallHook();
void UninstallHook();
void AddLog(const std::string& msg);
void ToggleAutoClicker();
std::string GetKeyName(int vkCode); // دالة لجلب اسم الزر كنص
