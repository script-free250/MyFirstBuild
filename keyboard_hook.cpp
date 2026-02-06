#include "keyboard_hook.h"
#include <thread>
#include <chrono>

// تعريف LLKHF_INJECTED يدوياً لتجنب الأخطاء
#ifndef LLKHF_INJECTED
#define LLKHF_INJECTED 0x00000010
#endif

// تهيئة المتغيرات
std::map<int, int> g_key_mappings;
std::map<int, KeyStats> g_key_stats;
std::vector<Macro> g_macros;
bool g_game_mode_active = false;
bool g_turbo_mode_active = false;
bool g_sound_enabled = false;
int g_last_pressed_key = 0;

static HHOOK g_keyboardHook = NULL;

void PlayMacro(std::vector<int> sequence) {
    std::thread([sequence]() {
        for (int vk : sequence) {
            INPUT input = { 0 };
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = (WORD)vk;
            SendInput(1, &input, sizeof(INPUT));
            Sleep(50); 
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            Sleep(50);
        }
    }).detach();
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

        if (p->flags & LLKHF_INJECTED) {
            return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            g_last_pressed_key = p->vkCode;
            g_key_stats[p->vkCode].pressCount++;

            if (g_sound_enabled) Beep(500, 50);

            if (g_game_mode_active && (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN)) return 1;

            for (const auto& macro : g_macros) {
                if (macro.triggerKey == p->vkCode) {
                    PlayMacro(macro.sequence);
                    return 1; 
                }
            }

            if (g_key_mappings.count(p->vkCode)) {
                int target = g_key_mappings[p->vkCode];
                if (target == -1) return 1;
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = (WORD)target;
                SendInput(1, &input, sizeof(INPUT));
                return 1;
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

// تعديل الأسماء هنا لتطابق Header و Main
void InstallHooks() {
    if (!g_keyboardHook)
        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
}

void UninstallHooks() {
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }
}

// الدالة المفقودة
void ProcessInputLogic() {
    // يمكن وضع منطق إضافي هنا يتم تنفيذه كل فريم
    // حالياً نتركها فارغة لأن المعالجة تتم داخل الـ Hook
}

void SaveSettings() {}
void LoadSettings() {}
