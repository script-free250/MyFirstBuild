#include "keyboard_hook.h"
#include <chrono>

HHOOK g_keyboardHook = NULL;
HHOOK g_mouseHook = NULL; // Added Mouse Hook for CPS

std::map<int, int> g_key_mappings;
std::map<int, int> g_key_states;
bool g_is_active = true;

// CPS Calculation Variables
int g_cps_counter = 0;
float g_current_cps = 0.0f;
auto g_last_cps_check = std::chrono::high_resolution_clock::now();

// Keyboard Hook Procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

        // --- FIX FOR LAG: Ignore Injected (Simulated) Events ---
        if (p->flags & LLKHF_INJECTED) {
            return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Visual feedback state
            g_key_states[p->vkCode] = 255; 

            // Master Toggle (e.g., F10 to toggle app)
            if (p->vkCode == VK_F10) {
                g_is_active = !g_is_active;
                return 1; // Consume F10
            }

            if (g_is_active && g_key_mappings.count(p->vkCode)) {
                int newVkCode = g_key_mappings[p->vkCode];
                
                // Send the NEW key
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = newVkCode;
                // Important: Setup flags so we don't block input unnecessarily
                SendInput(1, &input, sizeof(INPUT));
                
                return 1; // Block the ORIGINAL key
            }
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
             if (g_is_active && g_key_mappings.count(p->vkCode)) {
                int newVkCode = g_key_mappings[p->vkCode];
                INPUT input = { 0 };
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

// Mouse Hook Procedure (For CPS Test)
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN) {
            g_cps_counter++;
        }
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

void InstallHooks() {
    if (!g_keyboardHook) g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!g_mouseHook) g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
}

void UninstallHooks() {
    if (g_keyboardHook) UnhookWindowsHookEx(g_keyboardHook);
    if (g_mouseHook) UnhookWindowsHookEx(g_mouseHook);
    g_keyboardHook = NULL;
    g_mouseHook = NULL;
}

void UpdateHooksLogic() {
    // Calculate CPS every second
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = now - g_last_cps_check;
    if (elapsed.count() >= 1.0f) {
        g_current_cps = (float)g_cps_counter / elapsed.count();
        g_cps_counter = 0;
        g_last_cps_check = now;
    }

    // Update Visual Animations (Fade out keys)
    for (auto it = g_key_states.begin(); it != g_key_states.end(); ) {
        it->second -= 10; // Faster fade
        if (it->second <= 0) it = g_key_states.erase(it);
        else ++it;
    }
}
