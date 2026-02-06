// نستخدم مكتبة ويندوز لإنشاء الواجهات الرسومية
#include <windows.h>

// هذه الدالة تتعامل مع أحداث النافذة (مثل الضغط على زر الإغلاق)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
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

// هذه هي نقطة بداية البرنامج الرسومي (بدلاً من main)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // تصميم شكل النافذة
    WNDCLASSEX wc;
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "MyWindowClass";
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    // تسجيل شكل النافذة مع ويندوز
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // إنشاء النافذة
    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "MyWindowClass",
        "My First GUI Program", // هذا هو عنوان النافذة
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 320, // أبعاد النافذة
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // إظهار النافذة على الشاشة
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // حلقة الرسائل (تبقي البرنامج يعمل)
    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
