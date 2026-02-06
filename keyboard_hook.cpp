#pragma once
#include <map>
#include <string>

// متغيرات مشتركة بين الواجهة والهوك
extern std::map<int, int> g_key_mappings;
extern std::map<int, int> g_key_states;
extern int g_key_press_count;
extern std::string g_last_key_name;

void InstallHook();
void UninstallHook();
void AddOrUpdateMapping(int from, int to);
void UpdateAnimationState();
