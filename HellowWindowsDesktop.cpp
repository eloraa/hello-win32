#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <random>
#include <commctrl.h>
#include "resource.h"
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

static TCHAR szClass[] = _T("MyApp");
static TCHAR szTitle[] = _T("Hello World!");
HINSTANCE hInst;

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

#define DWMWCP_DEFAULT 0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#define DWMWCP_ROUNDSMALL 3

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

COLORREF GetRandomColor()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    return RGB(dis(gen), dis(gen), dis(gen));
}

BOOL IsDarkMode()
{
    HKEY hKey;
    DWORD value = 1;
    DWORD size = sizeof(value);
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hKey, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return (value == 0);
        }
        RegCloseKey(hKey);
    }

    COLORREF windowColor = GetSysColor(COLOR_WINDOW);
    int brightness = GetRValue(windowColor) + GetGValue(windowColor) + GetBValue(windowColor);

    return brightness < 384;
}

void ApplyWindowDecor(HWND hWnd, COLORREF borderColor = RGB(139, 195, 74))
{
    DWORD pref = DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));

    COLORREF bg = IsDarkMode()
                      ? RGB(31, 31, 31)
                      : RGB(255, 255, 255);
    COLORREF txt = IsDarkMode()
                       ? RGB(255, 255, 255)
                       : RGB(0, 0, 0);

    DwmSetWindowAttribute(hWnd, DWMWA_CAPTION_COLOR, &bg, sizeof(bg));
    DwmSetWindowAttribute(hWnd, DWMWA_TEXT_COLOR, &txt, sizeof(txt));

    DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}

HICON LoadIconFromResource(int resourceId, int size = 32)
{
    HICON hIcon = (HICON)LoadImage(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(resourceId),
        IMAGE_ICON,
        size, size,
        LR_DEFAULTCOLOR);

    return hIcon;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;
    WNDCLASSEX wc = {sizeof(wc)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = szClass;

    HICON hIcon = LoadIconFromResource(IDI_ICON1, 32);
    HICON hIconSmall = LoadIconFromResource(IDI_ICON1, 16);

    if (hIcon)
    {
        wc.hIcon = hIcon;
        wc.hIconSm = hIconSmall ? hIconSmall : hIcon;
    }
    else
    {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    }

    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(
        0,
        szClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 200,
        NULL, NULL, hInst, NULL);

    if (!hWnd)
        return 1;

    ApplyWindowDecor(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);

        COLORREF bg = IsDarkMode()
                          ? RGB(31, 31, 31)
                          : RGB(255, 255, 255);
        HBRUSH hBr = CreateSolidBrush(bg);
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);

        COLORREF txt = IsDarkMode()
                           ? RGB(255, 255, 255)
                           : RGB(0, 0, 0);
        SetTextColor(hdc, txt);
        SetBkMode(hdc, TRANSPARENT);

        LPCTSTR msgText = _T("Hello, win32 desktop!");
        DrawText(hdc, msgText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        ApplyWindowDecor(hWnd, GetRandomColor());
        break;

    case WM_SETFOCUS:
        ApplyWindowDecor(hWnd, GetRandomColor());
        break;

    case WM_KILLFOCUS:
    {
        COLORREF transparentColor = CLR_NONE;
        DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &transparentColor, sizeof(COLORREF));
        break;
    }

    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        ApplyWindowDecor(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
