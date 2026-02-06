#pragma once
#include <map>
#include <string>
#include <vector>
#include <windows.h>
#include "imgui/imgui.h"

// التوقيع السري لتمييز ضغطات برنامجنا (يمنع التعليق نهائياً)
#define MAGIC_SIGNATURE 0xFF99

enum class ActionType { SinglePress, RapidFire };

struct KeyAction {
    int targetVk;       
    ActionType type;    
    int delayMs;        
    DWORD lastExecutionTime = 0; 
};

// Global Data
extern std::map<int, std::vector<KeyAction>> g_complex_mappings;
extern std::map<int, bool> g_physical_key_status; // حالة الزر الحقيقية (إصبع المستخدم)
extern std::map<int, float> g_key_states;
extern int g_last_vk_code;
extern std::string g_last_key_name;

enum class AppState {
    Dashboard, Wizard_WaitForOriginal, Wizard_WaitForTarget, Wizard_Configure        
};
extern AppState g_app_state;

void InstallHooks();
void UninstallHooks();
void ProcessInputLogic(); 
void RenderUI();

std::string GetKeyNameSmart(int vkCode);
