#include <windows.h>

// خطوة 1: تعريف "معرّف" فريد للزر الخاص بنا
#define IDC_GAME_BUTTON 101

// هذه الدالة تتعامل مع كل أحداث النافذة
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // خطوة 3: إضافة حالة جديدة للتعامل مع الأوامر (مثل ضغطات الأزرار)
        case WM_COMMAND:
            // نتحقق مما إذا كان الأمر قد جاء من الزر الخاص بنا
            if (LOWORD(wParam) == IDC_GAME_BUTTON) {
                // "اللعبة" البسيطة: عرض صندوق رسالة
                MessageBox(hwnd, "لقد فزت!", "تهانينا", MB_OK | MB_ICONINFORMATION);
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// نقطة بداية البرنامج الرسومي
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ... (كود تصميم وتسجيل النافذة يبقى كما هو) ...
    WNDCLASSEX wc;
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "MyWindowClass";
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // إنشاء النافذة الرئيسية
    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "MyWindowClass", "My First Game", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 480, 320, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // خطوة 2: إنشاء الزر داخل النافذة الرئيسية
    CreateWindowEx(
        0,
        "BUTTON",                  // نوع العنصر: زر
        "اضغط هنا للعب!",         // النص الذي سيظهر على الزر
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, // خصائص الزر
        170, 120, 120, 40,         // إحداثيات (x, y) وأبعاد (عرض, ارتفاع) الزر
        hwnd,                      // النافذة الأم (الرئيسية) التي سيظهر بداخلها
        (HMENU)IDC_GAME_BUTTON,    // ربط الزر بالمعرف الفريد الذي عرفناه
        hInstance,
        NULL);

    // إظهار النافذة على الشاشة
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // ... (حلقة الرسائل تبقى كما هي) ...
    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
