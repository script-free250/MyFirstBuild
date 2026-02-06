#include "ui.h"
#include "imgui/imgui.h"
#include "keyboard_hook.h"
#include <string>
#include <vector>

// State variables for UI
static int selectedTab = 0;
static int remapKeyFrom = 0;
static int remapKeyTo = 0;
static bool isWaitingForKey = false;

// Helpers
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

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
}

void RenderUI() {
    static bool opt_always_on_top = false;
    static float opt_alpha = 1.0f;

    // --- Window Flags ---
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    if (opt_always_on_top) {
        // Note: In a real app, you'd apply this to the HWND, but here we just simulate the UI state
    }

    ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(opt_alpha);

    ImGui::Begin("MyFirstBuild Pro - V2.0", NULL, window_flags);

    // --- Header / Dashboard ---
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "Status: %s", g_HookActive ? "Active" : "Stopped");
    ImGui::SameLine();
    ImGui::Text("| CPS: %d", g_CurrentCPS);
    ImGui::SameLine();
    ImGui::Text("| Panic Key: END");
    ImGui::Separator();

    // --- Tabs ---
    if (ImGui::Button("Dashboard", ImVec2(100, 30))) selectedTab = 0; ImGui::SameLine();
    if (ImGui::Button("Auto Clicker", ImVec2(100, 30))) selectedTab = 1; ImGui::SameLine();
    if (ImGui::Button("Key Remapper", ImVec2(100, 30))) selectedTab = 2; ImGui::SameLine();
    if (ImGui::Button("Settings", ImVec2(100, 30))) selectedTab = 3;
    ImGui::Separator();

    // --- Tab Content ---
    if (selectedTab == 0) { // Dashboard
        ImGui::Text("Welcome to the Professional Build.");
        ImGui::Spacing();
        
        // Stats
        ImGui::Columns(2, "stats");
        ImGui::Text("Memory Usage:"); 
        ImGui::ProgressBar(0.4f, ImVec2(-1, 0.0f), "45 MB"); // Mockup
        ImGui::NextColumn();
        ImGui::Text("CPU Usage:");
        ImGui::ProgressBar(0.1f, ImVec2(-1, 0.0f), "2%"); // Mockup
        ImGui::Columns(1);

        ImGui::Spacing();
        ImGui::Text("Event Log:");
        ImGui::BeginChild("LogRegion", ImVec2(0, 150), true);
        for (const auto& log : g_Logs) {
            ImGui::TextUnformatted(log.c_str());
        }
        ImGui::EndChild();
    }
    else if (selectedTab == 1) { // Auto Clicker
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Auto Clicker Configuration");
        
        ImGui::Checkbox("Enable Random Delay (Humanization)", &g_Settings.RandomDelay);
        HelpMarker("Adds random variation to clicks to prevent detection.");

        if (g_Settings.RandomDelay) {
            ImGui::SliderInt("Min Delay (ms)", &g_Settings.MinDelay, 1, 1000);
            ImGui::SliderInt("Max Delay (ms)", &g_Settings.MaxDelay, g_Settings.MinDelay, 1000);
        } else {
            ImGui::SliderInt("Fixed Delay (ms)", &g_Settings.MinDelay, 1, 1000);
        }

        const char* buttons[] = { "Left Click", "Right Click", "Middle Click" };
        ImGui::Combo("Mouse Button", &g_Settings.ClickMouseButton, buttons, IM_ARRAYSIZE(buttons));

        ImGui::Text("Toggle Key: F8 (Fixed for now)");
        
        if (ImGui::Button(g_AutoClickerRunning ? "STOP (F8)" : "START (F8)", ImVec2(200, 50))) {
            ToggleAutoClicker();
        }
    }
    else if (selectedTab == 2) { // Key Remapper
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Key Remapper (Edit Buttons)");
        ImGui::Text("Map one key to another.");
        
        ImGui::Checkbox("Enable Remapping", &g_Settings.EnableRemap);
        
        ImGui::Separator();
        ImGui::InputInt("From Key Code (VK)", &remapKeyFrom);
        ImGui::InputInt("To Key Code (VK)", &remapKeyTo);
        
        if (ImGui::Button("Add Mapping")) {
            if (remapKeyFrom > 0 && remapKeyTo > 0) {
                g_key_mappings[remapKeyFrom] = remapKeyTo;
                AddLog("Added mapping.");
            }
        }

        ImGui::Separator();
        ImGui::Text("Active Mappings:");
        ImGui::BeginChild("MappingsList", ImVec2(0, 150), true);
        int id_to_remove = -1;
        for (auto const& [key, val] : g_key_mappings) {
            ImGui::Text("VK %d -> VK %d", key, val);
            ImGui::SameLine();
            ImGui::PushID(key);
            if (ImGui::SmallButton("Delete")) {
                id_to_remove = key;
            }
            ImGui::PopID();
        }
        if (id_to_remove != -1) g_key_mappings.erase(id_to_remove);
        ImGui::EndChild();
    }
    else if (selectedTab == 3) { // Settings
        ImGui::Text("Application Settings");
        ImGui::Checkbox("Play Sound Effects", &g_Settings.PlaySounds);
        ImGui::Checkbox("Always On Top", &opt_always_on_top);
        ImGui::SliderFloat("Window Opacity", &opt_alpha, 0.2f, 1.0f);
        
        if (ImGui::Button("Save Config")) {
            AddLog("Config Saved (Simulated).");
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults")) {
            g_Settings = AppSettings(); // Reset
            AddLog("Settings Reset.");
        }
    }

    [...](asc_slot://start-slot-7)ImGui::End();
}
