#include "imgui/imgui.h"
#include "keyboard_hook.h"
#include <string>
#include <vector>

// هذا الكود مسؤول عن رسم كل شيء تراه على الشاشة
void RenderUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Ultimate Keyboard Utility");
    ImGui::Separator();

    // -- نافذة تخصيص المفاتيح --
    ImGui::BeginChild("Remapper", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), true);
    ImGui::Text("Key Remapper");
    ImGui::Separator();
    
    // (هنا سنضع واجهة إضافة وإزالة التخصيصات)
    ImGui::Text("Feature coming soon!");

    ImGui::EndChild();

    ImGui::SameLine();

    // -- نافذة اختبار الكيبورد المرئي --
    ImGui::BeginChild("Tester", ImVec2(0, 0), true);
    ImGui::Text("Visual Keyboard Tester");
    ImGui::Separator();
    
    // (هنا سنضع الكود الذي يرسم الكيبورد والأنيميشن)
    ImGui::Text("Visualizer coming soon!");

    ImGui::EndChild();
    
    ImGui::End();
}
