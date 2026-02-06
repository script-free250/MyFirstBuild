#include <windows.h>
#include <map>

static HHOOK g_keyboardHook = NULL;
static std::map<int, int> g_keyMappings;

// هذه الدالة هي التي تعترض ضغطات المفاتيح
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // تحقق مما إذا كان المفتاح موجوداً في قائمة التخصيص
            if (g_keyMappings.count(p->vkCode)) {
                int newVkCode = g_keyMappings[p->vkCode];
                
                // محاكاة ضغطة المفتاح الجديد
                INPUT input[2];
                ZeroMemory(input, sizeof(input));
                input[0].type = INPUT_KEYBOARD;
                input[0].ki.wVk = newVkCode;
                input[1].type = INPUT_KEYBOARD;
                input[1].ki.wVk = newVkCode;
                input[1].ki.dwFlags = KEYEVENTF_KEYUP;
                
                SendInput(2, input, sizeof(INPUT));
                
                return 1; // منع المفتاح الأصلي من المرور
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void InstallHook() {
    // -- مثال: تغيير مفتاح R ليصبح G --
    g_keyMappings['R'] = 'G';
    
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
}

void UninstallHook() {
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
    }
}
