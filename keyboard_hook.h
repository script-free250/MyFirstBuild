#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>

// الهياكل
struct KeyStats {
    int pressCount = 0;
};

struct Macro {
    std::string name;
    int triggerKey;
    std::vector<int> sequence;
};

// المتغيرات العامة
extern std::map<int, int> g_key_mappings;
extern std::map<int, KeyStats> g_key_stats;
extern std::vector<Macro> g_macros;
extern bool g_game_mode_active;
extern bool g_turbo_mode_active;
extern bool g_sound_enabled;
extern int g_last_pressed_key;

// الدوال (تم تعديل الأسماء لتطابق main.cpp)
void InstallHooks();       // كان اسمها InstallHook
void UninstallHooks();     // كان اسمها UninstallHook
void ProcessInputLogic();  // دالة جديدة مطلوبة
void SaveSettings();
void LoadSettings();
