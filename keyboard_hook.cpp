#include "keyboard_hook.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

// تعريف المتغيرات
HHOOK hKeyboardHook = NULL;
HHOOK hMouseHook = NULL; // هوك الماوس الجديد

std::map<int, int> g_key_mappings;
AppSettings g_Settings;
bool g_HookActive = false;
bool g_AutoClickerRunning = false;
int g_CurrentCPS = 0;
std::vector<std::string> g_Logs;

// متغيرات الـ Binding
BindMode g_CurrentBindMode = BIND_NONE;
int g_TempKeyFrom = 0;

void AddLog(const std::string& msg) {
    if (g_Logs.size() > 50) g_Logs.erase(g_Logs.begin());
    g_Logs.push_back(msg);
}

// دالة مساعدة لتحويل كود الزر إلى اسم مقروء
std::string GetKeyName(int vkCode) {
    if (vkCode == 0) return "None";
    if (vkCode == VK_LBUTTON) return "Left Mouse";
    if (vkCode == VK_RBUTTON) return "Right Mouse";
    if (vkCode == VK_MBUTTON) return "Middle Mouse";
    if (vkCode == VK_XBUTTON1) return "Mouse Btn 4";
    if (vkCode == VK_XBUTTON2) return "Mouse Btn 5";
    
    char name[128] = {0};
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    int result = GetKeyNameTextA(scanCode << 16, name, 128);
    if (result > 0) return std::string(name);
    return "Key " + std::to_string(vkCode);
}

void AutoClickerThread() {
    static int clicksThisSecond = 0;
    static auto lastTime = std::chrono::steady_clock::now();

    while (g_AutoClickerRunning && !g_Settings.PanicMode) {
        DWORD flagsDown = MOUSEEVENTF_LEFTDOWN;
        DWORD flagsUp = MOUSEEVENTF_LEFTUP;
        
        if (g_Settings.ClickMouseButton == 1) { flagsDown = MOUSEEVENTF_RIGHTDOWN; flagsUp = MOUSEEVENTF_RIGHTUP; }
        else if (g_Settings.ClickMouseButton == 2) { flagsDown = MOUSEEVENTF_MIDDLEDOWN; flagsUp = MOUSEEVENTF_MIDDLEUP; }

        INPUT inputs[2] = {};
        inputs[0].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = flagsDown;
        inputs[1].type = INPUT_MOUSE;
        inputs[1].mi.dwFlags = flagsUp;

        SendInput(2, inputs, sizeof(INPUT));

        clicksThisSecond++;
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastTime).count() >= 1) {
            g_CurrentCPS = clicksThisSecond;
            clicksThisSecond = 0;
            lastTime = now;
        }

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

void ToggleAutoClicker() {
    if (g_AutoClickerRunning) {
        g_AutoClickerRunning = false;
        AddLog("[System] Stopped.");
        if(g_Settings.PlaySounds) Beep(500, 100);
    } else {
        g_AutoClickerRunning = true;
        AddLog("[System] Started.");
        if(g_Settings.PlaySounds) Beep(1000, 100);
        std::thread(AutoClickerThread).detach();
    }
}

// دالة لمعالجة "التقاط الزر" عند التخصيص
bool HandleBinding(int vkCode) {
    if (g_CurrentBindMode == BIND_NONE) return false;

    if (g_CurrentBindMode == BIND_TOGGLE_KEY) {
        g_Settings.ToggleKey = vkCode;
        AddLog("Toggle Key set to: " + GetKeyName(vkCode));
        g_CurrentBindMode = BIND_NONE;
        return true; // منع الزر من التنفيذ
    }
    else if (g_CurrentBindMode == BIND_REMAP_FROM) {
        g_TempKeyFrom = vkCode;
        AddLog("Remap Source set to: " + GetKeyName(vkCode) + ". Now select Target.");
        g_CurrentBindMode = BIND_REMAP_TO; // الانتقال للخطوة التالية
        return true;
    }
    else if (g_CurrentBindMode == BIND_REMAP_TO) {
        if (g_TempKeyFrom != 0) {
            g_key_mappings[g_TempKeyFrom] = vkCode;
            AddLog("Mapping Added: " + GetKeyName(g_TempKeyFrom) + " -> " + GetKeyName(vkCode));
        }
        g_CurrentBindMode = BIND_NONE;
        g_TempKeyFrom = 0;
        return true;
    }
    return false;
}

// هوك الكيبورد
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        
        if (pKey->flags & LLKHF_INJECTED) return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // أولوية لعملية الربط (Binding)
            if (g_CurrentBindMode != BIND_NONE) {
                if (HandleBinding(pKey->vkCode)) return 1;
            }

            if (pKey->vkCode == (DWORD)g_Settings.PanicKey) {
                g_Settings.PanicMode = true;
                g_AutoClickerRunning = false;
                UninstallHook();
                return 1;
            }

            if (pKey->vkCode == (DWORD)g_Settings.ToggleKey) {
                ToggleAutoClicker();
                // لا نمنع الزر إذا كان هو نفسه زر ماوس أو زر عادي، نتركه يمر
            }

            if (g_Settings.EnableRemap && g_key_mappings.count(pKey->vkCode)) {
                int newKey = g_key_mappings[pKey->vkCode];
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = newKey;
                SendInput(1, &input, sizeof(INPUT));
                return 1; 
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

// هوك الماوس (الجديد)
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
        if (pMouse->flags & LLKHF_INJECTED) return CallNextHookEx(hMouseHook, nCode, wParam, lParam);

        int vkCode = 0;
        if (wParam == WM_LBUTTONDOWN) vkCode = VK_LBUTTON;
        else if (wParam == WM_RBUTTONDOWN) vkCode = VK_RBUTTON;
        else if (wParam == WM_MBUTTONDOWN) vkCode = VK_MBUTTON;
        else if (wParam == WM_XBUTTONDOWN) {
            vkCode = (HIWORD(pMouse->mouseData) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
        }

        if (vkCode != 0) {
            // أولوية للربط
            if (g_CurrentBindMode != BIND_NONE) {
                // نمنع الزر الأيسر من الربط إذا كان المستخدم يحاول الضغط على الواجهة
                // لكن هنا الهوك يأتي قبل الواجهة.
                // لتجنب "التعليق"، سنسمح بالزر الأيسر فقط داخل الواجهة،
                // لكن هنا سنلتقطه. كن حذراً: إذا اخترت زر اليسار قد تضغط الزر والواجهة معاً.
                if (HandleBinding(vkCode)) return 1;
            }

            if (vkCode == g_Settings.ToggleKey) {
                ToggleAutoClicker();
            }
        }
    }
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

void InstallHook() {
    if (!hKeyboardHook) {
        hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
        // تثبيت هوك الماوس أيضاً
        hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
        
        g_HookActive = (hKeyboardHook && hMouseHook);
        if (g_HookActive) AddLog("Keyboard & Mouse Hooks Installed.");
    }
}

void UninstallHook() {
    if (hKeyboardHook) { UnhookWindowsHookEx(hKeyboardHook); hKeyboardHook = NULL; }
    if (hMouseHook) { UnhookWindowsHookEx(hMouseHook); hMouseHook = NULL; }
    g_HookActive = false;
    AddLog("Hooks Uninstalled.");
}
