#pragma once
#include <map>
#include <string>
#include <windows.h>
#include <vector>

// ==========================================
// Global Settings & Features Variables
// ==========================================
extern bool g_enable_remap;      // تفعيل/إيقاف التخصيص
extern bool g_play_sound;        // صوت عند الضغط
extern bool g_turbo_mode;        // وضع التكرار السريع
extern bool g_block_key_mode;    // وضع حظر المفاتيح
extern bool g_map_to_mouse;      // تحويل الكيبورد لماوس
extern bool g_always_on_top;     // النافذة فوق الجميع
extern float g_window_opacity;   // شفافية النافذة
extern int g_key_press_count;    // عداد الضغطات

// ==========================================
// State Variables
// ==========================================
extern std::map<int, int> g_key_mappings;
extern std::map<int, int> g_key_states; // For visual animations
extern int g_last_vk_code;
extern std::string g_last_key_name;

// حالة عملية التخصيص
enum class RemapState { None, WaitingForFrom, WaitingForTo };
extern RemapState g_remap_state;

// ==========================================
// Functions
// ==========================================
void InstallHook();
void UninstallHook();
void AddOrUpdateMapping(int from, int to);
void UpdateAnimationState();
void ResetAll();
