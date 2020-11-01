#include "kncecomm.h"

#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    MESSAGE_MARGIN = 25
};

static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
static void onCreate(HWND hWnd, CREATESTRUCT *createStruct);
static void onDestroy(HWND hWnd);
static void onPaint(HWND hWnd, HDC hDC);
static void changeWindowLayout(HWND hWnd);

extern HINSTANCE g_hInstance;

struct WaitBoxData {
    tstring message;
};

void registerWaitBoxClass() {
    WNDCLASS windowClass;
    memset(&windowClass, 0, sizeof(WNDCLASS));
    windowClass.lpfnWndProc = wndProc;
    windowClass.hInstance = g_hInstance;
    windowClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = _T("KNCommonLibWaitBox");
    RegisterClass(&windowClass);
}

void unregisterWaitBoxClass() {
    UnregisterClass(_T("KNCommonLibWaitBox"), g_hInstance);
}

HWND createWaitBox(HWND hOwnerWindow, const tstring &msg) {
    HWND hWnd = CreateWindowEx(
        0,
        _T("KNCommonLibWaitBox"), _T(""),
        WS_POPUP | WS_BORDER,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        hOwnerWindow, NULL, g_hInstance, (void *)&msg);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    return hWnd;
}

void destroyWaitBox(HWND hWnd) {
    SendMessage(hWnd, WM_CLOSE, 0, 0);
}

static LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam) {

	switch (msg) {
    case WM_CREATE:
        onCreate(hWnd, (CREATESTRUCT *)lParam);
        return 0;
    case WM_DESTROY:
        onDestroy(hWnd);
		return 0;
    case WM_COMMAND:
    {
        int id    = LOWORD(wParam); 
        int event = HIWORD(wParam);

        //switch (id) {
        //}

        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT paintStruct;
        HDC hDC = BeginPaint(hWnd, &paintStruct);
        onPaint(hWnd, hDC);
        EndPaint(hWnd, &paintStruct);

        return 0;
    }
	}

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void onCreate(HWND hWnd, CREATESTRUCT *createStruct) {
    WaitBoxData *data = new WaitBoxData();
    SetWindowLong(hWnd, GWL_USERDATA, (long)data);

    data->message = *(const tstring *)createStruct->lpCreateParams;

    if (data->message.empty())
        data->message = _T("Please wait...");

    changeWindowLayout(hWnd);
}

static void onDestroy(HWND hWnd) {
    WaitBoxData *data = (WaitBoxData *)GetWindowLong(hWnd, GWL_USERDATA);

    delete data;
}

static void onPaint(HWND hWnd, HDC hDC) {
    WaitBoxData *data = (WaitBoxData *)GetWindowLong(hWnd, GWL_USERDATA);

    int prevBkMode = SetBkMode(hDC, TRANSPARENT);

    ExtTextOut(hDC, MESSAGE_MARGIN, MESSAGE_MARGIN, 0, NULL,
        data->message.c_str(), data->message.size(), NULL);

    SetBkMode(hDC, prevBkMode);
}

static void changeWindowLayout(HWND hWnd) {
    WaitBoxData *data = (WaitBoxData *)GetWindowLong(hWnd, GWL_USERDATA);

    HDC hDC = GetDC(hWnd);

    SIZE textSize;
    GetTextExtentExPointW(hDC, data->message.c_str(), data->message.size(), 0,
        NULL, NULL, &textSize);

    ReleaseDC(hWnd, hDC);

    int windowWidth = textSize.cx + MESSAGE_MARGIN * 2 + 2;
    int windowHeight = textSize.cy + MESSAGE_MARGIN * 2 + 2;

    RECT desktopRect;
    HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktopRect); 

    MoveWindow(hWnd, (desktopRect.right - windowWidth) / 2,
        (desktopRect.bottom - windowHeight) / 2,  windowWidth, windowHeight,
        false);
}
