#include "imgui/imgui.h"
#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>
#include "keyboard_hook.h"

// --- [UI Header Declarations] ---
void RenderUI();
void SetupStyle();
void StartClickerThread();
void StopClickerThread();
void UpdateLogic();

// --- [Global State for Features] ---
// 15 New Features Variables
struct AppConfig {
    bool is_running = false;            // 1. حالة التشغيل
    int cps = 10;                       // 2. التحكم بالسرعة
    bool random_interval = false;       // 3. عشوائية الوقت (تخفي)
    int random_range = 20;              // مدى العشوائية بالميلي ثانية
    int trigger_key = VK_F1;            // 4. زر التفعيل
    bool sound_enabled = true;          // 5. أصوات النقر
    bool always_on_top = false;         // 6. دائماً في المقدمة
    int click_type = 0;                 // 7. نوع النقر (0=Left, 1=Right, 2=Middle)
    bool dark_mode = true;              // 8. الوضع الليلي
    long long total_clicks = 0;         // 9. عداد النقرات
    bool limit_clicks = false;          // 10. تحديد عدد نقرات معين
    int max_clicks = 1000;              // 11. الحد الأقصى للنقرات
    bool double_click_fix = false;      // 12. منع النقر المزدوج (Debounce)
    bool minimize_on_start = false;     // 13. تصغير عند البدء
    float ui_scale = 1.0f;              // 14. حجم الواجهة
    char status_msg[128] = "Ready";     // 15. [...](asc_slot://start-slot-11)شريط الحالة
} config;

// Threading Variables (The Lag Fix)
std::atomic<bool> g_thread_active = false;
std::thread g_clicker_thread;

// --- [Logic Implementation] ---

void ClickerLoop() {
    while (g_thread_active) {
        if (config.is_running) {
            // التحقق من حد النقرات
            if (config.limit_clicks && config.total_clicks >= config.max_clicks) {
                config.is_running = false;
                strcpy_s(config.status_msg, "Max clicks reached!");
                continue;
            }

            // تنفيذ النقر
            INPUT inputs[2] = { 0 };
            
            DWORD flags_down = 0;
            DWORD flags_up = 0;

            if (config.click_type == 0) { flags_down = MOUSEEVENTF_LEFTDOWN; flags_up = MOUSEEVENTF_LEFTUP; }
            else if (config.click_type == 1) { flags_down = MOUSEEVENTF_RIGHTDOWN; flags_up = MOUSEEVENTF_RIGHTUP; }
            else { flags_down = MOUSEEVENTF_MIDDLEDOWN; flags_up = MOUSEEVENTF_MIDDLEUP; }

            inputs[0].type = INPUT_MOUSE;
            inputs[0].mi.dwFlags = flags_down;
            
            inputs[1].type = INPUT_MOUSE;
            inputs[1].mi.dwFlags = flags_up;

            SendInput(2, inputs, sizeof(INPUT));
            
            config.total_clicks++;

            if (config.sound_enabled && (config.total_clicks % 5 == 0)) { // صوت كل 5 نقرات لعدم الازعاج
                Beep(400, 10); 
            }

            // حساب التأخير (Fix for Lag: Sleep is essential)
            int base_delay = 1000 / (config.cps > 0 ? config.cps : 1);
            int final_delay = base_delay;
            
            if (config.random_interval) {
                final_delay += (rand() % (config.random_range * 2)) - config.random_range;
                if (final_delay < 1) final_delay = 1;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(final_delay));
        } else {
            // إذا لم يكن يعمل، ننتظر قليلاً لعدم استهلاك المعالج
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
    // تفعيل/تعطيل عن طريق زر الكيبورد
    if (GetAsyncKeyState(config.trigger_key) & 0x8000) {
        config.is_running = !config.is_running;
        // Anti-bounce بسيط
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        sprintf_s(config.status_msg, config.is_running ? "Running..." : "Stopped");
    }
}

// --- [UI Styling & Rendering] ---

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.40f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.50f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);
}

void RenderUI() {
    // إعداد النافذة لتكون ملء النافذة الرئيسية
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("MainPanel", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    ImGui::Columns(2, "MainLayout", false);
    ImGui::SetColumnWidth(0, 250); // القائمة الجانبية

    // --- Sidebar ---
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "MY BUILD PRO");
    ImGui::Separator();
    ImGui::Spacing();
    
    static int active_tab = 0;
    if (ImGui::Button(" Clicker Settings ", ImVec2(-1, 40))) active_tab = 0;
    if (ImGui::Button(" Key Remapper ", ImVec2(-1, 40))) active_tab = 1;
    if (ImGui::Button(" Settings ", ImVec2(-1, 40))) active_tab = 2;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Status:");
    ImGui::TextColored(config.is_running ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), config.status_msg);
    ImGui::Text("Clicks: %lld", config.total_clicks);

    ImGui::NextColumn();

    // --- Main Content ---
    ImGui::BeginChild("ContentRegion", ImVec2(0, 0), true);

    if (active_tab == 0) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Auto Clicker Configuration");
        ImGui::Separator();
        ImGui::Spacing();

        // CPS Slider (Feature 2)
        ImGui::Text("Clicks Per Second (CPS):");
        ImGui::SliderInt("##CPS", &config.cps, 1, 100);
        
        // Randomization (Feature 3)
        ImGui::Checkbox("Randomize Interval (Anti-Detect)", &config.random_interval);
        if (config.random_interval) {
            ImGui::SliderInt("Jitter (ms)", &config.random_range, 0, 100);
        }

        ImGui::Spacing();
        
        // Mouse Button Choice (Feature 4)
        ImGui::Text("Mouse Button:");
        const char* click_types[] = { "Left Click", "Right Click", "Middle Click" };
        ImGui::Combo("##ClickType", &config.click_type, click_types, IM_ARRAYSIZE(click_types));

        // Limit Clicks (Feature 10 & 11)
        ImGui::Checkbox("Limit Clicks", &config.limit_clicks);
        if (config.limit_clicks) {
            ImGui::InputInt("Max Clicks", &config.max_clicks);
        }

        ImGui::Spacing();
        ImGui::Separator();
        
        // Big Toggle Button
        if (config.is_running) {
            if (ImGui::Button("STOP (F1)", ImVec2(200, 60))) {
                config.is_running = false;
                strcpy_s(config.status_msg, "Stopped");
            }
        } else {
            if (ImGui::Button("START (F1)", ImVec2(200, 60))) {
                config.is_running = true;
                strcpy_s(config.status_msg, "Running...");
            }
        }
    }
    else if (active_tab == 1) {
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Key Remapper");
        ImGui::Separator();
        
        ImGui::Text("Last Key Pressed: %s (Code: %d)", g_last_key_name, g_last_vk_code);
        
        static int key_from = 0;
        static int key_to = 0;
        
        ImGui::BeginGroup();
        if (ImGui::Button(g_waiting_for_remap ? "Press Key..." : "Select Key to Remap")) {
            g_waiting_for_remap = true;
            // المنطق هنا يحتاج لتكامل مع الهوك لالتقاط المفتاح
        }
        ImGui::EndGroup();
        
        // عرض القائمة الحالية
        ImGui::Text("Current Mappings:");
        for (auto const& [key, val] : g_key_mappings) {
            ImGui::BulletText("Key %d -> Key %d", key, val);
            ImGui::SameLine();
            if (ImGui::SmallButton(("Remove##" + std::to_string(key)).c_str())) {
                g_key_mappings.erase(key);
                break;
            }
        }
    }
    else if (active_tab == 2) {
        ImGui::Text("General Settings");
        ImGui::Separator();
        
        ImGui::Checkbox("Sound Effects", &config.sound_enabled);
        ImGui::Checkbox("Dark Mode", &config.dark_mode); // يحتاج لمنطق تغيير الستايل
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            if (config.dark_mode) ImGui::StyleColorsDark(); else ImGui::StyleColorsLight();
        }
        ImGui::Checkbox("Always On Top", &config.always_on_top); // يحتاج لمنطق SetWindowPos
        ImGui::Checkbox("Minimize to Tray (Simulated)", &config.minimize_on_start);
        
        ImGui::Spacing();
        if (ImGui::Button("Reset All Settings")) {
            config.cps = 10;
            config.total_clicks = 0;
        }
    }

    ImGui::EndChild();
    ImGui::End();
}
