#include <windows.h>
#include "keyboard_hook.h"
#include <thread>
#include <chrono>

// Define LLKHF_INJECTED manually if missing to prevent errors
#ifndef LLKHF_INJECTED
#define LLKHF_INJECTED 0x00000010
#endif

// Variables Initialization
std::map<int, int> g_key_mappings;
std::map<int, KeyStats> g_key_stats;
std::vector<Macro> g_macros;
bool g_game_mode_active = false;
bool g_turbo_mode_active = false;
bool g_sound_enabled = false;
int g_last_pressed_key = 0;

static HHOOK g_keyboardHook = NULL;

// Function to play macro in a separate thread
void PlayMacro(std::vector<int> sequence) {
    // We pass 'sequence' by value to the thread to ensure it has its own copy
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

        // FIX: Check for injected (artificial) keys to prevent infinite loops/lag
        if (p->flags & LLKHF_INJECTED) {
            return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            g_last_pressed_key = p->vkCode;
            g_key_stats[p->vkCode].pressCount++;

            if (g_sound_enabled) {
                Beep(500, 50);
            }

            // Game Mode: Disable Windows Keys
            if (g_game_mode_active && (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN)) {
                return 1;
            }

            // Macros
            for (const auto& macro : g_macros) {
                if (macro.triggerKey == p->vkCode) {
                    PlayMacro(macro.sequence);
                    return 1; 
                }
            }

            // Remapping
            if (g_key_mappings.count(p->vkCode)) {
                int target = g_key_mappings[p->vkCode];
                if (target == -1) return 1; // Disabled key

                // Send new key
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = (WORD)target;
                SendInput(1, &input, sizeof(INPUT));
                
                return 1; // Block original key
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

void SaveSettings() { 
    // Implementation for saving settings 
}

void LoadSettings() { 
    // Implementation for loading settings 
}
