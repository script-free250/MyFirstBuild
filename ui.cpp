#include "ui.h"
#include "imgui/imgui.h"
#include "keyboard_hook.h"
#include <string>

// متغيرات الواجهة
static int selectedTab = 0;

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.60f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
}

void RenderUI() {
    static float opt_alpha = 1.0f;
    ImGui::SetNextWindowBgAlpha(opt_alpha);
    ImGui::Begin("MyFirstBuild Pro - V3.0", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    ImGui::SetWindowSize(ImVec2(650, 480));

    // شريط الحالة
    if (g_HookActive) ImGui::TextColored(ImVec4(0, 1, 0, 1), "[ ACTIVE ]");
    else ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ STOPPED ]");
    ImGui::SameLine();
    ImGui::Text("| CPS: %d", g_CurrentCPS);
    
    // إشعار وضع الربط
    if (g_CurrentBindMode != BIND_NONE) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), " >> PRESS ANY KEY NOW << ");
    }
    ImGui::Separator();

    // التبويبات
    if (ImGui::Button(" Dashboard ", ImVec2(120, 30))) selectedTab = 0; ImGui::SameLine();
    if (ImGui::Button(" Auto Clicker ", ImVec2(120, 30))) selectedTab = 1; ImGui::SameLine();
    if (ImGui::Button(" Remapper ", ImVec2(120, 30))) selectedTab = 2; ImGui::SameLine();
    if (ImGui::Button(" Settings ", ImVec2(120, 30))) selectedTab = 3;
    ImGui::Separator();

    // 1. Dashboard
    if (selectedTab == 0) {
        ImGui::Text("Event Logs:");
        ImGui::BeginChild("LogRegion", ImVec2(0, 300), true);
        for (const auto& log : g_Logs) ImGui::TextUnformatted(log.c_str());
        ImGui::EndChild();
        if (ImGui::Button("Clear Logs")) g_Logs.clear();
    }

    // 2. Auto Clicker
    else if (selectedTab == 1) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Auto Clicker Settings");
        
        ImGui::Checkbox("Random Delay (Humanization)", &g_Settings.RandomDelay);
        if (g_Settings.RandomDelay) {
            ImGui::SliderInt("Min Delay", &g_Settings.MinDelay, 1, 1000);
            ImGui::SliderInt("Max Delay", &g_Settings.MaxDelay, g_Settings.MinDelay, 1000);
        } else {
            ImGui::SliderInt("Delay (ms)", &g_Settings.MinDelay, 1, 1000);
        }

        const char* btns[] = { "Left Click", "Right Click", "Middle Click" };
        ImGui::Combo("Click Type", &g_Settings.ClickMouseButton, btns, IM_ARRAYSIZE(btns));

        ImGui::Spacing();
        ImGui::Separator();
        
        // --- زر اختيار الـ Toggle Key ---
        ImGui::Text("Toggle Key (Start/Stop):");
        ImGui::SameLine();
        std::string keyName = GetKeyName(g_Settings.ToggleKey);
        
        if (g_CurrentBindMode == BIND_TOGGLE_KEY) {
            ImGui::Button("Press Key...", ImVec2(150, 30));
        } else {
            if (ImGui::Button(keyName.c_str(), ImVec2(150, 30))) {
                g_CurrentBindMode = BIND_TOGGLE_KEY;
                AddLog("Waiting for Toggle Key input...");
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(Click to change)");
    }

    // 3. Key Remapper
    else if (selectedTab == 2) {
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Key Remapper");
        ImGui::Checkbox("Enable Remapping", &g_Settings.EnableRemap);
        
        ImGui::Spacing();
        ImGui::Text("Add New Mapping:");
        
        // زر ذكي لإضافة تخطيط جديد
        if (g_CurrentBindMode == BIND_REMAP_FROM) {
            ImGui::Button("Press Original Key...", ImVec2(200, 40));
        } 
        else if (g_CurrentBindMode == BIND_REMAP_TO) {
            ImGui::Button("Press New Key...", ImVec2(200, 40));
        }
        else {
            if (ImGui::Button("Click to Add Mapping", ImVec2(200, 40))) {
                g_CurrentBindMode = BIND_REMAP_FROM;
                AddLog("Step 1: Press the key you want to CHANGE.");
            }
        }

        ImGui::Separator();
        ImGui::Text("Active Mappings:");
        ImGui::BeginChild("MapList", ImVec2(0, 200), true);
        
        int to_remove = -1;
        for (auto const& [key, val] : g_key_mappings) {
            std::string label = GetKeyName(key) + "  -->  " + GetKeyName(val);
            ImGui::Text("%s", label.c_str());
            ImGui::SameLine();
            ImGui::PushID(key);
            if (ImGui::SmallButton("Remove")) to_remove = key;
            ImGui::PopID();
        }
        if (to_remove != -1) g_key_mappings.erase(to_remove);
        ImGui::EndChild();
    }

    // 4. Settings
    else if (selectedTab == 3) {
        ImGui::Checkbox("Always On Top", &g_Settings.AlwaysOnTop);
        ImGui::Checkbox("Sound Effects", &g_Settings.PlaySounds);
        ImGui::SliderFloat("Opacity", &opt_alpha, 0.3f, 1.0f);
        
        if (ImGui::Button("Reset All Settings", ImVec2(150, 30))) {
            g_Settings = AppSettings();
            g_key_mappings.clear();
        }
    }

    ImGui::End();
}
