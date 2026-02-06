#include "ui.h"
#include "imgui/imgui.h"
#include <Windows.h>
#include <mmsystem.h> // For PlaySound
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <random>

// Link with winmm.lib for sound
#pragma comment(lib, "winmm.lib")

// --- Constants & Macros ---
#ifndef MAGIC_SIGNATURE
#define MAGIC_SIGNATURE 0xFF00AA55 // توقيع للتفريق بين نقرات البرنامج ونقرات المستخدم
#endif

// --- Structures ---
struct ClickState {
    bool isDown;
    DWORD nextActionTime;
};

// --- Global Config ---
bool g_jitter_enabled = false;
int g_jitter_intensity = 2;
bool g_sound_enabled = false;
int g_max_cps_limit = 20; // 0 = Unlimited
int g_current_theme = 0; // 0: Dark, 1: Light, 2: Matrix

// --- CPS Test Variables ---
int g_cps_clicks = 0;
double g_cps_result = 0.0;
DWORD g_cps_start_time = 0;
bool g_cps_active = false;

// --- State Variables ---
std::map<int, std::vector<KeyAction>> g_complex_mappings;
std::map<int, bool> g_physical_key_status;
std::map<int, float> g_key_states;
std::map<int, ClickState> g_rapid_states;

int g_last_vk_code = 0;
std::string g_last_key_name = "None";
AppState g_app_state = AppState::Dashboard;

// Wizard Variables
int g_wiz_source_key = -1;
int g_wiz_target_key = -1;
int g_wiz_delay = 50;
bool g_wiz_is_rapid = false;
bool g_wiz_swap_keys = false;

HHOOK g_hKeyboardHook = NULL;
HHOOK g_hMouseHook = NULL;

// --- Helper Functions ---

int GetRandomInt(int min, int max) {
    static std::mt19937 rng(std::random_device{}());
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

// --- Anti-Freeze Sender ---
void ExecuteSendInput(int vkCode, bool isUp) {
    INPUT input = {};
    if (vkCode >= VK_LBUTTON && vkCode <= VK_XBUTTON2) {
        input.type = INPUT_MOUSE;
        if (vkCode == VK_LBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
        else if (vkCode == VK_RBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
        else if (vkCode == VK_MBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN;
        else if (vkCode == VK_XBUTTON1) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON1; }
        else if (vkCode == VK_XBUTTON2) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON2; }
        
        // Jitter Feature
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
    
    // Sound Feature
    if (!isUp && g_sound_enabled) {
        // FIX: Use PlaySoundA for explicit char string support
        PlaySoundA("SystemDefault", NULL, SND_ASYNC | SND_NODEFAULT);
    }
}

// --- Logic ---
void ProcessInputLogic() {
    DWORD currentTime = GetTickCount();

    // 1. Rapid Fire Logic
    for (auto& [sourceKey, actions] : g_complex_mappings) {
        if (g_physical_key_status[sourceKey]) {
            for (auto& action : actions) {
                if (action.type == ActionType::RapidFire) {
                    
                    if (g_rapid_states.find(action.targetVk) == g_rapid_states.end()) {
                        g_rapid_states[action.targetVk] = { false, 0 };
                    }

                    ClickState& state = g_rapid_states[action.targetVk];

                    if (currentTime >= state.nextActionTime) {
                        if (!state.isDown) {
                            ExecuteSendInput(action.targetVk, false);
                            state.isDown = true;
                            state.nextActionTime = currentTime + GetRandomInt(20, 40); 
                            if (g_key_states.count(action.targetVk)) g_key_states[action.targetVk] = 1.0f;
                        } 
                        else {
                            ExecuteSendInput(action.targetVk, true);
                            state.isDown = false;
                            
                            int delay = action.delayMs;
                            if (g_max_cps_limit > 0) {
                                int minDelay = 1000 / g_max_cps_limit;
                                if (delay < minDelay) delay = minDelay;
                            }
                            state.nextActionTime = currentTime + delay + GetRandomInt(0, 10);
                        }
                    }
                }
            }
        } else {
            // Reset states
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

    // 2. Animation Decay
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

    if (g_complex_mappings.count(vkCode)) {
        g_physical_key_status[vkCode] = !isUp;
        auto& actions = g_complex_mappings[vkCode];
        for (auto& action : actions) {
            if (action.type == ActionType::SinglePress) {
                ExecuteSendInput(action.targetVk, isUp);
                if (!isUp) g_key_states[action.targetVk] = 1.0f;
            }
        }
        return true; 
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
}

void UninstallHooks() {
    if (g_hKeyboardHook) { UnhookWindowsHookEx(g_hKeyboardHook); g_hKeyboardHook = NULL; }
    if (g_hMouseHook) { UnhookWindowsHookEx(g_hMouseHook); g_hMouseHook = NULL; }
}

void ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    if (g_current_theme == 0) { // Dark
        ImGui::StyleColorsDark();
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
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
    ImGui::Begin("Pro Input Remapper (Fixed)", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::Columns(2, "MainLayout", true);
    ImGui::SetColumnWidth(0, 200);

    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.5f, 1.0f), " ULTIMATE BUILD");
    ImGui::Separator();

    static int tab = 0;
    ImGui::Dummy(ImVec2(0,10));
    if (ImGui::Button("Dashboard", ImVec2(180, 40))) tab = 0;
    if (ImGui::Button("Mappings", ImVec2(180, 40))) tab = 1;
    if (ImGui::Button("Features", ImVec2(180, 40))) tab = 2;
    if (ImGui::Button("CPS Test", ImVec2(180, 40))) tab = 3;
    if (ImGui::Button("Settings", ImVec2(180, 40))) tab = 4;

    ImGui::NextColumn();

    // --- Dashboard ---
    if (tab == 0) {
        ImGui::Text("System Status:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0,1,0,1), "Active");
        ImGui::Spacing();
        ImGui::Text("Last Key: %s", g_last_key_name.c_str());
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Active Features:");
        if (g_jitter_enabled) ImGui::BulletText("Jitter Enabled (Intensity: %d)", g_jitter_intensity);
        if (g_sound_enabled) ImGui::BulletText("Click Sound ON");
        if (g_max_cps_limit > 0) ImGui::BulletText("CPS Limit: %d", g_max_cps_limit);
        else ImGui::BulletText("CPS Limit: Unlimited");
    }
    // --- Mappings ---
    else if (tab == 1) {
        if (g_app_state == AppState::Dashboard) {
            if (ImGui::Button("+ Create Map", ImVec2(150, 30))) {
                g_app_state = AppState::Wizard_WaitForOriginal;
                g_wiz_swap_keys = false;
            }
        }
        else if (g_app_state == AppState::Wizard_WaitForOriginal) ImGui::Button("PRESS SOURCE KEY...", ImVec2(300, 50));
        else if (g_app_state == AppState::Wizard_WaitForTarget) ImGui::Button("PRESS TARGET KEY...", ImVec2(300, 50));
        else if (g_app_state == AppState::Wizard_Configure) {
            ImGui::Text("%s -> %s", GetKeyNameSmart(g_wiz_source_key).c_str(), GetKeyNameSmart(g_wiz_target_key).c_str());
            ImGui::Checkbox("Swap Keys?", &g_wiz_swap_keys);
            if (!g_wiz_swap_keys) {
                ImGui::RadioButton("Single", (int*)&g_wiz_is_rapid, 0); ImGui::SameLine();
                ImGui::RadioButton("Rapid", (int*)&g_wiz_is_rapid, 1);
                if (g_wiz_is_rapid) ImGui::SliderInt("Speed (ms)", &g_wiz_delay, 1, 1000);
            }
            if (ImGui::Button("Save Map")) {
                KeyAction act1; act1.targetVk = g_wiz_target_key;
                act1.type = g_wiz_is_rapid ? ActionType::RapidFire : ActionType::SinglePress;
                act1.delayMs = g_wiz_delay;
                g_complex_mappings[g_wiz_source_key].push_back(act1);
                if (g_wiz_swap_keys) {
                    KeyAction act2; act2.targetVk = g_wiz_source_key;
                    act2.type = ActionType::SinglePress; act2.delayMs = 0;
                    g_complex_mappings[g_wiz_target_key].push_back(act2);
                }
                g_app_state = AppState::Dashboard;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) g_app_state = AppState::Dashboard;
        }
        
        ImGui::Separator();
        ImGui::BeginChild("List", ImVec2(0, 300), true);
        for (auto it = g_complex_mappings.begin(); it != g_complex_mappings.end(); ) {
            if (it->second.empty()) { it = g_complex_mappings.erase(it); continue; }
            if (ImGui::TreeNode(GetKeyNameSmart(it->first).c_str())) {
                ImGui::SameLine(200);
                if (ImGui::Button(("Del Key##" + std::to_string(it->first)).c_str())) { it = g_complex_mappings.erase(it); ImGui::TreePop(); continue; }
                int idx=0;
                for (auto aIt = it->second.begin(); aIt != it->second.end(); ) {
                    ImGui::Text("-> %s (%s)", GetKeyNameSmart(aIt->targetVk).c_str(), aIt->type == ActionType::RapidFire ? "RAPID" : "ONE");
                    ImGui::SameLine(300);
                    if (ImGui::Button(("X##" + std::to_string(it->first) + std::to_string(idx++)).c_str())) aIt = it->second.erase(aIt); else ++aIt;
                }
                ImGui::TreePop();
            }
            ++it;
        }
        ImGui::EndChild();
    }
    // --- Features ---
    else if (tab == 2) {
        ImGui::Text("Advanced Features");
        ImGui::Separator();
        ImGui::Checkbox("Enable Jitter", &g_jitter_enabled);
        if (g_jitter_enabled) ImGui::SliderInt("Intensity", &g_jitter_intensity, 1, 10);
        ImGui::Dummy(ImVec2(0,10));
        ImGui::Checkbox("Enable Click Sound", &g_sound_enabled);
        ImGui::Dummy(ImVec2(0,10));
        ImGui::SliderInt("Max CPS", &g_max_cps_limit, 0, 50);
    }
    // --- CPS Test ---
    else if (tab == 3) {
        ImGui::Text("CPS Tester");
        ImGui::Separator();
        if (ImGui::Button("CLICK HERE\n(Test Area)", ImVec2(400, 200))) {}
        if (ImGui::IsItemHovered()) g_cps_active = true; else g_cps_active = false;
        ImGui::Text("Current Speed: %.1f CPS", g_cps_result);
    }
    // --- Settings ---
    else if (tab == 4) {
        ImGui::Text("Application Settings");
        ImGui::Separator();
        const char* themes[] = { "Professional Dark", "Clean Light", "Hacker Matrix" };
        ImGui::Combo("Theme", &g_current_theme, themes, 3);
    }

    ImGui::End();
}
