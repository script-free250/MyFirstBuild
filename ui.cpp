#include "imgui/imgui.h"
#include "keyboard_hook.h" // للوصول إلى المتغيرات المشتركة
#include <string>
#include <vector>
#include <map>

// تعريف المفاتيح التي سنرسمها
struct Key {
    int vkCode;
    ImVec2 pos;
    ImVec2 size;
    std::wstring name;
};

std::map<int, Key> keyboardLayout;
// متغيرات لحالة الواجهة
static int from_key = 'R';
static int to_key = 'G';

void InitializeLayout() {
    int x = 10, y = 80, w = 50, h = 50, s = 5;
    // صف QWERTY
    const int qwertyKeys[] = {'Q','W','E','R','T','Y','U','I','O','P'};
    for(int i = 0; i < 10; ++i) { keyboardLayout[qwertyKeys[i]] = {qwertyKeys[i], ImVec2(x + i*(w+s), y), ImVec2(w, h), std::wstring(1, (wchar_t)qwertyKeys[i])}; }
    // صف ASDF
    y += h + s;
    const int asdfKeys[] = {'A','S','D','F','G','H','J','K','L'};
    for(int i = 0; i < 9; ++i) { keyboardLayout[asdfKeys[i]] = {asdfKeys[i], ImVec2(x + 20 + i*(w+s), y), ImVec2(w, h), std::wstring(1, (wchar_t)asdfKeys[i])}; }
    // صف ZXC
    y += h + s;
    const int zxcvKeys[] = {'Z','X','C','V','B','N','M'};
    for(int i = 0; i < 7; ++i) { keyboardLayout[zxcvKeys[i]] = {zxcvKeys[i], ImVec2(x + 40 + i*(w+s), y), ImVec2(w, h), std::wstring(1, (wchar_t)zxcvKeys[i])}; }
    // مفاتيح خاصة
    keyboardLayout[VK_SPACE] = {VK_SPACE, ImVec2(x + 150, y + h + s), ImVec2(w * 5, h), L"SPACE"};
    keyboardLayout[VK_LSHIFT] = {VK_LSHIFT, ImVec2(x, y), ImVec2(w * 2 + 15, h), L"SHIFT"};
}

// الدالة الرئيسية لرسم الواجهة
void RenderUI() {
    if (keyboardLayout.empty()) InitializeLayout();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // -- القسم الأيسر: اختبار الكيبورد المرئي --
    ImGui::BeginChild("Tester", ImVec2(ImGui::GetContentRegionAvail().x * 0.65f, 0), true);
    ImGui::Text("Visual Keyboard Tester");
    ImGui::Separator();
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();

    for (auto& pair : keyboardLayout) {
        Key& key = pair.second;
        ImVec2 key_pos = ImVec2(p.x + key.pos.x, p.y + key.pos.y);
        ImVec2 key_end = ImVec2(key_pos.x + key.size.x, key_pos.y + key.size.y);
        
        // الأنيميشن
        float alpha = 0.0f;
        if (g_key_states.count(key.vkCode)) {
            alpha = g_key_states[key.vkCode] / 255.0f;
        }

        // رسم المفتاح
        draw_list->AddRectFilled(key_pos, key_end, IM_COL32(50, 50, 50, 255), 8.0f);
        // رسم التوهج (الأنيميشن)
        if (alpha > 0) {
            draw_list->AddRectFilled(key_pos, key_end, IM_COL32(0, 150, 255, (int)(alpha * 255)), 8.0f);
        }
        // رسم النص
        ImVec2 text_size = ImGui::CalcTextSize(std::string(key.name.begin(), key.name.end()).c_str());
        draw_list->AddText(ImVec2(key_pos.x + (key.size.x - text_size.x) * 0.5f, key_pos.y + (key.size.y - text_size.y) * 0.5f), IM_COL32(255, 255, 255, 255), std::string(key.name.begin(), key.name.end()).c_str());
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // -- القسم الأيمن: التخصيص والإحصائيات --
    ImGui::BeginChild("Controls", ImVec2(0, 0), true);
    ImGui::Text("Controls & Remapper");
    ImGui::Separator();

    ImGui::Text("Last Key Pressed: %s", g_last_key_name.c_str());
    ImGui::Text("Total Presses: %d", g_key_press_count);
    
    ImGui::Separator();
    ImGui::Text("Remap Keys");

    // (هذا جزء مبسط لواجهة التخصيص)
    ImGui::InputInt("From (VK Code)", &from_key);
    ImGui::InputInt("To (VK Code)", &to_key);
    
    if (ImGui::Button("Add/Update Mapping")) {
        AddOrUpdateMapping(from_key, to_key);
    }

    ImGui::Separator();
    ImGui::Text("Current Mappings:");
    for(const auto& mapping : g_key_mappings){
        ImGui::Text("%d -> %d", mapping.first, mapping.second);
    }
    
    ImGui::EndChild();
    
    ImGui::End();
}
