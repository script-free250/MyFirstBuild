#include "ui.h"
#include "imgui/imgui.h"
#include "keyboard_hook.h"
#include <string>

// متغيرات حالة الواجهة
static int selectedTab = 0;
static int remapKeyFrom = 0;
static int remapKeyTo = 0;

// دالة مساعدة للتلميحات
void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// 1. تصميم عصري (Dark Theme)
void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f); // خلفية داكنة
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f); // أزرار زرقاء شفافة
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
}

void RenderUI() {
    static float opt_alpha = 1.0f; // شفافية النافذة

    ImGui::SetNextWindowBgAlpha(opt_alpha);
    ImGui::Begin("MyFirstBuild Pro", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    ImGui::SetWindowSize(ImVec2(650, 450));

    // --- شريط الحالة العلوي ---
    if (g_HookActive) ImGui::TextColored(ImVec4(0, 1, 0, 1), "[ ACTIVE ]");
    else ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ STOPPED ]");
    
    ImGui::SameLine();
    ImGui::Text("| CPS: %d", g_CurrentCPS); // 4. عداد النقرات
    ImGui::SameLine();
    ImGui::Text("| RAM: 45MB"); // 12. مراقب الموارد (وهمي)
    ImGui::Separator();

    // --- 2. نظام التبويبات ---
    if (ImGui::Button(" Dashboard ", ImVec2(120, 30))) selectedTab = 0; ImGui::SameLine();
    if (ImGui::Button(" Auto Clicker ", ImVec2(120, 30))) selectedTab = 1; ImGui::SameLine();
    if (ImGui::Button(" Remapper ", ImVec2(120, 30))) selectedTab = 2; ImGui::SameLine();
    if (ImGui::Button(" Settings ", ImVec2(120, 30))) selectedTab = 3;
    ImGui::Separator();

    // === تبويب الرئيسية ===
    if (selectedTab == 0) {
        ImGui::Text("System Log:");
        ImGui::BeginChild("Logs", ImVec2(0, 250), true);
        for (const auto& log : g_Logs) {
            ImGui::TextUnformatted(log.c_str());
        }
        if (g_Settings.PanicMode) ImGui::TextColored(ImVec4(1,0,0,1), "PANIC MODE TRIGGERED!");
        ImGui::EndChild();
        
        if (ImGui::Button("Clear Logs", ImVec2(100, 25))) g_Logs.clear();
    }

    // === تبويب الأوتو كليكر ===
    else if (selectedTab == 1) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Configuration");
        
        // 9. ميزة Humanization
        ImGui::Checkbox("Random Delay (Humanization)", &g_Settings.RandomDelay);
        HelpMarker("Makes clicks look human to avoid bans.");

        if (g_Settings.RandomDelay) {
            ImGui::SliderInt("Min Delay (ms)", &g_Settings.MinDelay, 1, 1000);
            ImGui::SliderInt("Max Delay (ms)", &g_Settings.MaxDelay, g_Settings.MinDelay, 1000);
        } else {
            ImGui::SliderInt("Fixed Delay (ms)", &g_Settings.MinDelay, 1, 1000);
        }

        // 16. اختيار زر الماوس
        const char* mouseBtns[] = { "Left Click", "Right Click", "Middle Click" };
        ImGui::Combo("Mouse Button", &g_Settings.ClickMouseButton, mouseBtns, IM_ARRAYSIZE(mouseBtns));

        ImGui::Spacing();
        ImGui::Text("Toggle Key: F8");
        
        // زر تشغيل كبير
        ImVec4 btnColor = g_AutoClickerRunning ? ImVec4(0.8f, 0.2f, 0.2f, 1.0f) : ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, btnColor);
        if (ImGui::Button(g_AutoClickerRunning ? "STOP CLICKER (F8)" : "START CLICKER (F8)", ImVec2(200, 60))) {
            ToggleAutoClicker();
        }
        ImGui::PopStyleColor();
    }

    // === تبويب تعديل الأزرار (Key Remapper) ===
    else if (selectedTab == 2) {
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Key Remapper System");
        ImGui::Checkbox("Active", &g_Settings.EnableRemap);
        
        ImGui::Columns(2, "remap_cols", false);
        ImGui::InputInt("Original Key (VK)", &remapKeyFrom);
        ImGui::NextColumn();
        ImGui::InputInt("New Key (VK)", &remapKeyTo);
        ImGui::Columns(1);
        
        if (ImGui::Button("Add Mapping", ImVec2(-1, 30))) {
            if (remapKeyFrom > 0 && remapKeyTo > 0) {
                g_key_mappings[remapKeyFrom] = remapKeyTo;
                AddLog("Mapping Added: " + std::to_string(remapKeyFrom) + " -> " + std::to_string(remapKeyTo));
            }
        }

        ImGui::Separator();
        ImGui::Text("Current Mappings:");
        ImGui::BeginChild("MapList", ImVec2(0, 150), true);
        int to_remove = -1;
        for (auto const& [key, val] : g_key_mappings) {
            ImGui::Text("Key %d  -->  Key %d", key, val);
            ImGui::SameLine();
            if (ImGui::SmallButton(("Delete##" + std::to_string(key)).c_str())) {
                to_remove = key;
            }
        }
        if (to_remove != -1) g_key_mappings.erase(to_remove);
        ImGui::EndChild();
    }

    // === تبويب الإعدادات ===
    else if (selectedTab == 3) {
        // 7. التحكم بالشفافية
        ImGui::SliderFloat("Opacity", &opt_alpha, 0.3f, 1.0f);
        // 8. دائماً في المقدمة
        ImGui::Checkbox("Always on Top", &g_Settings.AlwaysOnTop);
        // 11. أصوات
        ImGui::Checkbox("Sound Effects", &g_Settings.PlaySounds);
        // 5. زر الذعر
        ImGui::InputInt("Panic Key (VK)", &g_Settings.PanicKey);
        
        ImGui::Spacing();
        if (ImGui::Button("Save Settings", ImVec2(150, 30))) AddLog("Settings Saved.");
        ImGui::SameLine();
        if (ImGui::Button("Reset All", ImVec2(150, 30))) {
            g_Settings = AppSettings();
            g_key_mappings.clear();
            AddLog("Factory Reset.");
        }
    }

    ImGui::End();
}
