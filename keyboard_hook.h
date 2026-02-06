#pragma once
#include <map>
#include <string>

// متغيرات مشتركة بين الواجهة والهوك
extern std::map<int, int> g_key_mappings;
extern std::map<int, int> g_key_states; // لتخزين حالة الأنيميشن
extern int g_key_press_count;
extern int g_last_vk_code; // تم التغيير لتمرير الكود بدلاً من الاسم

void InstallHook();
void UninstallHook();
void AddOrUpdateMapping(int from, int to);
void UpdateAnimationState();
