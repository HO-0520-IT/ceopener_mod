#include "cursorwindow.h"

#include <string>
#include <windows.h>
#include <knceutil.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
};

static LRESULT CALLBACK cursorWndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
static void onCreate(HWND hWnd, CREATESTRUCT *createStruct);
static void onDestroy(HWND hWnd);
static void onPaint(HWND hWnd, HDC hDC);
static void setLinePixels(HDC hDC, int x, int y, const TCHAR *lineCStr);
static void changeWindowLayout(HWND hWnd);
static void setLineRegion(HRGN hRgn, int x, int y, int len);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;

struct CursorWindowData {
};

void registerCursorWindowClass() {
    WNDCLASS windowClass;
    memset(&windowClass, 0, sizeof(WNDCLASS));
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = cursorWndProc;
    windowClass.hInstance = g_hInstance;
    windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    windowClass.lpszClassName = _T("OpenerPointerCursorWindow");
    RegisterClass(&windowClass);
}

void unregisterCursorWindowClass() {
    UnregisterClass(_T("OpenerPointerCursorWindow"), g_hInstance);
}

HWND createCursorWindow() {
    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST,
        _T("OpenerPointerCursorWindow"), _T("OpenerPointer Cursor Window"),
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, g_hInstance,
        NULL);

    //ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    return hWnd;
}

void destroyCursorWindow(HWND hWnd) {
    SendMessage(hWnd, WM_CLOSE, 0, 0);
}

void showCursorWindow(HWND hWnd) {
    changeWindowLayout(hWnd);

    ShowWindow(hWnd, SW_SHOW);

    SetTimer(hWnd, 1, 250, NULL);
}

void hideCursorWindow(HWND hWnd) {
    ShowWindow(hWnd, SW_HIDE);

    KillTimer(hWnd, 1);
}

void moveCursorWindow(HWND hWnd, int x, int y) {
    SetWindowPos(hWnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}

static LRESULT CALLBACK cursorWndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam) {

	switch (msg) {
    case WM_CREATE:
        onCreate(hWnd, (CREATESTRUCT *)lParam);
        return 0;
    case WM_DESTROY:
        onDestroy(hWnd);
		return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paintStruct;
        HDC hDC = BeginPaint(hWnd, &paintStruct);
        onPaint(hWnd, hDC);
        EndPaint(hWnd, &paintStruct);

        return 0;
    }
    case WM_TIMER:
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
            SWP_NOACTIVATE);
        return 0;
	}

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void onCreate(HWND hWnd, CREATESTRUCT *createStruct) {
    CursorWindowData *data = new CursorWindowData();
    SetWindowLong(hWnd, GWL_USERDATA, (long)data);

    changeWindowLayout(hWnd);
}

static void onDestroy(HWND hWnd) {
    CursorWindowData *data =
        (CursorWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    SendMessage(hWnd, WM_CLOSE, 0, 0);
}

static void onPaint(HWND hWnd, HDC hDC) {
    CursorWindowData *data =
        (CursorWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    int line = 0;
    setLinePixels(hDC, 0, line++, _T("*"));
    setLinePixels(hDC, 0, line++, _T("**"));
    setLinePixels(hDC, 0, line++, _T("* *"));
    setLinePixels(hDC, 0, line++, _T("*  *"));
    setLinePixels(hDC, 0, line++, _T("*   *"));
    setLinePixels(hDC, 0, line++, _T("*    *"));
    setLinePixels(hDC, 0, line++, _T("*     *"));
    setLinePixels(hDC, 0, line++, _T("*      *"));
    setLinePixels(hDC, 0, line++, _T("*       *"));
    setLinePixels(hDC, 0, line++, _T("*        *"));
    setLinePixels(hDC, 0, line++, _T("*         *"));
    setLinePixels(hDC, 0, line++, _T("*       **"));
    setLinePixels(hDC, 0, line++, _T("*     **"));
    setLinePixels(hDC, 0, line++, _T("*   **"));
    setLinePixels(hDC, 0, line++, _T("* **"));
    setLinePixels(hDC, 0, line++, _T("**"));
}

static void setLinePixels(HDC hDC, int x, int y, const TCHAR *lineCStr) {
    int i;

    for (i = 0; *lineCStr != _T('\0'); lineCStr++, i++) {
        if (*lineCStr == _T('*'))
            SetPixel(hDC, x + i, y, RGB(0, 0, 0));
    }
}

static void changeWindowLayout(HWND hWnd) {
    CursorWindowData *data =
        (CursorWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 16, 16, SWP_NOMOVE | SWP_NOACTIVATE);

    HRGN hRgn = CreateRectRgn(0, 0, 1, 1);

    int line = 0;
    setLineRegion(hRgn, 0, line++, 1);
    setLineRegion(hRgn, 0, line++, 2);
    setLineRegion(hRgn, 0, line++, 3);
    setLineRegion(hRgn, 0, line++, 4);
    setLineRegion(hRgn, 0, line++, 5);
    setLineRegion(hRgn, 0, line++, 6);
    setLineRegion(hRgn, 0, line++, 7);
    setLineRegion(hRgn, 0, line++, 8);
    setLineRegion(hRgn, 0, line++, 9);
    setLineRegion(hRgn, 0, line++, 10);
    setLineRegion(hRgn, 0, line++, 11);
    setLineRegion(hRgn, 0, line++, 10);
    setLineRegion(hRgn, 0, line++, 8);
    setLineRegion(hRgn, 0, line++, 6);
    setLineRegion(hRgn, 0, line++, 4);
    setLineRegion(hRgn, 0, line++, 2);

    SetWindowRgn(hWnd, hRgn, true);
}

static void setLineRegion(HRGN hRgn, int x, int y, int len) {
    HRGN hLineRgn = CreateRectRgn(x, y, x + len, y + 1);
    CombineRgn(hRgn, hRgn, hLineRgn, RGN_OR);
    DeleteObject(hLineRgn);
}
