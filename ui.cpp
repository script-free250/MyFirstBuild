#include "ui.h"
#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// --- Global Variables ---
std::map<int, std::vector<KeyAction>> g_complex_mappings;
std::map<int, bool> g_physical_key_status;
std::map<int, float> g_key_states;
std::vector<std::pair<int, bool>> g_pending_inputs; // {vkCode, isUp}

int g_last_vk_code = 0;
std::string g_last_key_name = "None";
AppState g_app_state = AppState::Dashboard;
bool g_safety_mode_active = false;

// لمنع التكرار (Anti-Loop Protection)
std::set<int> g_ignore_next_press; 

// Wizard Variables
int g_wiz_source_key = -1;
int g_wiz_target_key = -1;
int g_wiz_delay = 50; 
bool g_wiz_is_rapid = false;
bool g_wiz_swap_keys = false;

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

// --- دالة الإرسال الفعلية (تستدعى خارج الهوك) ---
void ExecuteSendInput(int vkCode, bool isUp) {
    if (g_safety_mode_active) return;

    // تسجيل أن هذا الزر نحن من ضغطه (لتجاهله في الهوك)
    if (!isUp) g_ignore_next_press.insert(vkCode);

    INPUT input = {};
    if (vkCode >= VK_LBUTTON && vkCode <= VK_XBUTTON2) { 
        input.type = INPUT_MOUSE;
        if (vkCode == VK_LBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
        else if (vkCode == VK_RBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
        else if (vkCode == VK_MBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN;
        else if (vkCode == VK_XBUTTON1) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON1; }
        else if (vkCode == VK_XBUTTON2) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON2; }
    } else { 
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = isUp ? KEYEVENTF_KEYUP : 0;
    }
    
    SendInput(1, &input, sizeof(INPUT));
}

// --- المعالج الرئيسي (يعمل في Main Loop) ---
void ProcessInputLogic() {
    if (g_safety_mode_active) return;

    DWORD currentTime = GetTickCount();

    // 1. معالجة Single Press المؤجلة (لضمان سرعة الاستجابة)
    // (يتم التعامل معها مباشرة في الهوك للسرعة القصوى، ولكن هنا ننظف الحالات)
    
    // 2. معالجة Rapid Fire
    for (auto& [sourceKey, actions] : g_complex_mappings) {
        if (g_physical_key_status[sourceKey]) {
            for (auto& action : actions) {
                if (action.type == ActionType::RapidFire) {
                    // هنا نسمح بـ 1ms لأننا خارج الهوك ولن نعلق النظام
                    if (currentTime - action.lastExecutionTime >= (DWORD)action.delayMs) {
                        ExecuteSendInput(action.targetVk, false); // Down
                        ExecuteSendInput(action.targetVk, true);  // Up
                        action.lastExecutionTime = currentTime;
                        
                        // تحديث خفيف جداً للحالة
                        if (g_key_states.count(action.targetVk)) g_key_states[action.targetVk] = 1.0f;
                    }
                }
            }
        }
    }

    // 3. تحديث الأنيميشن
    for (auto& pair : g_key_states) {
        if (pair.second > 0.0f) pair.second -= 0.02f; // أبطأ قليلاً لتقليل الحمل
    }
}

// --- Hook Logic ---
// هذه الدالة يجب أن تكون سريعة جداً جداً
bool HandleHook(int vkCode, bool isUp, bool isInjected) {
    // 1. الحماية من الحلقات
    if (isInjected) return false;
    
    // حماية إضافية: هل هذا الزر نحن من أرسله ولم يحمل علامة Injected؟
    if (!isUp) {
        auto it = g_ignore_next_press.find(vkCode);
        if (it != g_ignore_next_press.end()) {
            g_ignore_next_press.erase(it); // وجدناه، تجاهله واحذفه من القائمة
            return false;
        }
    }

    if (g_safety_mode_active) return false;

    // 2. Wizard Setup
    if (!isUp && g_app_state != AppState::Dashboard) {
        if (g_app_state == AppState::Wizard_WaitForOriginal) {
            g_wiz_source_key = vkCode; g_app_state = AppState::Wizard_WaitForTarget; return true;
        } else if (g_app_state == AppState::Wizard_WaitForTarget) {
            g_wiz_target_key = vkCode; g_app_state = AppState::Wizard_AskSwap; return true;
        }
    }

    // 3. Execution (Remapping)
    if (g_complex_mappings.count(vkCode)) {
        g_physical_key_status[vkCode] = !isUp; // تحديث الحالة للـ Rapid Fire
        
        // بالنسبة للـ Single Press، يجب الإرسال فوراً لمنع التأخير
        // لكن نستخدم دالة الإرسال المحمية
        auto& actions = g_complex_mappings[vkCode];
        for (auto& action : actions) {
            if (action.type == ActionType::SinglePress) {
                ExecuteSendInput(action.targetVk, isUp); // Send Immediately
                if (!isUp) g_key_states[action.targetVk] = 1.0f;
            }
        }
        return true; // Block original key
    }

    // 4. Visualizer
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
        // التحقق المزدوج
        if (HandleHook(p->vkCode, isUp, (p->flags & LLKHF_INJECTED) != 0)) return 1;
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* p = (MSLLHOOKSTRUCT*)lParam;
        // التحقق المزدوج للماوس
        if (p->flags & LLMHF_INJECTED) return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);

        int vk = 0; bool isUp = false;
        if (wParam == WM_LBUTTONDOWN) vk = VK_LBUTTON;
        else if (wParam == WM_LBUTTONUP) { vk = VK_LBUTTON; isUp = true; }
        else if (wParam == WM_RBUTTONDOWN) vk = VK_RBUTTON;
        else if (wParam == WM_RBUTTONUP) { vk = VK_RBUTTON; isUp = true; }
        else if (wParam == WM_MBUTTONDOWN) vk = VK_MBUTTON;
        else if (wParam == WM_MBUTTONUP) { vk = VK_MBUTTON; isUp = true; }
        else if (wParam == WM_XBUTTONDOWN) vk = (HIWORD(p->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        else if (wParam == WM_XBUTTONUP) { vk = (HIWORD(p->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2; isUp = true; }

        if (vk != 0 && HandleHook(vk, isUp, false)) return 1;
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

void RenderUI() {
    // --- Optimized UI Rendering ---
    // إذا لم يكن هناك تركيز أو تصغير، لا نرسم (توفير CPU)
    // لكننا لا نملك وصولاً مباشراً لـ HWND هنا، سنعتمد على ImGui
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
    
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Pro Input Remapper", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::Columns(2, "MainLayout", true);
    ImGui::SetColumnWidth(0, 250);

    // Sidebar
    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.5f, 1.0f), "  ULTIMATE LITE");
    ImGui::Separator();
    
    if (g_safety_mode_active) {
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0, 0, 1));
        if (ImGui::Button("SAFETY ON (F10)", ImVec2(230, 50))) g_safety_mode_active = false;
        ImGui::PopStyleColor();
    } else {
         ImGui::Dummy(ImVec2(0, 10));
         if (ImGui::Button("PANIC STOP (F10)", ImVec2(230, 30))) g_safety_mode_active = true;
    }

    static int tab = 0;
    ImGui::Dummy(ImVec2(0,10));
    if (ImGui::Button("Dashboard", ImVec2(230, 40))) tab = 0;
    if (ImGui::Button("Mappings", ImVec2(230, 40))) tab = 1;

    ImGui::NextColumn();

    if (g_safety_mode_active) {
        ImGui::TextColored(ImVec4(1,0,0,1), "!!! SAFETY MODE ACTIVE !!!");
    }
    else if (tab == 0) {
        ImGui::Text("Last Key: %s", g_last_key_name.c_str());
        ImGui::TextDisabled("Program is running in background optimized mode.");
    }
    else if (tab == 1) {
        // ... (Wizard Code Same as Before - Simplified) ...
        if (g_app_state == AppState::Dashboard) {
            if (ImGui::Button("+ New Map", ImVec2(150, 30))) { g_app_state = AppState::Wizard_WaitForOriginal; g_wiz_swap_keys = false; }
        }
        else if (g_app_state == AppState::Wizard_WaitForOriginal) ImGui::Button("PRESS SOURCE KEY", ImVec2(300, 50));
        else if (g_app_state == AppState::Wizard_WaitForTarget) ImGui::Button("PRESS TARGET KEY", ImVec2(300, 50));
        else if (g_app_state == AppState::Wizard_AskSwap) {
             ImGui::Text("Swap?");
             if (ImGui::Button("No", ImVec2(100,30))) { g_wiz_swap_keys = false; g_app_state = AppState::Wizard_Configure; }
             ImGui::SameLine();
             if (ImGui::Button("Yes", ImVec2(100,30))) { g_wiz_swap_keys = true; g_app_state = AppState::Wizard_Configure; }
        }
        else if (g_app_state == AppState::Wizard_Configure) {
            ImGui::Text("%s -> %s", GetKeyNameSmart(g_wiz_source_key).c_str(), GetKeyNameSmart(g_wiz_target_key).c_str());
            if (!g_wiz_swap_keys) {
                ImGui::RadioButton("Single", (int*)&g_wiz_is_rapid, 0); ImGui::SameLine();
                ImGui::RadioButton("Rapid", (int*)&g_wiz_is_rapid, 1);
                if (g_wiz_is_rapid) ImGui::SliderInt("ms", &g_wiz_delay, 1, 500);
            }
            if (ImGui::Button("Save")) {
                KeyAction act1 = { g_wiz_target_key, g_wiz_is_rapid ? ActionType::RapidFire : ActionType::SinglePress, g_wiz_delay };
                g_complex_mappings[g_wiz_source_key].push_back(act1);
                if (g_wiz_swap_keys) { KeyAction act2 = { g_wiz_source_key, ActionType::SinglePress, 0 }; g_complex_mappings[g_wiz_target_key].push_back(act2); }
                g_app_state = AppState::Dashboard;
            }
            ImGui::SameLine(); if (ImGui::Button("Cancel")) g_app_state = AppState::Dashboard;
        }

        ImGui::Separator();
        ImGui::BeginChild("List", ImVec2(0, 300), true);
        for (auto it = g_complex_mappings.begin(); it != g_complex_mappings.end(); ) {
            if (it->second.empty()) { it = g_complex_mappings.erase(it); continue; }
            if (ImGui::TreeNode(GetKeyNameSmart(it->first).c_str())) {
                ImGui::SameLine(200);
                if (ImGui::Button(("Del##" + std::to_string(it->first)).c_str())) { it = g_complex_mappings.erase(it); ImGui::TreePop(); continue; }
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
    ImGui::End();
}
