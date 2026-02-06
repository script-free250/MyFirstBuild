#include "ui.h"
#include "imgui/imgui.h"
#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>
#include "keyboard_hook.h"

// --- Global Config ---
struct AppConfig {
    bool is_running = false;
    int cps = 10;
    bool random_interval = false;
    int random_range = 20;
    int trigger_key = VK_F1;
    bool sound_enabled = true;
    bool always_on_top = false;
    int click_type = 0; // 0=Left, 1=Right, 2=Middle
    bool dark_mode = true;
    long long total_clicks = 0;
    bool limit_clicks = false;
    int max_clicks = 1000;
    bool double_click_fix = false;
    bool minimize_on_start = false;
    float ui_scale = 1.0f;
    char status_msg[128] = "Ready";
} config;

std::atomic<bool> g_thread_active = false;
std::thread g_clicker_thread;

// --- Logic ---

void ClickerLoop() {
    while (g_thread_active) {
        if (config.is_running) {
            if (config.limit_clicks && config.total_clicks >= config.max_clicks) {
                config.is_running = false;
                strcpy_s(config.status_msg, "Max clicks reached!");
                continue;
            }

            INPUT inputs[2] = { 0 };
            DWORD flags_down = 0, flags_up = 0;

            if (config.click_type == 0) { flags_down = MOUSEEVENTF_LEFTDOWN; flags_up = MOUSEEVENTF_LEFTUP; }
            else if (config.click_type == 1) { flags_down = MOUSEEVENTF_RIGHTDOWN; flags_up = MOUSEEVENTF_RIGHTUP; }
            else { flags_down = MOUSEEVENTF_MIDDLEDOWN; flags_up = MOUSEEVENTF_MIDDLEUP; }

            inputs[0].type = INPUT_MOUSE; inputs[0].mi.dwFlags = flags_down;
            inputs[1].type = INPUT_MOUSE; inputs[1].mi.dwFlags = flags_up;

            SendInput(2, inputs, sizeof(INPUT));
            config.total_clicks++;

            if (config.sound_enabled && (config.total_clicks % 5 == 0)) {
                Beep(400, 10);
            }

            int base_delay = 1000 / (config.cps > 0 ? config.cps : 1);
            int final_delay = base_delay;
            
            if (config.random_interval) {
                final_delay += (rand() % (config.random_range * 2)) - config.random_range;
                if (final_delay < 1) final_delay = 1;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(final_delay));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void StartClickerThread() {
    g_thread_active = true;
    g_clicker_thread = std::thread(ClickerLoop);
}

void StopClickerThread() {
    g_thread_active = false;
    if (g_clicker_thread.joinable())
        g_clicker_thread.join();
}

void UpdateLogic() {
    if (GetAsyncKeyState(config.trigger_key) & 0x8000) {
        config.is_running = !config.is_running;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        sprintf_s(config.status_msg, config.is_running ? "Running..." : "Stopped");
    }
}

// --- UI ---

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.40f, 0.60f, 1.00f);
}

void RenderUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("MainPanel", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::Columns(2, "MainLayout", false);
    ImGui::SetColumnWidth(0, 200);

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "MY BUILD PRO");
    ImGui::Separator();
    
    static int active_tab = 0;
    if (ImGui::Button("Clicker", ImVec2(-1, 40))) active_tab = 0;
    if (ImGui::Button("Remapper", ImVec2(-1, 40))) active_tab = 1;
    if (ImGui::Button("Settings", ImVec2(-1, 40))) active_tab = 2;

    ImGui::Spacing();
    ImGui::TextDisabled("Status: %s", config.status_msg);
    ImGui::Text("Clicks: %lld", config.total_clicks);

    ImGui::NextColumn();
    ImGui::BeginChild("Content", ImVec2(0, 0), true);

    if (active_tab == 0) {
        ImGui::Text("Clicker Settings");
        ImGui::Separator();
        ImGui::SliderInt("CPS", &config.cps, 1, 100);
        ImGui::Checkbox("Randomize", &config.random_interval);
        
        const char* click_types[] = { "Left", "Right", "Middle" };
        ImGui::Combo("Type", &config.click_type, click_types, IM_ARRAYSIZE(click_types));

        if (config.is_running) {
            if (ImGui::Button("STOP (F1)", ImVec2(150, 50))) config.is_running = false;
        } else {
            if (ImGui::Button("START (F1)", ImVec2(150, 50))) config.is_running = true;
        }
    }
    else if (active_tab == 1) {
        ImGui::Text("Key Remapper");
        ImGui::Separator();
        ImGui::Text("Last Key: %s (%d)", g_last_key_name, g_last_vk_code);
        
        if (ImGui::Button(g_waiting_for_remap ? "Press Key..." : "Add New Map")) {
            g_waiting_for_remap = true;
        }
        
        for (auto const& [key, val] : g_key_mappings) {
            ImGui::BulletText("%d -> %d", key, val);
            ImGui::SameLine();
            if (ImGui::SmallButton(("Del##" + std::to_string(key)).c_str())) {
                g_key_mappings.erase(key);
                break;
            }
        }
    }
    else if (active_tab == 2) {
        ImGui::Text("Settings");
        ImGui::Checkbox("Sound", &config.sound_enabled);
        ImGui::Checkbox("Dark Mode", &config.dark_mode);
        if (ImGui::Button("Reset")) config.total_clicks = 0;
    }

    ImGui::EndChild();
    ImGui::End();
}
