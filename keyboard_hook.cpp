#include <windows.h>
#include "keyboard_hook.h"
#include <string>
#include <map>

// المتغيرات العامة
std::map<int, int> g_key_mappings;
int g_last_vk_code = 0;
char g_last_key_name[64] = "None"; // استخدام مصفوفة ثابتة بدلاً من std::string لتسريع الهوك
bool g_waiting_for_remap = false;
static HHOOK g_keyboardHook = NULL;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        
        // فقط عند الضغط لتجنب التكرار غير الضروري
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            g_last_vk_code = p->vkCode;
            
            // تحديث الاسم فقط إذا طلبته الواجهة لتخفيف العبء (أو بشكل سريع)
            GetKeyNameTextA(p->scanCode << 16, g_last_key_name, sizeof(g_last_key_name));

            // وضع الانتظار للتخصيص
            if (g_waiting_for_remap) {
                return 1; // منع المفتاح
            }

            // تطبيق الـ Remap
            if (g_key_mappings.count(p->vkCode)) {
                int newVkCode = g_key_mappings[p->vkCode];
                
                // الحماية من الـ Loop (إذا كان المفتاح المعين هو نفسه المضغوط)
                if (newVkCode != p->vkCode) {
                    INPUT input = { 0 };
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = (WORD)newVkCode;
                    SendInput(1, &input, sizeof(INPUT));
                    return 1; // إلغاء المفتاح الأصلي
                }
            }
        }
        
        // معالجة رفع المفتاح
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (g_key_mappings.count(p->vkCode) && !g_waiting_for_remap) {
                int newVkCode = g_key_mappings[p->vkCode];
                if (newVkCode != p->vkCode) {
                    INPUT input = { 0 };
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = (WORD)newVkCode;
                    input.ki.dwFlags = KEYEVENTF_KEYUP;
                    SendInput(1, &input, sizeof(INPUT));
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void InstallHook() {
    if (!g_keyboardHook)
        g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
}

void UninstallHook() {
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }
}

void AddMapping(int from, int to) {
    if (from != 0 && to != 0 && from != to) {
        g_key_mappings[from] = to;
    }
}

void ClearMappings() {
    g_key_mappings.clear();
}
