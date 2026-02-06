#pragma once
#include <map>
#include <string>
#include <vector>
#include <windows.h>
#include "imgui/imgui.h"

// --- الهيكل الجديد للبيانات ---
enum class ActionType {
    SinglePress, // ضغطة عادية (مرة واحدة)
    RapidFire    // تكرار مستمر (Spam)
};

struct KeyAction {
    int targetVk;       // الزر المراد تنفيذه
    ActionType type;    // نوع العملية
    int delayMs;        // السرعة بالمللي ثانية (للـ Rapid فقط)
    
    // متغيرات داخلية للتحكم في الوقت
    DWORD lastExecutionTime = 0; 
};

// --- Global Data ---
// المفتاح (Source Key) -> قائمة من الإجراءات (Vector of Actions)
extern std::map<int, std::vector<KeyAction>> g_complex_mappings;

// لتتبع حالة الأزرار الفيزيائية (هل المستخدم ضاغط الآن؟)
extern std::map<int, bool> g_physical_key_status; 

extern std::map<int, float> g_key_states; // For Visualizer
extern int g_last_vk_code;
extern std::string g_last_key_name;

// --- App States (Wizard Flow) ---
enum class AppState {
    Dashboard,
    Wizard_WaitForOriginal, // خطوة 1: اضغط الزر الأصلي
    Wizard_WaitForTarget,   // خطوة 2: اضغط الزر الجديد
    Wizard_Configure        // خطوة 3: إعدادات السرعة والتكرار
};
extern AppState g_app_state;

// --- Functions ---
void InstallHooks();
void UninstallHooks();
void ProcessRapidFire(); // دالة جديدة يجب استدعاؤها باستمرار
void RenderUI();

// Helpers
std::string GetKeyNameSmart(int vkCode);
