#pragma once
#include <map>
#include <string>
#include <vector>
#include <windows.h>
#include "imgui/imgui.h"

// --- Global Data ---
// المفتاح: الزر الأصلي، القيمة: الزر الجديد
extern std::map<int, int> g_key_mappings; 
extern std::map<int, float> g_key_states; // للأنيميشن
extern int g_key_press_count;
extern int g_last_vk_code;
extern std::string g_last_key_name;

// --- Remapping Logic States ---
enum class AppState {
    Dashboard,
    Remapping_WaitForOriginal, // انتظار الزر المراد تغييره
    Remapping_WaitForTarget    // انتظار الزر الجديد
};
extern AppState g_app_state;

// --- Functions ---
void InstallHook();
void UninstallHook();
void UpdateAnimationState();
void RenderUI();

// دوال مساعدة لحفظ وتحميل الإعدادات (يمكن إضافتها لاحقاً)
std::string GetKeyName(int vkCode);
