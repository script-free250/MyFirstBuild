#include <windows.h>
#include "imgui/imgui.h"
#include "keyboard_hook.h"
#include <string>
#include <vector>
#include <map>

// بنية لتمثيل المفتاح
struct Key {
    std::string name;
    ImVec2 pos;
    ImVec2 size;
};

// خريطة لربط كود المفتاح ببياناته
std::map<int, Key> keyboardLayout;

// متغيرات للتخصيص
static int from_key = 0;
static std::string from_key_name = "None";
static int to_key = 0;
static std::string to_key_name = "None";

// دالة لإنشاء تخطيط الكيبورد الكامل
void InitializeLayout() {
    float x = 10, y = 50, w = 40, h = 40, s = 4;
    // ... (هنا سنضع الكود الكامل لتخطيط الكيبورد)
    // الصف العلوي
    keyboardLayout[VK_ESCAPE] = {"Esc", {x, y}, {w, h}};
    for (int i = 0; i < 12; i++) keyboardLayout[VK_F1 + i] = {"F" + std::to_string(i + 1), {x + (w*1.5f) + i*(w+s), y}, {w, h}};
    // صف الأرقام
    y += h + s;
    const char* num_keys = "`1234567890-=";
    const int num_vk[] = {VK_OEM_3, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', VK_OEM_MINUS, VK_OEM_PLUS};
    for (int i = 0; i < 13; i++) keyboardLayout[num_vk[i]] = {{1, num_keys[i]}, {x + i*(w+s), y}, {w, h}};
    keyboardLayout[VK_BACK] = {"Backspace", {x + 13*(w+s), y}, {w*2, h}};
    // صف QWERTY
    y += h + s;
    keyboardLayout[VK_TAB] = {"Tab", {x, y}, {w*1.5f, h}};
    const char* qwerty_keys = "QWERTYUIOP[]\\";
    const int qwerty_vk[] = {'Q','W','E','R','T','Y','U','I','O','P', VK_OEM_4, VK_OEM_6, VK_OEM_5};
    for (int i = 0; i < 13; i++) keyboardLayout[qwerty_vk[i]] = {{1, qwerty_keys[i]}, {x + (w*1.5f) + s + i*(w+s), y}, {w, h}};
    // صف ASDF
    y += h + s;
    keyboardLayout[VK_CAPITAL] = {"Caps Lock", {x, y}, {w*1.8f, h}};
    const char* asdf_keys = "ASDFGHJKL;'";
    const int asdf_vk[] = {'A','S','D','F','G','H','J','K','L', VK_OEM_1, VK_OEM_7};
    for (int i = 0; i < 11; i++) keyboardLayout[asdf_vk[i]] = {{1, asdf_keys[i]}, {x + (w*1.8f) + s + i*(w+s), y}, {w, h}};
    keyboardLayout[VK_RETURN] = {"Enter", {x + (w*1.8f) + s + 11*(w+s), y}, {w*2.2f, h}};
    // صف ZXC
    y += h + s;
    keyboardLayout[VK_LSHIFT] = {"L Shift", {x, y}, {w*2.5f, h}};
    const char* zxc_keys = "ZXCVBNM,./";
    const int zxc_vk[] = {'Z','X','C','V','B','N','M', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2};
    for (int i = 0; i < 10; i++) keyboardLayout[zxc_vk[i]] = {{1, zxc_keys[i]}, {x + (w*2.5f) + s + i*(w+s), y}, {w, h}};
    keyboardLayout[VK_RSHIFT] = {"R Shift", {x + (w*2.5f) + s + 10*(w+s), y}, {w*2.8f, h}};
    // الصف الأخير
    y += h + s;
    keyboardLayout[VK_LCONTROL] = {"L Ctrl", {x, y}, {w*1.5f, h}};
    keyboardLayout[VK_LWIN] = {"Win", {x + w*1.5f+s, y}, {w*1.2f, h}};
    keyboardLayout[VK_LMENU] = {"L Alt", {x + w*2.7f+s*2, y}, {w*1.2f, h}};
    keyboardLayout[VK_SPACE] = {"Space", {x + w*3.9f+s*3, y}, {w*5, h}};
    // الأسهم
    keyboardLayout[VK_UP] = {"Up", {x + (w*2.5f) + s + 11*(w+s) + s, y - (h+s)}, {w, h}};
    keyboardLayout[VK_LEFT] = {"Left", {x + (w*2.5f) + s + 10*(w+s), y}, {w, h}};
    keyboardLayout[VK_DOWN] = {"Down", {x + (w*2.5f) + s + 11*(w+s), y}, {w, h}};
    keyboardLayout[VK_RIGHT] = {"Right", {x + (w*2.5f) + s + 12*(w+s), y}, {w, h}};
}

// الدالة الرئيسية لرسم الواجهة
void RenderUI() {
    if (keyboardLayout.empty()) InitializeLayout();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

    ImGui::BeginChild("Tester", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.6f), false, ImGuiWindowFlags_NoScrollbar);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();

    for (auto& pair : keyboardLayout) {
        Key& key = pair.second;
        ImVec2 key_pos = ImVec2(p.x + key.pos.x, p.y + key.pos.y);
        ImVec2 key_end = ImVec2(key_pos.x + key.size.x, key_pos.y + key.size.y);
        
        float alpha = 0.0f;
        if (g_key_states.count(pair.first)) { alpha = g_key_states[pair.first] / 255.0f; }

        draw_list->AddRectFilled(key_pos, key_end, IM_COL32(45, 45, 48, 255), 4.0f);
        if (alpha > 0) { draw_list->AddRectFilled(key_pos, key_end, IM_COL32(0, 120, 215, (int)(alpha * 255)), 4.0f); }
        draw_list->AddRect(key_pos, key_end, IM_COL32(30, 30, 30, 255), 4.0f);
        
        ImVec2 text_size = ImGui::CalcTextSize(key.name.c_str());
        draw_list->AddText(ImVec2(key_pos.x + (key.size.x - text_size.x) * 0.5f, key_pos.y + (key.size.y - text_size.y) * 0.5f), IM_COL32(220, 220, 220, 255), key.name.c_str());
    }
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::BeginChild("Controls", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::Columns(2, "MyColumns", false);

    // العمود الأيسر: المعلومات
    ImGui::Text("Information");
    ImGui::Separator();
    ImGui::Text("Last Key Pressed: %s", g_last_key_name.c_str());
    ImGui::Text("Virtual-Key Code: %d", g_last_vk_code);
    ImGui::Text("Total Presses: %d", g_key_press_count);
    
    ImGui::NextColumn();

    // العمود الأيمن: التخصيص
    ImGui::Text("Key Remapper");
    ImGui::Separator();

    if (g_remap_state == RemapState::WaitingForFrom) {
        ImGui::Text("Press the key you want to CHANGE...");
        if (g_last_vk_code != 0) {
            from_key = g_last_vk_code;
            from_key_name = g_last_key_name;
            g_remap_state = RemapState::None;
        }
    } else if (g_remap_state == RemapState::WaitingForTo) {
        ImGui::Text("Press the NEW key...");
        if (g_last_vk_code != 0) {
            to_key = g_last_vk_code;
            to_key_name = g_last_key_name;
            g_remap_state = RemapState::None;
        }
    }

    ImGui::Text("From: %s (%d)", from_key_name.c_str(), from_key);
    if (ImGui::Button("Set 'From' Key")) { g_remap_state = RemapState::WaitingForFrom; g_last_vk_code = 0; }
    
    ImGui::Spacing();
    
    ImGui::Text("To: %s (%d)", to_key_name.c_str(), to_key);
    if (ImGui::Button("Set 'To' Key")) { g_remap_state = RemapState::WaitingForTo; g_last_vk_code = 0; }

    ImGui::Spacing();
    
    if (ImGui::Button("Apply Mapping")) { AddOrUpdateMapping(from_key, to_key); }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Current Mappings:");
    for(const auto& mapping : g_key_mappings){ ImGui::Text("%d -> %d", mapping.first, mapping.second); }

    ImGui::EndChild();
    
    ImGui::PopStyleVar();
    ImGui::End();
}
