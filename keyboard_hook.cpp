#include <windows.h>
#include "keyboard_hook.h"
#include <thread>
#include <chrono>

// المتغيرات
std::map<int, int> g_key_mappings;
std::map<int, KeyStats> g_key_stats;
std::vector<Macro> g_macros;
bool g_game_mode_active = false;
bool g_turbo_mode_active = false;
bool g_sound_enabled = false;
int g_last_pressed_key = 0;
static HHOOK g_keyboardHook = NULL;

// دالة لتشغيل الماكرو في خيط منفصل لتجنب اللاج
void PlayMacro(const std::vector<int>& sequence) {
    std::thread([sequence]() {
        for (int vk : sequence) {
            INPUT input = { 0 };
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = vk;
            SendInput(1, &input, sizeof(INPUT));
            Sleep(50); // تأخير بسيط بين الضغطات
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            Sleep(50);
        }
    }).detach();
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

        [...](asc_slot://start-slot-17)// 🟢 الحل الجذري لمشكلة اللاج: تجاهل الضغطات الوهمية الناتجة عن البرنامج نفسه
        if (p->flags & LLKHF_INJECTED) {
            return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
        }

        [...](asc_slot://start-slot-19)if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            g_last_pressed_key = p->vkCode;
            g_key_stats[p->vkCode].pressCount++;

            [...](asc_slot://start-slot-21)if (g_sound_enabled) {
                Beep(500, 50); // صوت بسيط (يمكن استبداله بـ PlaySound)
            }

            [...](asc_slot://start-slot-23)// Game Mode: تعطيل زر الويندوز
            if (g_game_mode_active && (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN)) {
                return 1;
            }

            [...](asc_slot://start-slot-25)// تشغيل الماكرو
            for (const auto& macro : g_macros) {
                if (macro.triggerKey == p->vkCode) {
                    PlayMacro(macro.sequence);
                    return 1; // منع المفتاح الأصلي
                }
            }

            [...](asc_slot://start-slot-27)// Remapping (تغيير الأزرار)
            if (g_key_mappings.count(p->vkCode)) {
                int target = g_key_mappings[p->vkCode];
                if (target == -1) return 1; // زر معطل

                // إرسال الزر الجديد
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = target;
                SendInput(1, &input, sizeof(INPUT));
                return 1; // إلغاء الزر الأصلي
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}
void InstallHook() {
    if (!g_keyboardHook)
        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
}

void UninstallHook() {
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }
}

// دوال وهمية للحفظ (يمكنك تنفيذها باستخدام fstream أو مكتبة JSON)
void SaveSettings() { /* implementation needed */ }
void LoadSettings() { /* implementation needed */ }
