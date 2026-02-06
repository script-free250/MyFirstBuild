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

// --- دالة الإرسال الآمنة (Anti-Freeze Sender) ---
void ExecuteSendInput(int vkCode, bool isUp) {
    INPUT input = {};
    if (vkCode >= VK_LBUTTON && vkCode <= VK_XBUTTON2) { 
        input.type = INPUT_MOUSE;
        if (vkCode == VK_LBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
        else if (vkCode == VK_RBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
        else if (vkCode == VK_MBUTTON) input.mi.dwFlags = isUp ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN;
        else if (vkCode == VK_XBUTTON1) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON1; }
        else if (vkCode == VK_XBUTTON2) { input.mi.dwFlags = isUp ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN; input.mi.mouseData = XBUTTON2; }
        
        // التوقيع السري للماوس
        input.mi.dwExtraInfo = MAGIC_SIGNATURE; 
    } else { 
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = isUp ? KEYEVENTF_KEYUP : 0;
        
        // التوقيع السري للكيبورد
        input.ki.dwExtraInfo = MAGIC_SIGNATURE; 
    }
    
    SendInput(1, &input, sizeof(INPUT));
}

// --- معالجة المنطق (Rapid Fire) ---
void ProcessInputLogic() {
    DWORD currentTime = GetTickCount();

    // 1. Rapid Fire Logic
    for (auto& [sourceKey, actions] : g_complex_mappings) {
        // إذا كان المستخدم يضغط الزر حالياً
        if (g_physical_key_status[sourceKey]) {
            for (auto& action : actions) {
                if (action.type == ActionType::RapidFire) {
                    if (currentTime - action.lastExecutionTime >= (DWORD)action.delayMs) {
                        ExecuteSendInput(action.targetVk, false); // Down
                        ExecuteSendInput(action.targetVk, true);  // Up
                        action.lastExecutionTime = currentTime;
                        if (g_key_states.count(action.targetVk)) g_key_states[action.targetVk] = 1.0f;
                    }
                }
            }
        }
    }

    // 2. Animation Decay
    for (auto& pair : g_key_states) {
        if (pair.second > 0.0f) pair.second -= 0.05f;
    }
}

// --- Hook Handler ---
bool HandleHookLogic(int vkCode, bool isUp) {
    // 1. Wizard Setup (أثناء الإعداد، امنع الزر واحفظه)
    if (!isUp) {
        if (g_app_state == AppState::Wizard_WaitForOriginal) {
            g_wiz_source_key = vkCode;
            g_app_state = AppState::Wizard_WaitForTarget;
            return true; // Block
        }
        else if (g_app_state == AppState::Wizard_WaitForTarget) {
            g_wiz_target_key = vkCode;
            g_app_state = AppState::Wizard_Configure;
            return true; // Block
        }
    }

    // 2. Main Remapping Logic
    if (g_complex_mappings.count(vkCode)) {
        // تحديث حالة الزر الفيزيائية
        g_physical_key_status[vkCode] = !isUp;

        auto& actions = g_complex_mappings[vkCode];
        for (auto& action : actions) {
            // الـ Rapid Fire يتم معالجته في الـ Loop الخارجي
            // الـ Single Press يتم معالجته فوراً هنا لضمان السرعة
            if (action.type == ActionType::SinglePress) {
                ExecuteSendInput(action.targetVk, isUp);
                if (!isUp) g_key_states[action.targetVk] = 1.0f;
            }
        }
        return true; // Block original input
    }

    // 3. Visualizer
    if (!isUp) {
        g_key_states[vkCode] = 1.0f;
        g_last_vk_code = vkCode;
        g_last_key_name = GetKeyNameSmart(vkCode);
    }

    return false; // Pass through
}

// --- System Hooks (Critical Fix) ---

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        
        // >>>> الحل النهائي <<<<
        // إذا وجدنا التوقيع السري، هذا يعني أننا من أرسل الزر
        // تجاهله فوراً ومرره للنظام (لا تعالجه مرة أخرى)
        if (p->dwExtraInfo == MAGIC_SIGNATURE) {
            return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        }

        bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        if (HandleHookLogic(p->vkCode, isUp)) return 1;
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* p = (MSLLHOOKSTRUCT*)lParam;

        // >>>> الحل النهائي للماوس <<<<
        if (p->dwExtraInfo == MAGIC_SIGNATURE) {
            return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
        }

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

void RenderUI() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
    
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Pro Input Remapper (Fixed)", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    ImGui::Columns(2, "MainLayout", true);
    ImGui::SetColumnWidth(0, 250);

    ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.5f, 1.0f), "  ULTIMATE STABLE");
    ImGui::Separator();
    
    static int tab = 0;
    ImGui::Dummy(ImVec2(0,10));
    if (ImGui::Button("Dashboard", ImVec2(230, 40))) tab = 0;
    if (ImGui::Button("Mappings", ImVec2(230, 40))) tab = 1;

    ImGui::NextColumn();

    if (tab == 0) {
        ImGui::Text("Last Key Detected: %s", g_last_key_name.c_str());
        ImGui::TextDisabled("System: Active & Stable.");
    }
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
            
            // خيارات التبديل (Swap)
            ImGui::Checkbox("Swap Keys?", &g_wiz_swap_keys);

            if (!g_wiz_swap_keys) {
                ImGui::RadioButton("Single", (int*)&g_wiz_is_rapid, 0); ImGui::SameLine();
                ImGui::RadioButton("Rapid", (int*)&g_wiz_is_rapid, 1);
                if (g_wiz_is_rapid) ImGui::SliderInt("Speed (ms)", &g_wiz_delay, 1, 1000);
            }

            if (ImGui::Button("Save Map")) {
                // حفظ التعيين الأول
                KeyAction act1; 
                act1.targetVk = g_wiz_target_key;
                act1.type = g_wiz_is_rapid ? ActionType::RapidFire : ActionType::SinglePress;
                act1.delayMs = g_wiz_delay;
                g_complex_mappings[g_wiz_source_key].push_back(act1);

                // حفظ التعيين العكسي (إذا تم اختيار التبديل)
                if (g_wiz_swap_keys) {
                    KeyAction act2;
                    act2.targetVk = g_wiz_source_key;
                    act2.type = ActionType::SinglePress; // التبديل دائماً Single
                    act2.delayMs = 0;
                    g_complex_mappings[g_wiz_target_key].push_back(act2);
                }
                g_app_state = AppState::Dashboard;
            }
            ImGui::SameLine(); 
            if (ImGui::Button("Cancel")) g_app_state = AppState::Dashboard;
        }

        ImGui::Separator();
        ImGui::BeginChild("List", ImVec2(0, 300), true);
        
        // عرض وحذف التعيينات
        for (auto it = g_complex_mappings.begin(); it != g_complex_mappings.end(); ) {
            if (it->second.empty()) { it = g_complex_mappings.erase(it); continue; }
            
            if (ImGui::TreeNode(GetKeyNameSmart(it->first).c_str())) {
                ImGui::SameLine(200);
                // زر حذف الزر بالكامل
                if (ImGui::Button(("Del Key##" + std::to_string(it->first)).c_str())) { 
                    it = g_complex_mappings.erase(it); 
                    ImGui::TreePop(); 
                    continue; 
                }
                
                int idx=0;
                for (auto aIt = it->second.begin(); aIt != it->second.end(); ) {
                    ImGui::Text("-> %s (%s)", GetKeyNameSmart(aIt->targetVk).c_str(), aIt->type == ActionType::RapidFire ? "RAPID" : "ONE");
                    ImGui::SameLine(300);
                    // زر حذف وظيفة واحدة
                    if (ImGui::Button(("X##" + std::to_string(it->first) + std::to_string(idx++)).c_str())) 
                        aIt = it->second.erase(aIt); 
                    else ++aIt;
                }
                ImGui::TreePop();
            }
            ++it;
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
