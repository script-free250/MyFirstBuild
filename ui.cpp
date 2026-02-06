#include "ui.h"
#include "imgui/imgui.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>

// --- Global Variables Definition ---
std::map<int, int> g_key_mappings;
std::map<int, float> g_key_states;
int g_key_press_count = 0;
int g_last_vk_code = 0;
std::string g_last_key_name = "None";
RemapState g_remap_state = RemapState::None;

// Internal Hook Handle
HHOOK g_hKeyboardHook = NULL;

// --- Key Structure for Visualization ---
struct KeyDef {
    std::string label;
    int vkCode;
    ImVec2 pos; // Relative position (0-15 grid)
    float width; // Width in grid units
};

// --- QWERTY Layout Definition ---
// Simplified layout for demonstration
std::vector<KeyDef> g_keyboardLayout = {
    // Row 1
    {"ESC", VK_ESCAPE, {0,0}, 1}, {"F1", VK_F1, {2,0}, 1}, {"F2", VK_F2, {3,0}, 1}, {"F3", VK_F3, {4,0}, 1}, {"F4", VK_F4, {5,0}, 1},
    // Row 2
    {"~", 192, {0,1.5}, 1}, {"1", '1', {1,1.5}, 1}, {"2", '2', {2,1.5}, 1}, {"3", '3', {3,1.5}, 1}, {"4", '4', {4,1.5}, 1}, {"5", '5', {5,1.5}, 1},
    {"6", '6', {6,1.5}, 1}, {"7", '7', {7,1.5}, 1}, {"8", '8', {8,1.5}, 1}, {"9", '9', {9,1.5}, 1}, {"0", '0', {10,1.5}, 1}, {"BkSp", VK_BACK, {13,1.5}, 2},
    // Row 3
    {"Tab", VK_TAB, {0,2.5}, 1.5}, {"Q", 'Q', {1.5,2.5}, 1}, {"W", 'W', {2.5,2.5}, 1}, {"E", 'E', {3.5,2.5}, 1}, {"R", 'R', {4.5,2.5}, 1}, {"T", 'T', {5.5,2.5}, 1},
    {"Y", 'Y', {6.5,2.5}, 1}, {"U", 'U', {7.5,2.5}, 1}, {"I", 'I', {8.5,2.5}, 1}, {"O", 'O', {9.5,2.5}, 1}, {"P", 'P', {10.5,2.5}, 1},
    // Row 4
    {"Caps", VK_CAPITAL, {0,3.5}, 1.75}, {"A", 'A', {1.75,3.5}, 1}, {"S", 'S', {2.75,3.5}, 1}, {"D", 'D', {3.75,3.5}, 1}, {"F", 'F', {4.75,3.5}, 1}, {"G", 'G', {5.75,3.5}, 1},
    {"H", 'H', {6.75,3.5}, 1}, {"J", 'J', {7.75,3.5}, 1}, {"K", 'K', {8.75,3.5}, 1}, {"L", 'L', {9.75,3.5}, 1}, {"Enter", VK_RETURN, {12.75,3.5}, 2.25},
    // Row 5
    {"Shift", VK_LSHIFT, {0,4.5}, 2.25}, {"Z", 'Z', {2.25,4.5}, 1}, {"X", 'X', {3.25,4.5}, 1}, {"C", 'C', {4.25,4.5}, 1}, {"V", 'V', {5.25,4.5}, 1}, {"B", 'B', {6.25,4.5}, 1},
    {"N", 'N', {7.25,4.5}, 1}, {"M", 'M', {8.25,4.5}, 1}, {"Shift", VK_RSHIFT, {12.25,4.5}, 2.75},
    // Row 6
    {"Ctrl", VK_LCONTROL, {0,5.5}, 1.5}, {"Win", VK_LWIN, {1.5,5.5}, 1}, {"Alt", VK_LMENU, {2.5,5.5}, 1.25}, {"Space", VK_SPACE, {3.75,5.5}, 6.25}, {"Alt", VK_RMENU, {10,5.5}, 1}, {"Ctrl", VK_RCONTROL, {13.5,5.5}, 1.5}
};

// --- Low Level Keyboard Hook ---
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            g_key_states[pKey->vkCode] = 1.0f; // Set intensity to Max
            g_last_vk_code = pKey->vkCode;
            g_key_press_count++;
            
            // Simple key name retrieval
            char keyNameBuffer[128];
            if (GetKeyNameTextA(pKey->scanCode << 16, keyNameBuffer, 128)) {
                 g_last_key_name = std::string(keyNameBuffer);
            } else {
                 g_last_key_name = std::to_string(pKey->vkCode);
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

// --- Animation Update ---
// Call this every frame before rendering
void UpdateAnimationState() {
    float decaySpeed = 0.05f; // Speed of fade out
    for (auto& pair : g_key_states) {
        if (pair.second > 0.0f) {
            pair.second -= decaySpeed;
            if (pair.second < 0.0f) pair.second = 0.0f;
        }
    }
}

// --- Main Render Function ---
void RenderUI() {
    // 1. Setup Style (Run once or every frame if dynamic)
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.10f, 0.30f, 1.00f);
    
    // Main Window
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1200, 700), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Ultimate Keyboard Utility", NULL, ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("MainTabs")) {
        
        // --- Tab 1: Visualizer ---
        if (ImGui::BeginTabItem("Visualizer")) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.4f, 1.0f, 1.0f), "LIVE KEYBOARD FEEDBACK");
            ImGui::Separator();
            ImGui::Spacing();

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            float scale = 50.0f; // Base size of a key in pixels
            
            for (const auto& key : g_keyboardLayout) {
                float intensity = g_key_states[key.vkCode]; // 0.0 to 1.0
                
                // Color interpolation: Dark Grey -> Neon Purple
                ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(
                    0.2f + (0.6f * intensity), // R
                    0.2f + (0.0f * intensity), // G
                    0.2f + (0.8f * intensity), // B
                    1.0f
                ));

                ImVec2 keyPos = ImVec2(p.x + key.pos.x * scale, p.y + key.pos.y * scale);
                ImVec2 keySize = ImVec2(key.width * scale - 5, scale - 5); // -5 for gap

                // Draw Key Background (Rounded Rect)
                draw_list->AddRectFilled(keyPos, ImVec2(keyPos.x + keySize.x, keyPos.y + keySize.y), color, 4.0f);
                
                // Draw Key Border
                draw_list->AddRect(keyPos, ImVec2(keyPos.x + keySize.x, keyPos.y + keySize.y), IM_COL32(255, 255, 255, 50), 4.0f);

                // Draw Text
                ImVec2 textSize = ImGui::CalcTextSize(key.label.c_str());
                ImVec2 textPos = ImVec2(keyPos.x + (keySize.x - textSize.x) * 0.5f, keyPos.y + (keySize.y - textSize.y) * 0.5f);
                draw_list->AddText(textPos, IM_COL32(255, 255, 255, 255), key.label.c_str());
            }
            
            // Advance cursor to avoid overlap if we add more widgets below
            ImGui::Dummy(ImVec2(0, 350)); 
            ImGui::EndTabItem();
        }

        // --- Tab 2: Stats & Remap ---
        if (ImGui::BeginTabItem("Stats & Settings")) {
            ImGui::Columns(2, "StatCols", false);
            
            // Left Column: Stats
            ImGui::Text("Total Key Presses:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0,1,0,1), "%d", g_key_press_count);
            
            ImGui::Spacing();
            
            ImGui::Text("Last Key Pressed:");
            ImGui::SameLine(); 
            ImGui::TextColored(ImVec4(1,1,0,1), "%s (VK: %d)", g_last_key_name.c_str(), g_last_vk_code);

            ImGui::NextColumn();
            
            // Right Column: Remap (Placeholder)
            ImGui::Text("Key Remapper");
            ImGui::Separator();
            if (ImGui::Button("Start Remapping")) {
                // Logic to start remapping flow
            }
            ImGui::TextDisabled("Remapping feature is currently a placeholder.");
            
            ImGui::Columns(1);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
