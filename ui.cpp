#include "ui.h"
#include "imgui/imgui.h"
#include "keyboard_hook.h"
#include <string>

// متغيرات مؤقتة للواجهة
static int current_key_from = 0;
static int current_key_to = 0;
static const char* status_msg = "Ready";

void RenderUI()
{
    // --- Header Section ---
    ImGui::TextDisabled("UltiMap Pro v2.0");
    ImGui::SameLine();
    ImGui::Text("| Total Presses: %d", g_key_press_count);
    ImGui::Separator();

    // --- Control Panel (New Features) ---
    if (ImGui::CollapsingHeader("Settings & Controls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Columns(2, "settings_cols", false); // تقسيم عمودين

        ImGui::Checkbox("Enable Remapping", &g_enable_remap);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle all remapping logic properly.");

        ImGui::Checkbox("Sound Effects", &g_play_sound);
        ImGui::Checkbox("Turbo Mode (Repeat)", &g_turbo_mode);
        
        ImGui::NextColumn();
        
        ImGui::Checkbox("Block Key Mode", &g_block_key_mode);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Selected keys will be blocked instead of remapped.");

        ImGui::Checkbox("Always On Top", &g_always_on_top);
        ImGui::SliderFloat("Opacity", &g_window_opacity, 0.2f, 1.0f);
        
        ImGui::Columns(1); // إنهاء التقسيم
    }

    ImGui::Spacing();

    // --- Remapping Section ---
    if (ImGui::BeginChild("RemapArea", ImVec2(0, 150), true))
    {
        ImGui::Text("Current Key Detector:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "%s (%d)", g_last_key_name.c_str(), g_last_vk_code);

        ImGui::Separator();
        
        if (g_remap_state == RemapState::None) {
            if (ImGui::Button("Add New Mapping", ImVec2(-1, 40))) {
                g_remap_state = RemapState::WaitingForFrom;
                status_msg = "Press the key you want to CHANGE...";
            }
        } else if (g_remap_state == RemapState::WaitingForFrom) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), ">> PRESS SOURCE KEY <<");
            if (g_last_vk_code != 0) {
                current_key_from = g_last_vk_code;
                g_last_vk_code = 0; // Reset
                g_remap_state = RemapState::WaitingForTo;
                status_msg = "Now press the TARGET key...";
            }
        } else if (g_remap_state == RemapState::WaitingForTo) {
             ImGui::TextColored(ImVec4(0, 1, 0, 1), ">> PRESS TARGET KEY <<");
             if (g_last_vk_code != 0) {
                 current_key_to = g_last_vk_code;
                 AddOrUpdateMapping(current_key_from, current_key_to);
                 g_remap_state = RemapState::None;
                 status_msg = "Mapping Added Successfully!";
                 g_last_vk_code = 0;
             }
        }
        ImGui::Text("Status: %s", status_msg);
    }
    ImGui::EndChild();

    // --- Active Mappings List (Table) ---
    ImGui::Text("Active Mappings:");
    if (ImGui::BeginTable("MappingsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Original Key");
        ImGui::TableSetupColumn("Mapped To");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        for (auto it = g_key_mappings.begin(); it != g_key_mappings.end(); )
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", it->first);

            ImGui::TableSetColumnIndex(1);
            if (g_block_key_mode) ImGui::TextColored(ImVec4(1,0,0,1), "BLOCKED");
            else ImGui::Text("%d", it->second);

            ImGui::TableSetColumnIndex(2);
            if (ImGui::SmallButton(("Delete##" + std::to_string(it->first)).c_str())) {
                it = g_key_mappings.erase(it);
            } else {
                ++it;
            }
        }
        ImGui::EndTable();
    }

    // --- Footer Buttons ---
    ImGui::Spacing();
    if (ImGui::Button("Reset All", ImVec2(100, 30))) {
        ResetAll();
        status_msg = "All mappings cleared.";
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?) Help");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Press 'Add New Mapping' then follow instructions.");
}
