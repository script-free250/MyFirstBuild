#include "ui.h"
#include "imgui.h"
#include "keyboard_hook.h"
#include <string>

// Helper to center text
void TextCentered(std::string text) {
    float windowWidth = ImGui::GetWindowSize().x;
    float textWidth = ImGui::CalcTextSize(text.c_str()).x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text("%s", text.c_str());
}

void ApplyStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    
    // Professional Dark Theme Colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
}

void RenderUI() {
    // Main Window
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("MyFirstBuild - Pro Edition", nullptr, ImGuiWindowFlags_NoCollapse);

    // 1. Status Header
    if (g_is_active) 
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "[ ACTIVE ] System is Running");
    else 
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ DISABLED ] System Paused (Press F10)");
    
    ImGui::Separator();

    [...](asc_slot://start-slot-5)// 2. CPS Test Feature
    ImGui::Text("Clicks Per Second (CPS) Test:");
    ImGui::ProgressBar(g_current_cps / 20.0f, ImVec2(-1, 0), std::to_string((int)g_current_cps).c_str());

    ImGui::Spacing();
    ImGui::Separator();

    // 3. Remapping Section
    ImGui::Text("Key Remapper:");
    
    static int key_from = 0;
    static int key_to = 0;
    
    ImGui::InputInt("Key to Replace (VK Code)", &key_from);
    ImGui::InputInt("New Key (VK Code)", &key_to);

    if (ImGui::Button("Add Mapping", ImVec2(-1, 30))) {
        if (key_from != 0 && key_to != 0) {
            g_key_mappings[key_from] = key_to;
        }
    }

    // List current mappings
    if (ImGui::BeginChild("Mappings", ImVec2(0, 100), true)) {
        for (auto const& [key, val] : g_key_mappings) {
            ImGui::Text("Key %d -> Key %d", key, val);
            ImGui::SameLine();
            if (ImGui::Button(("Remove##" + std::to_string(key)).c_str())) {
                g_key_mappings.erase(key);
                break; 
            }
        }
        ImGui::EndChild();
    }

    ImGui::Separator();

    // 4. Visual Feedback (Pressed Keys)
    ImGui::Text("Live Key Visualization:");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    
    float x_offset = 0;
    for (auto const& [key, alpha] : g_key_states) {
        if (alpha > 0) {
            char buf[32];
            sprintf_s(buf, "VK:%d", key);
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(p.x + x_offset, p.y), 
                ImVec2(p.x + x_offset + 50, p.y + 30), 
                IM_COL32(46, 204, 113, alpha)
            );
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(p.x + x_offset + 5, p.y + 5), 
                IM_COL32(255, 255, 255, 255), 
                buf
            );
            x_offset += 55;
        }
    }

    [...](asc_slot://start-slot-7)ImGui::End();
}
