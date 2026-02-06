#pragma once
#include <map>
#include <string>
#include <vector>
#include <windows.h>
#include "imgui/imgui.h"

// --- Global Data ---
// نستخدم أكواد VK الموحدة (تشمل الماوس والكيبورد)
extern std::map<int, int> g_key_mappings; 
extern std::map<int, float> g_key_states; 
extern int g_key_press_count;
extern int g_last_vk_code;
extern std::string g_last_key_name;

// --- App States ---
enum class AppState {
    Dashboard,
    Remapping_WaitForOriginal, 
    Remapping_WaitForTarget    
};
extern AppState g_app_state;

// --- Functions ---
void InstallHooks(); // تم تغيير الاسم ليعبر عن الاثنين
void UninstallHooks();
void UpdateAnimationState();
void RenderUI();

// Helper
std::string GetKeyNameSmart(int vkCode);
