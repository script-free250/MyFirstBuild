#include <windows.h>
#include "keyboard_hook.h"

// تعريف المتغيرات المشتركة
std::map<int, int> g_key_mappings;
std::map<int, int> g_key_states;
int g_key_press_count = 0;
int g_last_vk_code = 0; // القيمة الافتراضية

static HHOOK g_keyboardHook = NULL;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // تحديث الواجهة
            g_key_press_count++;
            g_last_vk_code = p->vkCode; // --- هذا هو الإصلاح --- نمرر الكود مباشرة
            g_key_states[p->vkCode] = 255; // بدء الأنيميشن

            // تنفيذ التخصيص
            if (g_key_mappings.count(p->vkCode)) {
                int newVkCode = g_key_mappings[p->vkCode];
                INPUT input = {0};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = newVkCode;
                SendInput(1, &input, sizeof(INPUT));
                return 1; 
            }
        }
         if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (g_key_mappings.count(p->vkCode)) {
                int newVkCode = g_key_mappings[p->vkCode];
                INPUT input = {0};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = newVkCode;
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
                return 1;
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void InstallHook() {
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
}

void UninstallHook() {
    if (g_keyboardHook) UnhookWindowsHookEx(g_keyboardHook);
}

void AddOrUpdateMapping(int from, int to) {
    g_key_mappings[from] = to;
}

void UpdateAnimationState() {
    for (auto it = g_key_states.begin(); it != g_key_states.end(); ) {
        it->second -= 5; 
        if (it->second <= 0) {
            it = g_key_states.erase(it);
        } else {
            ++it;
        }
    }
}
