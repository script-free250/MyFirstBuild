#include <windows.h>
#include <vector>
#include <string>

// بنية لتمثيل كل مفتاح على الشاشة
struct Key {
    int vkCode;          // الكود الافتراضي للمفتاح (e.g., 'A', VK_SHIFT)
    RECT rect;           // إحداثيات المفتاح على الشاشة
    std::wstring name;   // اسم المفتاح
    bool isPressed = false;
};

// قائمة المفاتيح التي سنرسمها (هذه عينة مبسطة)
std::vector<Key> keyboardLayout = {
    {VK_ESCAPE, {10, 10, 60, 40}, L"ESC"},
    {'1', {70, 10, 120, 40}, L"1"},
    {'2', {130, 10, 180, 40}, L"2"},
    {'3', {190, 10, 240, 40}, L"3"},
    {VK_TAB, {10, 50, 80, 80}, L"TAB"},
    {'Q', {90, 50, 140, 80}, L"Q"},
    {'W', {150, 50, 200, 80}, L"W"},
    {'E', {210, 50, 260, 80}, L"E"},
    {VK_CAPITAL, {10, 90, 100, 120}, L"CAPS"},
    {'A', {110, 90, 160, 120}, L"A"},
    {'S', {170, 90, 220, 120}, L"S"},
    {'D', {230, 90, 280, 120}, L"D"},
    {VK_LSHIFT, {10, 130, 120, 160}, L"SHIFT"},
    {'Z', {130, 130, 180, 160}, L"Z"},
    {'X', {190, 130, 240, 160}, L"X"},
    {'C', {250, 130, 300, 160}, L"C"},
    {VK_LCONTROL, {10, 170, 90, 200}, L"CTRL"},
    {VK_SPACE, {100, 170, 350, 200}, L"SPACE"}
};

// دالة لمعالجة أحداث النافذة
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // أهم جزء: التعامل مع الرسم
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // إعدادات الرسم (فرشاة ولون)
            HBRUSH hBrushNormal = CreateSolidBrush(RGB(50, 50, 50)); // لون المفتاح العادي (رمادي غامق)
            HBRUSH hBrushPressed = CreateSolidBrush(RGB(0, 150, 255)); // لون المفتاح عند الضغط (أزرق ساطع)
            SetBkMode(hdc, TRANSPARENT); // جعل خلفية النص شفافة
            SetTextColor(hdc, RGB(255, 255, 255)); // لون النص (أبيض)

            // المرور على كل مفتاح في التصميم ورسمه
            for (const auto& key : keyboardLayout) {
                FillRect(hdc, &key.rect, key.isPressed ? hBrushPressed : hBrushNormal);
                DrawTextW(hdc, key.name.c_str(), -1, (LPRECT)&key.rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            // حذف أدوات الرسم
            DeleteObject(hBrushNormal);
            DeleteObject(hBrushPressed);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_KEYDOWN: {
            // البحث عن المفتاح الذي تم الضغط عليه وتغيير حالته
            for (auto& key : keyboardLayout) {
                if (key.vkCode == wParam) {
                    key.isPressed = true;
                    InvalidateRect(hwnd, NULL, TRUE); // إجبار النافذة على إعادة رسم نفسها
                    break;
                }
            }
            break;
        }

        case WM_KEYUP: {
            // البحث عن المفتاح الذي تم رفع الإصبع عنه وإرجاع حالته
            for (auto& key : keyboardLayout) {
                if (key.vkCode == wParam) {
                    key.isPressed = false;
                    InvalidateRect(hwnd, NULL, TRUE); // إجبار النافذة على إعادة رسم نفسها
                    break;
                }
            }
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
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30)); // خلفية سوداء للبرنامج
    wc.lpszClassName = L"ProKeyboardTesterClass";
    wc.hIcon = LoadIcon(NULL, IDI_SHIELD);

    if (!RegisterClassExW(&wc)) return 0;

    HWND hwnd = CreateWindowExW(0, L"ProKeyboardTesterClass", L"Professional Keyboard Tester", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 250, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
