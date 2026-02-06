#include "ui.h"
#include "imgui/imgui.h"
#include "keyboard_hook.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio> // for sprintf

static int selected_key_from = 0;
static int selected_key_to = 0;
static char search_buffer[128] = "";
static int current_tab = 0;

void RenderUI() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowFlags(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus);

    if (ImGui::Begin("MainUI", nullptr)) {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "MY FIRST BUILD - ULTIMATE KEYBOARD UTILITY");
        ImGui::Separator();

        if (ImGui::Button("Remap", ImVec2(100, 30))) current_tab = 0; ImGui::SameLine();
        if (ImGui::Button("Stats", ImVec2(100, 30))) current_tab = 2; ImGui::SameLine();
        if (ImGui::Button("Settings", ImVec2(100, 30))) current_tab = 3;

        ImGui::Separator();

        if (current_tab == 0) {
            ImGui::Columns(2, "RemapCols", true);
            ImGui::Text("Add New Mapping");
            ImGui::InputInt("Key Code (From)", &selected_key_from);
            ImGui::TextDisabled("Last Pressed: %d", g_last_pressed_key);
            if (ImGui::Button("Use Last")) selected_key_from = g_last_pressed_key;
            ImGui::InputInt("Key Code (To)", &selected_key_to);
            if (ImGui::Button("Add Mapping")) {
                if (selected_key_from != 0) g_key_mappings[selected_key_from] = selected_key_to;
            }
            ImGui::NextColumn();
            ImGui::Text("Active Mappings");
            for (auto it = g_key_mappings.begin(); it != g_key_mappings.end(); ) {
                char label[64];
                sprintf(label, "%d -> %d", it->first, it->second);
                if (ImGui::Selectable(label)) {
                    selected_key_from = it->first;
                    selected_key_to = it->second;
                }
                ++it;
            }
            ImGui::Columns(1);
        }

        if (current_tab == 2) {
            for (auto const& [key, stats] : g_key_stats) {
                ImGui::Text("Key %d: %d presses", key, stats.pressCount);
            }
        }

        if (current_tab == 3) {
            ImGui::Checkbox("Game Mode", &g_game_mode_active);
            ImGui::Checkbox("Sound Effects", &g_sound_enabled);
        }
    }
    ImGui::End();
}
