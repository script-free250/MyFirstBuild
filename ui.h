#pragma once
#include <map>
#include <string>
#include <windows.h>
#include "imgui/imgui.h"

// Shared Global Variables
extern std::map<int, int> g_key_mappings;
extern std::map<int, float> g_key_states; // Float for smooth animation (0.0 to 1.0)
extern int g_key_press_count;
extern int g_last_vk_code;
extern std::string g_last_key_name;

// Remapping State
enum class RemapState {
    None,
    WaitingForFrom,
    WaitingForTo
};
extern RemapState g_remap_state;

// Functions
void InstallHook();
void UninstallHook();
void UpdateAnimationState();
void RenderUI();
