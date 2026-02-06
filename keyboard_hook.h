#pragma once
#include <windows.h>
#include <map>
#include <string>

// Global variables for UI and Logic
extern std::map<int, int> g_key_mappings;
extern std::map<int, int> g_key_states; // For visual feedback
extern bool g_is_active; // Master toggle
extern int g_cps_counter;
extern float g_current_cps;

// Hook Functions
void InstallHooks();
void UninstallHooks();
void UpdateHooksLogic(); // Called from main loop for timers/CPS calculations
