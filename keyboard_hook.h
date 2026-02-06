#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>

// Structures
struct KeyStats {
    int pressCount = 0;
};

struct Macro {
    std::string name;
    int triggerKey;
    std::vector<int> sequence;
};

// Global Variables
extern std::map<int, int> g_key_mappings;
extern std::map<int, KeyStats> g_key_stats;
extern std::vector<Macro> g_macros;
extern bool g_game_mode_active;
extern bool g_turbo_mode_active;
extern bool g_sound_enabled;
extern int g_last_pressed_key;

// Functions
void InstallHook();
void UninstallHook();
void SaveSettings();
void LoadSettings();
