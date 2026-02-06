#include <windows.h>
#include <stdio.h>
#include "keyboard_hook.h"

// ==========================================
// Global Variable Definitions
// ==========================================
std::map<int, int> g_key_mappings;
std::map<int, int> g_key_states;
int g_key_press_count = 0;
int g_last_vk_code = 0;
std::string g_last_key_name = "None";
RemapState g_remap_state = RemapState::None;

// Feature Defaults
bool g_enable_remap = true;
bool g_play_sound = false;
bool g_turbo_mode = false;
bool g_block_key_mode = false;
bool g_map_to_mouse = false;
bool g_always_on_top = false;
float g_window_opacity = 1.0f;

static HHOOK g_keyboardHook = NULL;

// ==========================================
// Low Level Keyboard Hook Procedure
// ==========================================
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

        // [CRITICAL FIX] 
        // تجاهل الأحداث التي تم إرسالها بواسطة البرنامج نفسه (Injected)
        // هذا يمنع الدخول في حلقة مفرغة ويحل مشكلة اللاج تماماً
        if (p->flags & LLKHF_INJECTED) {
            return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            g_key_press_count++;
            g_last_vk_code = p->vkCode;

            // 1. Get Key Name safely
            char key_name[64];
            int scanCode = (p->scanCode & 0xFF);
            if (p->flags & LLKHF_EXTENDED) scanCode |= 0x100;
            if (GetKeyNameTextA(scanCode << 16, key_name, sizeof(key_name)) == 0) {
                wsprintfA(key_name, "VK_%d", p->vkCode);
            }
            g_last_key_name = key_name;

            // 2. Update Visual Animation
            g_key_states[p->vkCode] = 255;

            // 3. Feature: Sound Effect
            if (g_play_sound) {
                // صوت خفيف جداً وغير مزعج لعدم تعطيل الأداء
                Beep(800, 10); 
            }

            // 4. Setup Mode Blocking
            if (g_remap_state != RemapState::None) {
                return 1; // منع المفتاح أثناء الإعداد
            }

            // 5. Global Disable Check
            if (!g_enable_remap) {
                return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
            }

            // 6. Mapping Logic
            if (g_key_mappings.count(p->vkCode)) {
                int target = g_key_mappings[p->vkCode];

                // Feature: Block Key
                if (g_block_key_mode || target == -1) {
                    return 1; // Block original
                }

                // Feature: Map to Mouse (Example: Left Click)
                if (g_map_to_mouse && target == VK_LBUTTON) {
                    INPUT input = { 0 };
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput(1, &input, sizeof(INPUT));
                    return 1;
                }

                // Standard Keyboard Remap
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = target;
                input.ki.wScan = MapVirtualKey(target, MAPVK_VK_TO_VSC);
                // Note: We don't set KEYUP here
                SendInput(1, &input, sizeof(INPUT));

                // Feature: Turbo Mode (Repeat extra time)
                if (g_turbo_mode) {
                    SendInput(1, &input, sizeof(INPUT));
                }

                return 1; // Block original key
            }
        }
        
        // Handle Key Up to prevent "stuck" keys
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
             if (g_enable_remap && g_key_mappings.count(p->vkCode)) {
                int target = g_key_mappings[p->vkCode];
                
                if (g_block_key_mode || target == -1) return 1;

                if (g_map_to_mouse && target == VK_LBUTTON) {
                    INPUT input = { 0 };
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &input, sizeof(INPUT));
                    return 1;
                }

                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = target;
                input.ki.wScan = MapVirtualKey(target, MAPVK_VK_TO_VSC);
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
                return 1;
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

void AddOrUpdateMapping(int from, int to) {
    if (from != 0 && to != 0) {
        g_key_mappings[from] = to;
    }
}

void UpdateAnimationState() {
    for (auto it = g_key_states.begin(); it != g_key_states.end(); ) {
        it->second -= 15; // سرعة اختفاء الألوان
        if (it->second <= 0) {
            it = g_key_states.erase(it);
        } else {
            ++it;
        }
    }
}

void ResetAll() {
    g_key_mappings.clear();
    g_key_press_count = 0;
    g_block_key_mode = false;
    g_turbo_mode = false;
}
