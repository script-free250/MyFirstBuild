#pragma once
#include <map>
#include <string>
#include <vector>
#include <windows.h>
#include "imgui/imgui.h"

// --- Data Structures ---
enum class ActionType {
    SinglePress, 
    RapidFire    
};

struct KeyAction {
    int targetVk;       
    ActionType type;    
    int delayMs;        
    DWORD lastExecutionTime = 0; 
};

// --- Global Data ---
extern std::map<int, std::vector<KeyAction>> g_complex_mappings;
extern std::map<int, bool> g_physical_key_status; 
extern std::map<int, float> g_key_states;
extern int g_last_vk_code;
extern std::string g_last_key_name;

// --- App States (Wizard Flow) ---
enum class AppState {
    Dashboard,
    Wizard_WaitForOriginal, 
    Wizard_WaitForTarget,
    Wizard_AskSwap,         // <--- خطوة جديدة: السؤال عن التبديل
    Wizard_Configure        
};
extern AppState g_app_state;

// --- Functions ---
void InstallHooks();
void UninstallHooks();
void ProcessRapidFire();
void UpdateAnimationState();
void RenderUI();

// Helpers
std::string GetKeyNameSmart(int vkCode);
