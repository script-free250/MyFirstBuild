#include "ui.h"
#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <map>

// --- Global Variables ---
std::map<int, std::vector<KeyAction>> g_complex_mappings;
std::map<int, bool> g_physical_key_status;
std::map<int, float> g_key_states;

int g_last_vk_code = 0;
std::string g_last_key_name = "None";
AppState g_app_state = AppState::Dashboard;

// متغيرات مؤقتة لعملية الإعداد (Wizard Variables)
int g_wiz_source_key = -1;
int g_wiz_target_key = -1;
int g_wiz_delay = 100; // Default 100ms
bool g_wiz_is_rapid = false;

// Hooks
HHOOK g_hKeyboardHook = NULL;
HHOOK g_hMouseHook = NULL;

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

// --- Input Injection Helper ---
void SendInputEvent(int vkCode, bool isUp) {
    INPUT input = {};
    if (vkCode >= VK_LBUTTON && vkCode <= VK_XBUTTON2) { // Mouse
        input.type = INPUT_MOUSE;
        if (vkCode == VK_LBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
        else if (vkCode == VK_RBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
        else if (vkCode == VK_MBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN;
        else if (vkCode == VK_XBUTTON1) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON1; }
        else if (vkCode == VK_XBUTTON2) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON2; }
    } else { // Keyboard
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = isUp ? KEYEVENTF_KEYUP : 0;
    }
    SendInput(1, &input, sizeof(INPUT));
}

// --- Rapid Fire Logic Processor ---
// هذه الدالة يجب استدعاؤها في الـ Loop الرئيسي في main.cpp
void ProcessRapidFire() {
    DWORD currentTime = GetTickCount();

    // نمر على كل الأزرار المخصصة
    for (auto& [sourceKey, actions] : g_complex_mappings) {
        // هل المستخدم ضاغط على الزر الأصلي حالياً؟
        if (g_physical_key_status[sourceKey]) {
            for (auto& action : actions) {
                if (action.type == ActionType::RapidFire) {
                    // فحص الوقت (هل حان وقت الضغطة التالية؟)
                    if (currentTime - action.lastExecutionTime >= action.delayMs) {
                        // تنفيذ الضغطة (Click Down -> Click Up fast)
                        SendInputEvent(action.targetVk, false); // Down
                        SendInputEvent(action.targetVk, true);  // Up
                        
                        // تحديث الوقت والأنيميشن
                        action.lastExecutionTime = currentTime;
                        g_key_states[action.targetVk] = 1.0f; 
                    }
                }
            }
        }
    }
}

// --- Unified Hook Handler ---
bool HandleHook(int vkCode, bool isUp, bool isInjected) {
    if (isInjected) return false;

    // Wizard Logic (Capture Keys for Setup)
    if (!isUp) {
        if (g_app_state == AppState::Wizard_WaitForOriginal) {
            g_wiz_source_key = vkCode;
            g_app_state = AppState::Wizard_WaitForTarget;
            return true; // Block
        }
        else if (g_app_state == AppState::Wizard_WaitForTarget) {
            g_wiz_target_key = vkCode;
            g_app_state = AppState::Wizard_Configure; // Go to Config Screen
            return true; // Block
        }
    }

    // Main Functionality Logic
    if (g_complex_mappings.count(vkCode)) {
        // تحديث حالة الزر الفيزيائي (مهم جداً للـ Rapid Fire)
        g_physical_key_status[vkCode] = !isUp;

        auto& actions = g_complex_mappings[vkCode];
        
        if (!isUp) { // عند الضغط (Down)
            bool shouldBlock = false;
            for (auto& action : actions) {
                if (action.type == ActionType::SinglePress) {
                    SendInputEvent(action.targetVk, false); // Send Down
                    g_key_states[action.targetVk] = 1.0f;
                    shouldBlock = true;
                } else if (action.type == ActionType::RapidFire) {
                    // الـ Rapid Fire يتم معالجته في ProcessRapidFire
                    // لكننا نحتاج لمنع الزر الأصلي
                    shouldBlock = true;
                }
            }
            return shouldBlock;
        } 
        else { // عند الرفع (Up)
            bool shouldBlock = false;
            for (auto& action : actions) {
                if (action.type == ActionType::SinglePress) {
                    SendInputEvent(action.targetVk, true); // Send Up
                    shouldBlock = true;
                } else {
                    shouldBlock = true; // Stop Rapid Fire logic handled by status map
                }
            }
            return shouldBlock;
        }
    }

    // Visualizer only
    if (!isUp) {
        g_key_states[vkCode] = 1.0f;
        g_last_vk_code = vkCode;
        g_last_key_name = GetKeyNameSmart(vkCode);
    }
    
    return false;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        if (HandleHook(p->vkCode, isUp, p->flags & LLKHF_INJECTED)) return 1;
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* p = (MSLLHOOKSTRUCT*)lParam;
        int vk = 0; bool isUp = false;
        if (wParam == WM_LBUTTONDOWN) vk = VK_LBUTTON;
        else if (wParam == WM_LBUTTONUP) { vk = VK_LBUTTON; isUp = true; }
        else if (wParam == WM_RBUTTONDOWN) vk = VK_RBUTTON;
        else if (wParam == WM_RBUTTONUP) { vk = VK_RBUTTON; isUp = true; }
        else if (wParam == WM_MBUTTONDOWN) vk = VK_MBUTTON;
        else if (wParam == WM_MBUTTONUP) { vk = VK_MBUTTON; isUp = true; }
        else if (wParam == WM_XBUTTONDOWN) vk = (HIWORD(p->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        else if (wParam == WM_XBUTTONUP) { vk = (HIWORD(p->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2; isUp = true; }

        if (vk != 0 && HandleHook(vk, isUp, p->flags & LLMHF_INJECTED)) return 1;
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void InstallHooks() {
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
}
void UninstallHooks() {
    if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
    if (g_hMouseHook) UnhookWindowsHookEx(g_hMouseHook);
}

// --- Render UI ---
void RenderUI() {
    // Style
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
    ImGui::Begin("Pro Input Remapper", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::Columns(2, "MainLayout", true);
    ImGui::SetColumnWidth(0, 300);

    // Sidebar
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.5f, 1.0f), "  ULTIMATE REMAPPER");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 20));

    static int tab = 0;
    if (ImGui::Button("  Dashboard  ", ImVec2(280, 50))) tab = 0;
    ImGui::Dummy(ImVec2(0, 10));
    if (ImGui::Button("  Configure Maps  ", ImVec2(280, 50))) tab = 1;

    ImGui::NextColumn();

    // Content
    if (tab == 0) {
        ImGui::Text("Active Inputs");
        ImGui::Separator();
        // Visualizer Code (Same as before, simplified for brevity)
        ImGui::TextColored(ImVec4(1,1,0,1), "Last Key: %s", g_last_key_name.c_str());
        
        ImGui::Dummy(ImVec2(0, 50));
        ImGui::Text("Instructions:");
        ImGui::BulletText("Go to 'Configure Maps' to create new bindings.");
        ImGui::BulletText("You can assign MULTIPLE actions to a single key.");
        ImGui::BulletText("You can choose 'Rapid Fire' mode for auto-clicking.");
    }
    else if (tab == 1) {
        ImGui::TextColored(ImVec4(0, 0.8f, 1, 1), "Mapping Configuration Wizard");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 10));

        // --- Wizard UI ---
        if (g_app_state == AppState::Dashboard) {
            if (ImGui::Button("+ Create New Map", ImVec2(200, 40))) {
                g_app_state = AppState::Wizard_WaitForOriginal;
            }
        }
        else if (g_app_state == AppState::Wizard_WaitForOriginal) {
            ImGui::Button("STEP 1: PRESS SOURCE KEY", ImVec2(400, 60));
            ImGui::Text("Press the Mouse Button or Key you want to modify.");
        }
        else if (g_app_state == AppState::Wizard_WaitForTarget) {
            ImGui::Button("STEP 2: PRESS TARGET ACTION", ImVec2(400, 60));
            ImGui::Text("Press the Key/Button you want executed.");
        }
        else if (g_app_state == AppState::Wizard_Configure) {
            // Configuration Step
            ImGui::BeginGroup();
            ImGui::TextColored(ImVec4(1,1,0,1), "STEP 3: ACTION DETAILS");
            ImGui::Text("Source: %s", GetKeyNameSmart(g_wiz_source_key).c_str());
            ImGui::Text("Target: %s", GetKeyNameSmart(g_wiz_target_key).c_str());
            
            ImGui::Separator();
            
            // Mode Selection
            ImGui::Text("Trigger Mode:");
            if (ImGui::RadioButton("Single Press (Normal)", !g_wiz_is_rapid)) g_wiz_is_rapid = false;
            if (ImGui::RadioButton("Rapid Fire (Spam/Hold)", g_wiz_is_rapid)) g_wiz_is_rapid = true;

            if (g_wiz_is_rapid) {
                ImGui::Indent();
                ImGui::Text("Speed (Milliseconds):");
                ImGui::SliderInt("##Delay", &g_wiz_delay, 1, 1000, "%d ms");
                ImGui::TextDisabled("(Lower = Faster. 10ms = Very Fast)");
                ImGui::Unindent();
            }

            ImGui::Dummy(ImVec2(0, 20));
            if (ImGui::Button("CONFIRM & ADD", ImVec2(150, 40))) {
                // Add to Global Map
                KeyAction newAction;
                newAction.targetVk = g_wiz_target_key;
                newAction.type = g_wiz_is_rapid ? ActionType::RapidFire : ActionType::SinglePress;
                newAction.delayMs = g_wiz_delay;
                
                g_complex_mappings[g_wiz_source_key].push_back(newAction);
                
                g_app_state = AppState::Dashboard; // Reset
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 40))) {
                g_app_state = AppState::Dashboard;
            }
            ImGui::EndGroup();
        }

        ImGui::Dummy(ImVec2(0, 30));
        ImGui::Separator();
        ImGui::Text("Active Mappings List:");
        
        ImGui::BeginChild("Mappings", ImVec2(0, 300), true);
        std::vector<int> emptyKeys;
        
        for (auto& [src, actions] : g_complex_mappings) {
            if (actions.empty()) { emptyKeys.push_back(src); continue; }

            if (ImGui::TreeNode(GetKeyNameSmart(src).c_str())) {
                int actionIndex = 0;
                std::vector<int> actionsToRemove;

                for (auto& action : actions) {
                    ImGui::PushID(actionIndex);
                    std::string desc = " -> Trigger: " + GetKeyNameSmart(action.targetVk);
                    if (action.type == ActionType::RapidFire) desc += " [RAPID: " + std::to_string(action.delayMs) + "ms]";
                    else desc += " [SINGLE]";
                    
                    ImGui::Text(desc.c_str());
                    ImGui::SameLine(400);
                    if (ImGui::Button("Delete")) actionsToRemove.push_back(actionIndex);
                    
                    ImGui::PopID();
                    actionIndex++;
                }

                // Remove deleted actions
                for (int i = actionsToRemove.size() - 1; i >= 0; i--) {
                    actions.erase(actions.begin() + actionsToRemove[i]);
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
