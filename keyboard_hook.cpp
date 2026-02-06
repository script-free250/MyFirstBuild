#include "keyboard_hook.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

HHOOK hKeyboardHook = NULL;
// تعريف المتغيرات
std::map<int, int> g_key_mappings;
AppSettings g_Settings;
bool g_HookActive = false;
bool g_AutoClickerRunning = false;
int g_CurrentCPS = 0;
std::vector<std::string> g_Logs;

// دالة لإضافة السجلات
void AddLog(const std::string& msg) {
    if (g_Logs.size() > 50) g_Logs.erase(g_Logs.begin());
    g_Logs.push_back(msg);
}

// دالة الخيط (Thread) الخاصة بالأوتو كليكر - تعمل في الخلفية لمنع تهنيج الواجهة
void AutoClickerThread() {
    static int clicksThisSecond = 0;
    static auto lastTime = std::chrono::steady_clock::now();

    while (g_AutoClickerRunning && !g_Settings.PanicMode) {
        // تحديد نوع الزر (يسار، يمين، وسط)
        DWORD flagsDown = MOUSEEVENTF_LEFTDOWN;
        DWORD flagsUp = MOUSEEVENTF_LEFTUP;
        
        if (g_Settings.ClickMouseButton == 1) { flagsDown = MOUSEEVENTF_RIGHTDOWN; flagsUp = MOUSEEVENTF_RIGHTUP; }
        else if (g_Settings.ClickMouseButton == 2) { flagsDown = MOUSEEVENTF_MIDDLEDOWN; flagsUp = MOUSEEVENTF_MIDDLEUP; }

        // إرسال النقرة
        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = flagsDown;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = flagsUp;

        SendInput(2, inputs, sizeof(INPUT)); // تنفيذ النقرة

        // حساب الـ CPS
        clicksThisSecond++;
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count() >= 1) {
            g_CurrentCPS = clicksThisSecond;
            clicksThisSecond = 0;
            lastTime = now;
        }

        // منطق التأخير (ثابت أو عشوائي)
        int delay = g_Settings.MinDelay;
        if (g_Settings.RandomDelay) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> distr(g_Settings.MinDelay, g_Settings.MaxDelay);
            delay = distr(gen);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    g_CurrentCPS = 0;
}

// تشغيل/إيقاف الكليكر
void ToggleAutoClicker() {
    if (g_AutoClickerRunning) {
        g_AutoClickerRunning = false;
        AddLog("[System] Auto Clicker Stopped.");
        if(g_Settings.PlaySounds) Beep(500, 100); // تأثير صوتي
    } else {
        g_AutoClickerRunning = true;
        AddLog("[System] Auto Clicker Started.");
        if(g_Settings.PlaySounds) Beep(1000, 100); // تأثير صوتي
        std::thread(AutoClickerThread).detach(); // تشغيل في خيط منفصل
    }
}

// معالج أحداث الكيبورد (Hook Procedure)
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;

        // +++ الحل النهائي لمشكلة اللاج +++
        // إذا كانت النقرة ناتجة عن برنامج (Injected) نتجاهلها فوراً ولا نعالجها
        if (pKey->flags & LLKHF_INJECTED) {
            return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN) {
            // 1. تفعيل وضع الذعر (Panic Mode)
            if (pKey->vkCode == (DWORD)g_Settings.PanicKey) {
                g_Settings.PanicMode = true;
                g_AutoClickerRunning = false;
                UninstallHook();
                AddLog("[URGENT] PANIC MODE ACTIVATED!");
                return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
            }

            // 2. زر التشغيل/الإيقاف
            if (pKey->vkCode == (DWORD)g_Settings.ToggleKey) {
                ToggleAutoClicker();
                // لا نمنع الزر الأصلي، نمرره
            }

            // 3. ميزة تعديل الأزرار (Key Remapper) - لم يتم حذفها
            if (g_Settings.EnableRemap) {
                if (g_key_mappings.find(pKey->vkCode) != g_key_mappings.end()) {
                    int newKey = g_key_mappings[pKey->vkCode];
                    
                    // محاكاة الزر الجديد
                    INPUT input = { 0 };
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = newKey;
                    SendInput(1, &input, sizeof(INPUT));
                    
                    AddLog("[Remap] Key " + std::to_string(pKey->vkCode) + " -> " + std::to_string(newKey));
                    return 1; // منع الزر الأصلي من الوصول للنظام
                }
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void InstallHook() {
    if (!hKeyboardHook) {
        hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
        g_HookActive = (hKeyboardHook != NULL);
        if (g_HookActive) AddLog("System Hooks Installed.");
    }
}

void UninstallHook() {
    if (hKeyboardHook) {
        UnhookWindowsHookEx(hKeyboardHook);
        hKeyboardHook = NULL;
        g_HookActive = false;
        AddLog("System Hooks Removed.");
    }
}
