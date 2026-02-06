#include <windows.h>
#include <vector>
#include <string>
#include <map>

// -- الإضافات --
#define ID_TIMER_ANIMATION 1
#define RESET_BUTTON_ID 999
HWND hStaticLastKey, hStaticCounter;
int keyPressCount = 0;

// بنية احترافية لتمثيل المفاتيح
struct Key {
    int vkCode;
    RECT rect;
    std::wstring name;
    int animationAlpha = 0; // للتحكم في شفافية الأنيميشن
};

std::map<int, Key> keyboardLayout;

void InitializeKeyboard() {
    int x = 10, y = 100, w = 50, h = 50, s = 5; // أبعاد أساسية
    const int numKeys[] = {'1','2','3','4','5','6','7','8','9','0'};
    for(int i = 0; i < 10; ++i) { keyboardLayout[numKeys[i]] = {numKeys[i], {x + i*(w+s), y, x + i*(w+s) + w, y + h}, std::to_wstring(i+1)}; }
    y += h + s;
    const int qwertyKeys[] = {'Q','W','E','R','T','Y','U','I','O','P'};
    for(int i = 0; i < 10; ++i) { keyboardLayout[qwertyKeys[i]] = {qwertyKeys[i], {x + 20 + i*(w+s), y, x + 20 + i*(w+s) + w, y + h}, std::wstring(1, (wchar_t)qwertyKeys[i])}; }
    y += h + s;
    const int asdfKeys[] = {'A','S','D','F','G','H','J','K','L'};
    for(int i = 0; i < 9; ++i) { keyboardLayout[asdfKeys[i]] = {asdfKeys[i], {x + 40 + i*(w+s), y, x + 40 + i*(w+s) + w, y + h}, std::wstring(1, (wchar_t)asdfKeys[i])}; }
    y += h + s;
    const int zxcvKeys[] = {'Z','X','C','V','B','N','M'};
    for(int i = 0; i < 7; ++i) { keyboardLayout[zxcvKeys[i]] = {zxcvKeys[i], {x + 60 + i*(w+s), y, x + 60 + i*(w+s) + w, y + h}, std::wstring(1, (wchar_t)zxcvKeys[i])}; }
    keyboardLayout[VK_SPACE] = {VK_SPACE, {x + 180, y + h + s, x + 480, y + h + s + h}, L"SPACE"};
    keyboardLayout[VK_LSHIFT] = {VK_LSHIFT, {x, y, x + 110, y + h}, L"SHIFT"};
    keyboardLayout[VK_LCONTROL] = {VK_LCONTROL, {x, y + h + s, x + 80, y + h + s + h}, L"CTRL"};
    keyboardLayout[RESET_BUTTON_ID] = {RESET_BUTTON_ID, {620, 10, 780, 40}, L"Reset Counter"};
}

void UpdateCounters(HWND hwnd, std::wstring lastKey) {
    keyPressCount++;
    std::wstring counterText = L"Total Presses: " + std::to_wstring(keyPressCount);
    SetWindowTextW(hStaticCounter, counterText.c_str());
    SetWindowTextW(hStaticLastKey, (L"Last Key: " + lastKey).c_str());
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // --- هذا هو الجزء الذي تم إصلاحه ---
            LPCREATESTRUCTW lpcs = (LPCREATESTRUCTW)lParam;
            HINSTANCE hInstance = lpcs->hInstance;

            InitializeKeyboard();
            // تم تمرير كل المتطلبات الـ 11 الآن
            hStaticLastKey = CreateWindowW(L"STATIC", L"Last Key: None", WS_CHILD | WS_VISIBLE, 10, 10, 300, 40, hwnd, (HMENU)1, hInstance, NULL);
            hStaticCounter = CreateWindowW(L"STATIC", L"Total Presses: 0", WS_CHILD | WS_VISIBLE, 10, 50, 300, 40, hwnd, (HMENU)2, hInstance, NULL);
            
            HFONT hFont = CreateFont(24, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            SendMessage(hStaticLastKey, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hStaticCounter, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            SetTimer(hwnd, ID_TIMER_ANIMATION, 16, NULL);
            break;
        }
        case WM_TIMER: {
            bool needsRedraw = false;
            for (auto& pair : keyboardLayout) {
                if (pair.second.animationAlpha > 0) {
                    pair.second.animationAlpha -= 10;
                    if (pair.second.animationAlpha < 0) pair.second.animationAlpha = 0;
                    needsRedraw = true;
                }
            }
            if (needsRedraw) InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC memDC = CreateCompatibleDC(hdc);
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            SelectObject(memDC, memBitmap);
            HBRUSH hBackground = CreateSolidBrush(RGB(30, 30, 30));
            FillRect(memDC, &clientRect, hBackground);
            DeleteObject(hBackground);
            HFONT hKeyFont = CreateFont(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            SelectObject(memDC, hKeyFont);
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, RGB(255, 255, 255));
            for (const auto& pair : keyboardLayout) {
                const auto& key = pair.second;
                int glow = key.animationAlpha;
                HBRUSH hKeyBrush = CreateSolidBrush(RGB(50 + glow / 3, 50 + glow, 50 + glow));
                FillRect(memDC, &key.rect, hKeyBrush); // Use FillRect for solid background color
                // Draw rounded rectangle border on top
                HPEN hPen = CreatePen(PS_SOLID, 2, RGB(80 + glow/3, 80 + glow, 80+glow));
                SelectObject(memDC, hPen);
                SelectObject(memDC, GetStockObject(NULL_BRUSH)); // No fill for RoundRect
                RoundRect(memDC, key.rect.left, key.rect.top, key.rect.right, key.rect.bottom, 8, 8);
                DeleteObject(hPen);
                
                DrawTextW(memDC, key.name.c_str(), -1, (LPRECT)&key.rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                DeleteObject(hKeyBrush);
            }
            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);
            DeleteObject(hKeyFont);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_LBUTTONDOWN: {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            for (auto& pair : keyboardLayout) {
                if (PtInRect(&pair.second.rect, pt)) {
                    if (pair.first == RESET_BUTTON_ID) {
                        keyPressCount = -1;
                        UpdateCounters(hwnd, L"RESET");
                    } else {
                        pair.second.animationAlpha = 255;
                        UpdateCounters(hwnd, pair.second.name);
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                }
            }
            break;
        }
        case WM_KEYDOWN: {
            if (keyboardLayout.count(wParam)) {
                keyboardLayout[wParam].animationAlpha = 255;
                UpdateCounters(hwnd, keyboardLayout[wParam].name);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER_ANIMATION);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"UltimateKeyboardTester";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassExW(&wc)) return 0;
    HWND hwnd = CreateWindowExW(0, L"UltimateKeyboardTester", L"Ultimate Keyboard Tester", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    MSG Msg;
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
