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

// Wizard Variables
int g_wiz_source_key = -1;
int g_wiz_target_key = -1;
int g_wiz_delay = 50; 
bool g_wiz_is_rapid = false;
bool g_wiz_swap_keys = false;

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

// --- Input Injection (Safe Mode) ---
void SendInputEvent(int vkCode, bool isUp) {
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
    // إرسال الإدخال
    SendInput(1, &input, sizeof(INPUT));
}

// --- Rapid Fire Processor (Optimized) ---
void ProcessRapidFire() {
    // نستخدم static لتقليل الحمل على الذاكرة
    static DWORD lastGlobalCheck = 0;
    DWORD currentTime = GetTickCount();

    // حماية النظام: لا تقم بالفحص أكثر من مرة كل 1 مللي ثانية
    if (currentTime == lastGlobalCheck) return; 
    lastGlobalCheck = currentTime;

    for (auto& [sourceKey, actions] : g_complex_mappings) {
        // فقط إذا كان الزر مضغوطاً فعلياً
        if (g_physical_key_status[sourceKey]) {
            for (auto& action : actions) {
                if (action.type == ActionType::RapidFire) {
                    // حماية: تأكد أن التأخير لا يقل عن 10ms في التنفيذ الفعلي لمنع تجميد النظام
                    // حتى لو اختار المستخدم 1ms، النظام لا يستطيع التعامل مع ذلك بدقة وسيعلق.
                    int safeDelay = (action.delayMs < 5) ? 5 : action.delayMs;

                    if (currentTime - action.lastExecutionTime >= (DWORD)safeDelay) {
                        SendInputEvent(action.targetVk, false); // Down
                        SendInputEvent(action.targetVk, true);  // Up
                        
                        action.lastExecutionTime = currentTime;
                        // تحديث حالة العرض (بدون إفراط)
                        if (g_key_states.find(action.targetVk) != g_key_states.end()) {
                             g_key_states[action.targetVk] = 1.0f; 
                        }
                    }
                }
            }
        }
    }
}

void UpdateAnimationState() {
    for (auto& pair : g_key_states) {
        if (pair.second > 0.0f) {
            pair.second -= 0.05f;
            if (pair.second < 0.0f) pair.second = 0.0f;
        }
    }
}

// --- Hook Logic ---
bool HandleHook(int vkCode, bool isUp, bool isInjected) {
    // 1. أهم خطوة لمنع التعليق: تجاهل ما أرسله البرنامج فوراً
    if (isInjected) return false;

    // 2. منطق المعالج (Wizard) - لا يسبب تعليق عادة
    if (!isUp) {
        if (g_app_state == AppState::Wizard_WaitForOriginal) {
            g_wiz_source_key = vkCode;
            g_app_state = AppState::Wizard_WaitForTarget;
            return true; 
        }
        else if (g_app_state == AppState::Wizard_WaitForTarget) {
            g_wiz_target_key = vkCode;
            g_app_state = AppState::Wizard_AskSwap; 
            return true; 
        }
    }

    // 3. المنطق الرئيسي (Execution)
    // نستخدم count للفحص السريع بدلاً من find
    if (g_complex_mappings.count(vkCode)) {
        // تحديث الحالة الفيزيائية للزر (مهم للـ Rapid Fire)
        g_physical_key_status[vkCode] = !isUp;

        auto& actions = g_complex_mappings[vkCode];
        bool hasBlocked = false;
        
        // تنفيذ الإجراءات
        if (!isUp) { // Press Down
            for (auto& action : actions) {
                if (action.type == ActionType::SinglePress) {
                    SendInputEvent(action.targetVk, false); 
                    g_key_states[action.targetVk] = 1.0f;
                    hasBlocked = true;
                } else {
                    // Rapid Fire يتم التعامل معه في ProcessRapidFire
                    // لكن يجب أن نحجب الزر الأصلي هنا
                    hasBlocked = true; 
                }
            }
        } 
        else { // Release Up
            for (auto& action : actions) {
                if (action.type == ActionType::SinglePress) {
                    SendInputEvent(action.targetVk, true); 
                    hasBlocked = true;
                } else {
                    hasBlocked = true; 
                }
            }
        }
        
        // إذا كان هناك أي إجراء مرتبط، قم بمنع الزر الأصلي
        if (hasBlocked) return true;
    }

    // Visualization (للعرض فقط)
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
        // التحقق الصارم من الـ Injected Flag
        if (HandleHook(p->vkCode, isUp, (p->flags & LLKHF_INJECTED) != 0)) return 1;
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

        if (vk != 0 && HandleHook(vk, isUp, (p->flags & LLMHF_INJECTED) != 0)) return 1;
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

// --- Render UI ---
void RenderUI() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
    
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

    if (tab == 0) {
        ImGui::Text("Active Inputs");
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1,1,0,1), "Last Key: %s", g_last_key_name.c_str());
        ImGui::Dummy(ImVec2(0,20));
        ImGui::Text("Status: Running smoothly.");
    }
    else if (tab == 1) {
        ImGui::TextColored(ImVec4(0, 0.8f, 1, 1), "Mapping Configuration");
        ImGui::Separator();
        
        // --- Wizard Section ---
        if (g_app_state == AppState::Dashboard) {
            if (ImGui::Button("+ Create New Map", ImVec2(200, 40))) {
                g_app_state = AppState::Wizard_WaitForOriginal;
                g_wiz_swap_keys = false; 
            }
        }
        else if (g_app_state == AppState::Wizard_WaitForOriginal) {
            ImGui::Button("STEP 1: PRESS SOURCE KEY", ImVec2(400, 60));
            ImGui::Text("Click the button to modify.");
        }
        else if (g_app_state == AppState::Wizard_WaitForTarget) {
            ImGui::Button("STEP 2: PRESS TARGET ACTION", ImVec2(400, 60));
        }
        else if (g_app_state == AppState::Wizard_AskSwap) {
            ImGui::BeginGroup();
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "STEP 3: MAPPING TYPE");
            ImGui::Text("Source: %s", GetKeyNameSmart(g_wiz_source_key).c_str());
            ImGui::Text("Target: %s", GetKeyNameSmart(g_wiz_target_key).c_str());
            ImGui::Dummy(ImVec2(0, 10));
            ImGui::Text("Do you want to SWAP them?");
            
            if (ImGui::Button("NO (Remap Only)", ImVec2(180, 40))) {
                g_wiz_swap_keys = false;
                g_app_state = AppState::Wizard_Configure;
            }
            ImGui::SameLine();
            if (ImGui::Button("YES (Swap Keys)", ImVec2(180, 40))) {
                g_wiz_swap_keys = true;
                g_app_state = AppState::Wizard_Configure;
            }
            ImGui::EndGroup();
        }
        else if (g_app_state == AppState::Wizard_Configure) {
            ImGui::Text("Setup: %s -> %s", GetKeyNameSmart(g_wiz_source_key).c_str(), GetKeyNameSmart(g_wiz_target_key).c_str());
            if (g_wiz_swap_keys) ImGui::TextColored(ImVec4(0,1,0,1), "[SWAP MODE]");
            
            ImGui::Separator();
            
            if (!g_wiz_swap_keys) {
                ImGui::Text("Mode:");
                if (ImGui::RadioButton("Single Press", !g_wiz_is_rapid)) g_wiz_is_rapid = false;
                ImGui::SameLine();
                if (ImGui::RadioButton("Rapid Fire", g_wiz_is_rapid)) g_wiz_is_rapid = true;
            } else {
                g_wiz_is_rapid = false; // Force single for swap
            }

            if (g_wiz_is_rapid) {
                ImGui::SliderInt("Speed (ms)", &g_wiz_delay, 1, 100); 
                if (g_wiz_delay < 10) ImGui::TextColored(ImVec4(1,0,0,1), "Caution: Very Fast!");
            }

            ImGui::Dummy(ImVec2(0, 20));
            if (ImGui::Button("SAVE", ImVec2(150, 40))) {
                KeyAction act1;
                act1.targetVk = g_wiz_target_key;
                act1.type = g_wiz_is_rapid ? ActionType::RapidFire : ActionType::SinglePress;
                act1.delayMs = g_wiz_delay;
                g_complex_mappings[g_wiz_source_key].push_back(act1);

                if (g_wiz_swap_keys) {
                    KeyAction act2;
                    act2.targetVk = g_wiz_source_key;
                    act2.type = ActionType::SinglePress;
                    act2.delayMs = 0;
                    g_complex_mappings[g_wiz_target_key].push_back(act2);
                }
                g_app_state = AppState::Dashboard;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 40))) g_app_state = AppState::Dashboard;
        }

        // --- Active Mappings List with DELETE ---
        ImGui::Separator();
        ImGui::Text("Active Mappings (Click [X] to delete):");
        
        ImGui::BeginChild("MappingsList", ImVec2(0, 300), true);
        
        // نستخدم Iterator للمرور والحذف بأمان
        for (auto it = g_complex_mappings.begin(); it != g_complex_mappings.end(); ) {
            int srcKey = it->first;
            auto& actions = it->second;

            if (actions.empty()) {
                // تنظيف القوائم الفارغة
                it = g_complex_mappings.erase(it);
                continue;
            }

            bool nodeOpen = ImGui::TreeNode(GetKeyNameSmart(srcKey).c_str());
            ImGui::SameLine(300);
            
            // زر لحذف الزر بالكامل (كل الوظائف المرتبطة به)
            ImGui::PushID(srcKey * 999);
            if (ImGui::Button("Delete Key")) {
                it = g_complex_mappings.erase(it);
                if (nodeOpen) ImGui::TreePop(); // إغلاق العقدة إذا كانت مفتوحة
                ImGui::PopID();
                continue; // الانتقال للعنصر التالي بعد الحذف
            }
            ImGui::PopID();

            if (nodeOpen) {
                int actionIdx = 0;
                for (auto actIt = actions.begin(); actIt != actions.end(); ) {
                    ImGui::PushID(srcKey + actionIdx * 1000); // ID فريد لكل زر
                    
                    std::string label = "-> " + GetKeyNameSmart(actIt->targetVk);
                    if (actIt->type == ActionType::RapidFire) 
                        label += " [RAPID " + std::to_string(actIt->delayMs) + "ms]";
                    else 
                        label += " [SINGLE]";

                    ImGui::Text(label.c_str());
                    ImGui::SameLine(250);
                    
                    // زر حذف لوظيفة محددة داخل الزر
                    if (ImGui::Button("[X]")) {
                        actIt = actions.erase(actIt);
                    } else {
                        ++actIt;
                    }
                    
                    ImGui::PopID();
                    actionIdx++;
                }
                ImGui::TreePop();
            }
            
            ++it;
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
