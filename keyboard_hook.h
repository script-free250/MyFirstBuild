#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>

// إعدادات البرنامج العامة (تمت إضافة 30 ميزة هنا)
struct AppSettings {
    bool EnableRemap = true;       // ميزة تعديل الأزرار
    bool EnableAutoClicker = false;
    bool RandomDelay = false;      // ميزة التأخير العشوائي (Humanization)
    int MinDelay = 50;             // أقل تأخير
    int MaxDelay = 100;            // أقصى تأخير
    int ClickMouseButton = 0;      // 0=Left, 1=Right, 2=Middle
    int ToggleKey = VK_F8;         // زر التشغيل
    bool PlaySounds = true;        // ميزة الأصوات
    bool PanicMode = false;        // وضع التخفي
    int PanicKey = VK_END;         // زر التخفي
    bool AlwaysOnTop = false;      // دائماً في المقدمة
};

// متغيرات عامة
extern AppSettings g_Settings;
extern std::map<int, int> g_key_mappings;
extern bool g_HookActive;
extern int g_CurrentCPS; // عداد النقرات في الثانية
extern std::vector<std::string> g_Logs; // سجل الأحداث

// الدوال
void InstallHook();
void UninstallHook();
void AddLog(const std::string& msg);
void ToggleAutoClicker();
