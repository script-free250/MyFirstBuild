#include "keyboard_hook.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

HHOOK hKeyboardHook = NULL;
HHOOK hMouseHook = NULL;

std::map<int, int> g_key_mappings;
AppSettings g_Settings;
bool g_HookActive = false;
bool g_AutoClickerRunning = false;
int g_CurrentCPS = 0;
std::vector<std::string> g_Logs;

// Helper to log messages
void AddLog(const std::string& msg) {
    if (g_Logs.size() > 100) g_Logs.erase(g_Logs.begin());
    g_Logs.push_back(msg);
}

// Auto Clicker Thread Function
void AutoClickerThread() {
    static int clicksThisSecond = 0;
    static auto lastTime = std::chrono::steady_clock::now();

    while (g_AutoClickerRunning && !g_Settings.PanicMode) {
        // Calculate logic for mouse button
        DWORD flagsDown = MOUSEEVENTF_LEFTDOWN;
        DWORD flagsUp = MOUSEEVENTF_LEFTUP;
        
        if (g_Settings.ClickMouseButton == 1) { flagsDown = MOUSEEVENTF_RIGHTDOWN; flagsUp = MOUSEEVENTF_RIGHTUP; }
        else if (g_Settings.ClickMouseButton == 2) { flagsDown = MOUSEEVENTF_MIDDLEDOWN; flagsUp = MOUSEEVENTF_MIDDLEUP; }

        // Send Input (Injected)
        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = flagsDown;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = flagsUp;

        SendInput(2, inputs, sizeof(INPUT));

        clicksThisSecond++;
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count() >= 1) {
            g_CurrentCPS = clicksThisSecond;
            clicksThisSecond = 0;
            lastTime = now;
        }

        // Delay logic
        int delay = g_Settings.MinDelay;
        if (g_Settings.RandomDelay) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> distr(g_Settings.MinDelay, g_Settings.MaxDelay);
            delay = distr(gen);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    g_CurrentCPS = 0;
}

void ToggleAutoClicker() {
    if (g_AutoClickerRunning) {
        g_AutoClickerRunning = false;
        AddLog("[System] Auto Clicker Stopped.");
        if(g_Settings.PlaySounds) Beep(500, 100);
    } else {
        g_AutoClickerRunning = true;
        AddLog("[System] Auto Clicker Started.");
        if(g_Settings.PlaySounds) Beep(1000, 100);
        std::thread(AutoClickerThread).detach();
    }
}

// Keyboard Hook Procedure
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;

        // Ignore injected keys to prevent loops/lag
        if (pKey->flags & LLKHF_INJECTED) {
            return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
        }

        // Handle Panic Key
        if (wParam == WM_KEYDOWN && pKey->vkCode == (DWORD)g_Settings.PanicKey) {
            g_Settings.PanicMode = true;
            g_AutoClickerRunning = false;
            UninstallHook(); // Safe exit
            AddLog("[URGENT] PANIC MODE ACTIVATED!");
            return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
        }

        // Handle Toggle Key
        if (wParam == WM_KEYDOWN && pKey->vkCode == (DWORD)g_Settings.ToggleKey) {
            ToggleAutoClicker();
            return 1; // Block key if needed, or return CallNextHookEx to pass it
        }

        // Handle Remapping
        if (g_Settings.EnableRemap && wParam == WM_KEYDOWN) {
            if (g_key_mappings.find(pKey->vkCode) != g_key_mappings.end()) {
                int newKey = g_key_mappings[pKey->vkCode];
                
                // Simulate new key
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = newKey;
                SendInput(1, &input, sizeof(INPUT));
                
                // Log and Block original key
                AddLog("[Remap] Key " + std::to_string(pKey->vkCode) + " -> " + std::to_string(newKey));
                return 1; 
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void InstallHook() {
    if (!hKeyboardHook) {
        hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
        g_HookActive = (hKeyboardHook != NULL);
        if (g_HookActive) AddLog("Hooks Installed Successfully.");
        else AddLog("Failed to install Hooks!");
    }
}

void UninstallHook() {
    if (hKeyboardHook) {
        UnhookWindowsHookEx(hKeyboardHook);
        hKeyboardHook = NULL;
        g_HookActive = false;
        AddLog("Hooks Uninstalled.");
    }
}
