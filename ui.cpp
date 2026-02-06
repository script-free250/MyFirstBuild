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

// متغيرات مؤقتة لعملية تغيير الأزرار
int g_remap_source_key = -1;

HHOOK g_hKeyboardHook = NULL;

// --- Helper: Get Key Name ---
std::string GetKeyName(int vkCode) {
    char keyNameBuffer[128];
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    if (GetKeyNameTextA(scanCode << 16, keyNameBuffer, 128)) {
        return std::string(keyNameBuffer);
    }
    return "Unknown (" + std::to_string(vkCode) + ")";
}

// --- Low Level Keyboard Hook (The Core Logic) ---
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;

        // 1. تجاهل المفاتيح التي قمنا نحن بإرسالها (لتجنب الدوائر اللانهائية)
        if (pKey->flags & LLKHF_INJECTED) {
            return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            
            // --- Logic for Remapping Setup (إعداد التغيير) ---
            if (g_app_state == AppState::Remapping_WaitForOriginal) {
                g_remap_source_key = pKey->vkCode;
                g_app_state = AppState::Remapping_WaitForTarget;
                return 1; // Block key
            }
            else if (g_app_state == AppState::Remapping_WaitForTarget) {
                // حفظ التغيير الجديد
                if (g_remap_source_key != -1) {
                    g_key_mappings[g_remap_source_key] = pKey->vkCode;
                }
                g_app_state = AppState::Dashboard;
                return 1; // Block key
            }

            // --- Logic for Active Remapping (تنفيذ التغيير) ---
            // هل هذا الزر موجود في قائمة التغييرات؟
            if (g_key_mappings.count(pKey->vkCode)) {
                int targetKey = g_key_mappings[pKey->vkCode];
                
                // محاكاة الضغط على الزر الجديد
                INPUT inputs[1] = {};
                inputs[0].type = INPUT_KEYBOARD;
                inputs[0].ki.wVk = targetKey;
                
                // إرسال Down
                SendInput(1, inputs, sizeof(INPUT));
                
                // (في التطبيق الحقيقي نحتاج معالجة KeyUp أيضاً، لكن هذا للتبسيط)
                
                // تحديث الأنيميشن للزر الجديد
                g_key_states[targetKey] = 1.0f; 
                
                return 1; // Block the original key! (منع الزر الأصلي)
            }

            // Visualizer logic (للعرض فقط)
            g_key_states[pKey->vkCode] = 1.0f;
            g_last_vk_code = pKey->vkCode;
            g_key_press_count++;
            g_last_key_name = GetKeyName(pKey->vkCode);
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            // معالجة رفع الزر في حالة التغيير
            if (g_key_mappings.count(pKey->vkCode)) {
                int targetKey = g_key_mappings[pKey->vkCode];
                INPUT inputs[1] = {};
                inputs[0].type = INPUT_KEYBOARD;
                inputs[0].ki.wVk = targetKey;
                inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, inputs, sizeof(INPUT));
                return 1;
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

void InstallHook() {
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
}

void UninstallHook() {
    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
        g_hKeyboardHook = NULL;
    }
}

void UpdateAnimationState() {
    for (auto& pair : g_key_states) {
        if (pair.second > 0.0f) pair.second -= 0.05f;
    }
}

// --- UI Styling Helpers ---
void StyleButton3D(bool active) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    }
}

// --- Main Render Logic ---
void RenderUI() {
    // إعداد الستايل العام
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);

    ImGui::Begin("Keyboard Master Control", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    // --- تقسيم الشاشة: قائمة جانبية ومحتوى رئيسي ---
    ImGui::Columns(2, "MainLayout", true);
    ImGui::SetColumnWidth(0, 250); // عرض القائمة الجانبية

    // --- Side Bar (القائمة الجانبية) ---
    ImGui::Dummy(ImVec2(0, 20));
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "  KEY MASTER");
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 20));

    static int selectedTab = 0;
    if (ImGui::Button("  Dashboard  ", ImVec2(230, 40))) selectedTab = 0;
    ImGui::Dummy(ImVec2(0, 10));
    if (ImGui::Button("  Remapper  ", ImVec2(230, 40))) selectedTab = 1;
    ImGui::Dummy(ImVec2(0, 10));
    
    // حالة البرنامج
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 100);
    ImGui::TextDisabled("  Status: Running");
    ImGui::TextDisabled("  Hook ID: %p", g_hKeyboardHook);

    ImGui::NextColumn();

    // --- Main Content Area ---
    ImGui::Dummy(ImVec2(0, 10));
    
    if (selectedTab == 0) {
        // --- Dashboard Tab ---
        ImGui::Text("Live Input Visualizer");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 20));

        // عرض أزرار كبيرة لمحاكاة الكيبورد
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        
        // رسم مربع كبير يعبر عن حالة الضغط
        float boxSize = 100.0f;
        
        // Key Name Display
        draw->AddRectFilled(p, ImVec2(p.x + 300, p.y + 150), IM_COL32(40, 40, 50, 255), 10.0f);
        draw->AddText(ImGui::GetFont(), 24.0f, ImVec2(p.x + 20, p.y + 20), IM_COL32(255, 255, 255, 255), "Last Key:");
        draw->AddText(ImGui::GetFont(), 40.0f, ImVec2(p.x + 40, p.y + 70), IM_COL32(0, 255, 0, 255), g_last_key_name.c_str());

        // Stats Box
        ImVec2 p2 = ImVec2(p.x + 320, p.y);
        draw->AddRectFilled(p2, ImVec2(p2.x + 200, p2.y + 150), IM_COL32(50, 40, 40, 255), 10.0f);
        draw->AddText(ImGui::GetFont(), 20.0f, ImVec2(p2.x + 20, p2.y + 20), IM_COL32(255, 255, 255, 255), "Total Presses:");
        char countBuf[32]; sprintf_s(countBuf, "%d", g_key_press_count);
        draw->AddText(ImGui::GetFont(), 40.0f, ImVec2(p2.x + 50, p2.y + 70), IM_COL32(255, 100, 100, 255), countBuf);

    } 
    else if (selectedTab == 1) {
        // --- Remapper Tab ---
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Key Remapper Configuration");
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 20));

        // State Machine UI
        if (g_app_state == AppState::Dashboard) {
            if (ImGui::Button(" + ADD NEW REMAP ", ImVec2(200, 50))) {
                g_app_state = AppState::Remapping_WaitForOriginal;
            }
            ImGui::SameLine();
            ImGui::TextWrapped("Click to start mapping a key to another.");
        }
        else if (g_app_state == AppState::Remapping_WaitForOriginal) {
            ImGui::Button(" PRESS KEY TO CHANGE... ", ImVec2(300, 50));
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Press the physical key you want to change now.");
        }
        else if (g_app_state == AppState::Remapping_WaitForTarget) {
            std::string label = " Remap [" + GetKeyName(g_remap_source_key) + "] To... (Press Key)";
            ImGui::Button(label.c_str(), ImVec2(400, 50));
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Now press the key you want it to become.");
        }

        ImGui::Dummy(ImVec2(0, 30));
        ImGui::Text("Active Mappings:");
        ImGui::BeginChild("MappingsList", ImVec2(0, 300), true);
        
        // List mappings
        std::vector<int> toRemove;
        for (auto const& [original, target] : g_key_mappings) {
            ImGui::PushID(original);
            std::string mapText = GetKeyName(original) + "  -->  " + GetKeyName(target);
            ImGui::Text(mapText.c_str());
            ImGui::SameLine(300);
            if (ImGui::Button("Delete")) {
                toRemove.push_back(original);
            }
            ImGui::PopID();
            ImGui::Separator();
        }

        // Clean up deleted
        for (int key : toRemove) g_key_mappings.erase(key);

        ImGui::EndChild();
    }

    ImGui::End();
}
