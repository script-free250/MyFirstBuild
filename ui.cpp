#include "ui.h"
#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <map>

// --- Global Variables ---
std::map<int, int> g_key_mappings;
std::map<int, float> g_key_states;
int g_key_press_count = 0;
int g_last_vk_code = 0;
std::string g_last_key_name = "None";
AppState g_app_state = AppState::Dashboard;

// متغيرات التغيير المؤقتة
int g_remap_source_key = -1;

// Hooks Handles
HHOOK g_hKeyboardHook = NULL;
HHOOK g_hMouseHook = NULL;

// --- Helper: Get Smart Name (Mouse & Keyboard) ---
std::string GetKeyNameSmart(int vkCode) {
    switch (vkCode) {
        case VK_LBUTTON: return "Mouse Left";
        case VK_RBUTTON: return "Mouse Right";
        case VK_MBUTTON: return "Mouse Middle";
        case VK_XBUTTON1: return "Mouse Side 1";
        case VK_XBUTTON2: return "Mouse Side 2";
        default: break;
    }
    
    char keyNameBuffer[128];
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    if (GetKeyNameTextA(scanCode << 16, keyNameBuffer, 128)) {
        return std::string(keyNameBuffer);
    }
    return "VK_" + std::to_string(vkCode);
}

// --- Smart Input Sender (يحاكي الضغط سواء ماوس أو كيبورد) ---
void SendSmartInput(int vkCode, bool keyUp) {
    INPUT input = {};
    
    // 1. فحص هل هو ماوس؟
    if (vkCode == VK_LBUTTON || vkCode == VK_RBUTTON || vkCode == VK_MBUTTON || vkCode == VK_XBUTTON1 || vkCode == VK_XBUTTON2) {
        input.type = INPUT_MOUSE;
        if (vkCode == VK_LBUTTON) input.mi.dwFlags = keyUp ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
        else if (vkCode == VK_RBUTTON) input.mi.dwFlags = keyUp ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
        else if (vkCode == VK_MBUTTON) input.mi.dwFlags = keyUp ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN;
        else if (vkCode == VK_XBUTTON1) {
            input.mi.dwFlags = keyUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN;
            input.mi.mouseData = XBUTTON1;
        }
        else if (vkCode == VK_XBUTTON2) {
            input.mi.dwFlags = keyUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN;
            input.mi.mouseData = XBUTTON2;
        }
    }
    // 2. إذا كان كيبورد
    else {
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;
    }

    SendInput(1, &input, sizeof(INPUT));
}

// --- Unified Logic Handler ---
// هذه الدالة تعالج المنطق سواء جاء من الماوس أو الكيبورد
// return true means "BLOCK ORIGINAL INPUT"
bool HandleInputEvent(int vkCode, bool isUp, bool isInjected) {
    if (isInjected) return false; // لا تتدخل في ما أرسلناه نحن

    if (!isUp) { // عند الضغط فقط
        // 1. Remapping Setup Logic
        if (g_app_state == AppState::Remapping_WaitForOriginal) {
            g_remap_source_key = vkCode;
            g_app_state = AppState::Remapping_WaitForTarget;
            return true; // Block
        }
        else if (g_app_state == AppState::Remapping_WaitForTarget) {
            if (g_remap_source_key != -1) {
                g_key_mappings[g_remap_source_key] = vkCode;
            }
            g_app_state = AppState::Dashboard;
            return true; // Block
        }

        // 2. Active Remapping
        if (g_key_mappings.count(vkCode)) {
            int target = g_key_mappings[vkCode];
            SendSmartInput(target, false); // Press Down
            
            g_key_states[target] = 1.0f; // Animation for target
            return true; // Block original
        }

        // 3. Visualization
        g_key_states[vkCode] = 1.0f;
        g_last_vk_code = vkCode;
        g_key_press_count++;
        g_last_key_name = GetKeyNameSmart(vkCode);
    }
    else { // عند الرفع (Key Up)
        if (g_key_mappings.count(vkCode)) {
            int target = g_key_mappings[vkCode];
            SendSmartInput(target, true); // Release Up
            return true; // Block original
        }
    }
    return false;
}

// --- Keyboard Hook ---
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        bool isInjected = (pKey->flags & LLKHF_INJECTED);
        
        if (HandleInputEvent(pKey->vkCode, isUp, isInjected))
            return 1;
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// --- Mouse Hook ---
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
        bool isInjected = (pMouse->flags & LLMHF_INJECTED);
        
        int vkCode = 0;
        bool isUp = false;

        // Map Messages to VK
        if (wParam == WM_LBUTTONDOWN) vkCode = VK_LBUTTON;
        else if (wParam == WM_LBUTTONUP) { vkCode = VK_LBUTTON; isUp = true; }
        else if (wParam == WM_RBUTTONDOWN) vkCode = VK_RBUTTON;
        else if (wParam == WM_RBUTTONUP) { vkCode = VK_RBUTTON; isUp = true; }
        else if (wParam == WM_MBUTTONDOWN) vkCode = VK_MBUTTON;
        else if (wParam == WM_MBUTTONUP) { vkCode = VK_MBUTTON; isUp = true; }
        else if (wParam == WM_XBUTTONDOWN) {
            vkCode = (HIWORD(pMouse->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        }
        else if (wParam == WM_XBUTTONUP) {
            vkCode = (HIWORD(pMouse->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
            isUp = true;
        }

        if (vkCode != 0) {
            if (HandleInputEvent(vkCode, isUp, isInjected))
                return 1; 
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

void InstallHooks() {
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
}

void UninstallHooks() {
    if (g_hKeyboardHook) { UnhookWindowsHookEx(g_hKeyboardHook); g_hKeyboardHook = NULL; }
    if (g_hMouseHook) { UnhookWindowsHookEx(g_hMouseHook); g_hMouseHook = NULL; }
}

void UpdateAnimationState() {
    for (auto& pair : g_key_states) {
        if (pair.second > 0.0f) pair.second -= 0.05f;
    }
}

// --- UI Rendering ---
void RenderUI() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1000, 650), ImGuiCond_FirstUseEver);

    ImGui::Begin("Ultimate Input Manager", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::Columns(2, "MainLayout", true);
    ImGui::SetColumnWidth(0, 250);

    // Sidebar
    ImGui::Dummy(ImVec2(0, 20));
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "  INPUT MASTER");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 20));

    static int selectedTab = 0;
    if (ImGui::Button("  Dashboard  ", ImVec2(230, 45))) selectedTab = 0;
    ImGui::Dummy(ImVec2(0, 10));
    if (ImGui::Button("  Remapper  ", ImVec2(230, 45))) selectedTab = 1;
    
    ImGui::NextColumn();

    // Main Content
    ImGui::Dummy(ImVec2(0, 10));
    
    if (selectedTab == 0) {
        ImGui::Text("Device Visualizer");
        ImGui::Separator();
        
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        
        // --- Draw Keyboard Representation (Simplified) ---
        draw->AddRect(ImVec2(p.x + 20, p.y + 50), ImVec2(p.x + 320, p.y + 200), IM_COL32(255, 255, 255, 50), 5.0f);
        draw->AddText(ImVec2(p.x + 30, p.y + 30), IM_COL32(200, 200, 200, 255), "Keyboard Activity");
        
        // Show last key big
        if (g_last_vk_code >= 7 && g_last_vk_code < 255) { // Assuming keyboard range mostly
             draw->AddText(ImGui::GetFont(), 40.0f, ImVec2(p.x + 50, p.y + 100), IM_COL32(0, 255, 255, 255), g_last_key_name.c_str());
        }

        // --- Draw Mouse Representation ---
        ImVec2 mPos = ImVec2(p.x + 400, p.y + 50);
        // Body
        draw->AddRectFilled(mPos, ImVec2(mPos.x + 120, mPos.y + 180), IM_COL32(40, 40, 45, 255), 40.0f);
        draw->AddRect(mPos, ImVec2(mPos.x + 120, mPos.y + 180), IM_COL32(100, 100, 100, 255), 40.0f);
        
        // Left Button
        ImU32 colL = (g_key_states[VK_LBUTTON] > 0.1f) ? IM_COL32(0, 255, 0, 255) : IM_COL32(80, 80, 80, 255);
        draw->AddRectFilled(mPos, ImVec2(mPos.x + 59, mPos.y + 70), colL, 40.0f, ImDrawFlags_RoundCornersTopLeft);
        
        // Right Button
        ImU32 colR = (g_key_states[VK_RBUTTON] > 0.1f) ? IM_COL32(0, 255, 0, 255) : IM_COL32(80, 80, 80, 255);
        draw->AddRectFilled(ImVec2(mPos.x + 61, mPos.y), ImVec2(mPos.x + 120, mPos.y + 70), colR, 40.0f, ImDrawFlags_RoundCornersTopRight);

        // Side Buttons
        ImU32 colS1 = (g_key_states[VK_XBUTTON1] > 0.1f) ? IM_COL32(255, 165, 0, 255) : IM_COL32(60, 60, 60, 255);
        draw->AddRectFilled(ImVec2(mPos.x - 10, mPos.y + 80), ImVec2(mPos.x + 5, mPos.y + 110), colS1, 5.0f); // Side 1

        ImU32 colS2 = (g_key_states[VK_XBUTTON2] > 0.1f) ? IM_COL32(255, 165, 0, 255) : IM_COL32(60, 60, 60, 255);
        draw->AddRectFilled(ImVec2(mPos.x - 10, mPos.y + 50), ImVec2(mPos.x + 5, mPos.y + 75), colS2, 5.0f); // Side 2

        draw->AddText(ImVec2(mPos.x + 20, mPos.y + 200), IM_COL32(200, 200, 200, 255), "Mouse State");
    }
    else if (selectedTab == 1) {
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Universal Remapper (Mouse & Keyboard)");
        ImGui::Separator();
        
        // State Machine Messages
        if (g_app_state == AppState::Dashboard) {
            if (ImGui::Button(" + ADD NEW MAP ", ImVec2(200, 50))) {
                g_app_state = AppState::Remapping_WaitForOriginal;
            }
        }
        else if (g_app_state == AppState::Remapping_WaitForOriginal) {
            ImGui::Button(" WAITING INPUT... ", ImVec2(300, 50));
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Press ANY Key or Mouse Button to Change.");
        }
        else if (g_app_state == AppState::Remapping_WaitForTarget) {
            std::string label = " Remap [" + GetKeyNameSmart(g_remap_source_key) + "] To...";
            ImGui::Button(label.c_str(), ImVec2(400, 50));
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Press ANY Key or Mouse Button for the Result.");
        }

        // List
        ImGui::Dummy(ImVec2(0, 20));
        ImGui::Text("Active Configurations:");
        ImGui::BeginChild("List", ImVec2(0, 300), true);
        std::vector<int> toRemove;
        for (auto const& [src, dst] : g_key_mappings) {
            ImGui::PushID(src);
            std::string txt = GetKeyNameSmart(src) + "  -->  " + GetKeyNameSmart(dst);
            ImGui::Text(txt.c_str());
            ImGui::SameLine(350);
            if (ImGui::Button("Remove")) toRemove.push_back(src);
            ImGui::PopID();
        }
        for (int k : toRemove) g_key_mappings.erase(k);
        ImGui::EndChild();
    }

    ImGui::End();
}
