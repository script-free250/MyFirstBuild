#include <windows.h>

// سنحتاج إلى متغير عام للاحتفاظ بمؤشر (handle) على شاشة العرض النصية
HWND hStaticDisplay;

// دالة لمعالجة أحداث النافذة
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // يتم استدعاء هذه الحالة مرة واحدة فقط عند إنشاء النافذة
        case WM_CREATE: {
            // هنا سننشئ "شاشة" نصية ثابتة في منتصف النافذة
            hStaticDisplay = CreateWindowExW(
                0, L"STATIC",   // نوع العنصر: نص ثابت
                L"Press any key...", // النص المبدئي
                WS_CHILD | WS_VISIBLE | SS_CENTER, // الخصائص: يظهر في المنتصف
                50, 100, 360, 40, // الإحداثيات والأبعاد
                hwnd, NULL, NULL, NULL);

            // تغيير حجم الخط ليبدو احترافياً
            HFONT hFont = CreateFont(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, TEXT("Arial"));
            SendMessage(hStaticDisplay, WM_SETFONT, (WPARAM)hFont, TRUE);
            break;
        }

        // يتم استدعاء هذه الحالة عند الضغط على أي مفتاح في الكيبورد
        case WM_KEYDOWN: {
            // مصفوفة لتخزين اسم المفتاح
            wchar_t keyName[256];
            // lParam يحتوي على معلومات إضافية عن المفتاح
            // GetKeyNameTextW هي دالة ويندوز سحرية تحول كود المفتاح إلى اسمه الحقيقي
            if (GetKeyNameTextW(lParam, keyName, sizeof(keyName) / sizeof(wchar_t)) > 0) {
                // نغير النص المعروض على الشاشة ليحمل اسم المفتاح الذي تم الضغط عليه
                SetWindowTextW(hStaticDisplay, keyName);
            }
            break;
        }

        // يتم استدعاء هذه الحالة عند رفع الإصبع عن المفتاح
        case WM_KEYUP: {
            // نعيد الشاشة إلى النص الافتراضي
            SetWindowTextW(hStaticDisplay, L"Press any key...");
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// نقطة بداية البرنامج
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW); // تغيير لون الخلفية للون النافذة الافتراضي
    wc.lpszClassName = L"KeyboardTesterClass";
    wc.hIcon         = LoadIcon(NULL, IDI_SHIELD); // تغيير أيقونة البرنامج

    if (!RegisterClassExW(&wc)) return 0;

    HWND hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"KeyboardTesterClass",
        L"Professional Keyboard Tester", // عنوان احترافي
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 320,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
