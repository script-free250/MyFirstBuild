#include "ui.h"
#include "imgui/imgui.h"
#include <Windows.h>
#include <mmsystem.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <random>
#include <thread>
#include <atomic>
#include <mutex>

#pragma comment(lib, "winmm.lib")

// --- Constants ---
#ifndef MAGIC_SIGNATURE
#define MAGIC_SIGNATURE 0xFF00AA55
#endif

// --- Global Config ---
bool g_jitter_enabled = false;
int g_jitter_intensity = 2;
bool g_sound_enabled = false;
int g_max_cps_limit = 20; 
int g_current_theme = 0; 

// --- CPS Test ---
std::atomic<int> g_cps_clicks = 0;
double g_cps_result = 0.0;
DWORD g_cps_start_time = 0;
bool g_cps_active = false;

// --- State & Threading ---
struct ClickState {
    bool isDown;
    std::chrono::steady_clock::time_point nextActionTime;
};

std::map<int, std::vector<KeyAction>> g_complex_mappings;
std::map<int, bool> g_physical_key_status;
std::map<int, float> g_key_states;
std::map<int, ClickState> g_rapid_states;

// Mutex to protect shared maps
std::mutex g_map_mutex;

// Threading Control
std::atomic<bool> g_logic_running = false;
std::thread g_logic_thread;

int g_last_vk_code = 0;
std::string g_last_key_name = "None";
AppState g_app_state = AppState::Dashboard;

// Wizard
int g_wiz_source_key = -1;
int g_wiz_target_key = -1;
int g_wiz_delay = 50;
bool g_wiz_is_rapid = false;
bool g_wiz_swap_keys = false;

HHOOK g_hKeyboardHook = NULL;
HHOOK g_hMouseHook = NULL;

// --- Helper Functions ---
int GetRandomInt(int min, int max) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

std::string GetKeyNameSmart(int vkCode) {
    switch (vkCode) {
    case VK_LBUTTON: return "Left Click";
    case VK_RBUTTON: return "Right Click";
    case VK_MBUTTON: return "Middle Click";
    case VK_XBUTTON1: return "Mouse Side 1";
    case VK_XBUTTON2: return "Mouse Side 2";
    }
    char keyNameBuffer[128];
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    if (GetKeyNameTextA(scanCode << 16, keyNameBuffer, 128)) return std::string(keyNameBuffer);
    return "VK_" + std::to_string(vkCode);
}

void ExecuteSendInput(int vkCode, bool isUp) {
    INPUT input = {};
    if (vkCode >= VK_LBUTTON && vkCode <= VK_XBUTTON2) {
        input.type = INPUT_MOUSE;
        if (vkCode == VK_LBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
        else if (vkCode == VK_RBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
        else if (vkCode == VK_MBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN;
        else if (vkCode == VK_XBUTTON1) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON1; }
        else if (vkCode == VK_XBUTTON2) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON2; }
        
        if (!isUp && g_jitter_enabled) {
            input.mi.dx = GetRandomInt(-g_jitter_intensity, g_jitter_intensity);
            input.mi.dy = GetRandomInt(-g_jitter_intensity, g_jitter_intensity);
            input.mi.dwFlags |= MOUSEEVENTF_MOVE;
        }
        input.mi.dwExtraInfo = MAGIC_SIGNATURE;
    } else {
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = isUp ? KEYEVENTF_KEYUP : 0;
        input.ki.dwExtraInfo = MAGIC_SIGNATURE;
    }
    SendInput(1, &input, sizeof(INPUT));
    
    // Sound Limiter (Prevents audio lag)
    if (!isUp && g_sound_enabled) {
        static DWORD lastSoundTime = 0;
        DWORD now = GetTickCount();
        if (now - lastSoundTime > 50) { // Max 20 sounds per sec
            PlaySoundA("SystemDefault", NULL, SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
            lastSoundTime = now;
        }
    }
}

// --- Threaded Logic (NO LAG) ---
void LogicThreadFunc() {
    while (g_logic_running) {
        auto now = std::chrono::steady_clock::now();
        bool didWork = false;

        {
            // Lock map for thread safety
            std::lock_guard<std::mutex> lock(g_map_mutex);
            
            for (auto& [sourceKey, actions] : g_complex_mappings) {
                if (g_physical_key_status[sourceKey]) { // User is holding key
                    for (auto& action : actions) {
                        if (action.type == ActionType::RapidFire) {
                            
                            // Init state
                            if (g_rapid_states.find(action.targetVk) == g_rapid_states.end()) {
                                g_rapid_states[action.targetVk] = { false, now };
                            }

                            ClickState& state = g_rapid_states[action.targetVk];

                            if (now >= state.nextActionTime) {
                                if (!state.isDown) {
                                    // PRESS
                                    ExecuteSendInput(action.targetVk, false);
                                    state.isDown = true;
                                    // Hold time 20-40ms
                                    state.nextActionTime = now + std::chrono::milliseconds(GetRandomInt(20, 40));
                                    
                                    // Update visual state (safe copy?)
                                    // Direct access to g_key_states is risky for UI, but float write is atomic-ish on x86
                                    // We will skip visual update here to be 100% safe, or use atomic
                                } 
                                else {
                                    // RELEASE
                                    ExecuteSendInput(action.targetVk, true);
                                    state.isDown = false;
                                    
                                    int delay = action.delayMs;
                                    // Safety Clamp
                                    if (delay < 15) delay = 15; // Minimum 15ms to prevent freeze

                                    if (g_max_cps_limit > 0) {
                                        int minDelay = 1000 / g_max_cps_limit;
                                        if (delay < minDelay) delay = minDelay;
                                    }
                                    state.nextActionTime = now + std::chrono::milliseconds(delay + GetRandomInt(0, 5));
                                }
                                didWork = true;
                            }
                        }
                    }
                } else {
                    // Cleanup stuck keys
                    for (auto& action : actions) {
                        if (action.type == ActionType::RapidFire) {
                            if (g_rapid_states.count(action.targetVk) && g_rapid_states[action.targetVk].isDown) {
                                 ExecuteSendInput(action.targetVk, true);
                                 g_rapid_states[action.targetVk].isDown = false;
                            }
                        }
                    }
                }
            }
        }

        // Sleep to save CPU
        if (didWork) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

// Called by Main Loop (for visuals only)
void ProcessInputLogic() {
    std::lock_guard<std::mutex> lock(g_map_mutex);
    // Animation Decay
    for (auto& pair : g_key_states) {
        if (pair.second > 0.0f) pair.second -= 0.05f;
        if (pair.second < 0.0f) pair.second = 0.0f;
    }
}

// --- Hook Logic ---
bool HandleHookLogic(int vkCode, bool isUp) {
    if (!isUp) {
        if (g_app_state == AppState::Wizard_WaitForOriginal) {
            g_wiz_source_key = vkCode;
            g_app_state = AppState::Wizard_WaitForTarget;
            return true; 
        }
        else if (g_app_state == AppState::Wizard_WaitForTarget) {
            g_wiz_target_key = vkCode;
            g_app_state = AppState::Wizard_Configure;
            return true; 
        }
    }

    // Lock for map access
    std::unique_lock<std::mutex> lock(g_map_mutex, std::try_to_lock);
    if (lock.owns_lock()) {
        if (g_complex_mappings.count(vkCode)) {
            g_physical_key_status[vkCode] = !isUp;
            
            auto& actions = g_complex_mappings[vkCode];
            for (auto& action : actions) {
                if (action.type == ActionType::SinglePress) {
                    // Send Input immediately for Single Press
                    ExecuteSendInput(action.targetVk, isUp);
                    if (!isUp) g_key_states[action.targetVk] = 1.0f;
                }
            }
            return true; // Block original
        }
    }

    if (!isUp) {
        g_key_states[vkCode] = 1.0f;
        g_last_vk_code = vkCode;
        g_last_key_name = GetKeyNameSmart(vkCode);
        
        if (g_cps_active) {
            if (g_cps_start_time == 0) g_cps_start_time = GetTickCount();
            g_cps_clicks++;
            DWORD elapsed = GetTickCount() - g_cps_start_time;
            if (elapsed >= 1000) {
                g_cps_result = (double)g_cps_clicks / (elapsed / 1000.0);
                g_cps_clicks = 0;
                g_cps_start_time = GetTickCount();
            }
        }
    }
    return false; 
}

// --- Callbacks ---
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        if (p->dwExtraInfo == MAGIC_SIGNATURE) return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        if (HandleHookLogic(p->vkCode, isUp)) return 1;
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* p = (MSLLHOOKSTRUCT*)lParam;
        if (p->dwExtraInfo == MAGIC_SIGNATURE) return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
        int vk = 0; bool isUp = false;
        if (wParam == WM_LBUTTONDOWN) vk = VK_LBUTTON;
        else if (wParam == WM_LBUTTONUP) { vk = VK_LBUTTON; isUp = true; }
        else if (wParam == WM_RBUTTONDOWN) vk = VK_RBUTTON;
        else if (wParam == WM_RBUTTONUP) { vk = VK_RBUTTON; isUp = true; }
        else if (wParam == WM_MBUTTONDOWN) vk = VK_MBUTTON;
        else if (wParam == WM_MBUTTONUP) { vk = VK_MBUTTON; isUp = true; }
        else if (wParam == WM_XBUTTONDOWN) vk = (HIWORD(p->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        else if (wParam == WM_XBUTTONUP) { vk = (HIWORD(p->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2; isUp = true; }
        if (vk != 0 && HandleHookLogic(vk, isUp)) return 1;
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void InstallHooks() {
    if (!g_hKeyboardHook) g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (!g_hMouseHook) g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
    
    // Start Logic Thread
    if (!g_logic_running) {
        g_logic_running = true;
        g_logic_thread = std::thread(LogicThreadFunc);
    }
}

void UninstallHooks() {
    // Stop Logic Thread
    if (g_logic_running) {
        g_logic_running = false;
        if (g_logic_thread.joinable()) g_logic_thread.join();
    }

    if (g_hKeyboardHook) { UnhookWindowsHookEx(g_hKeyboardHook); g_hKeyboardHook = NULL; }
    if (g_hMouseHook) { UnhookWindowsHookEx(g_hMouseHook); g_hMouseHook = NULL; }
}

void ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    if (g_current_theme == 0) { // Dark
        ImGui::StyleColorsDark();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
    } else if (g_current_theme == 1) { // Light
        ImGui::StyleColorsLight();
    } else { // Matrix Green
        ImGui::StyleColorsDark();
        style.Colors[ImGuiCol_Text] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.05f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.3f, 0.0f, 1.0f);
    }
}

void RenderUI() {
    ApplyTheme();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Pro 
