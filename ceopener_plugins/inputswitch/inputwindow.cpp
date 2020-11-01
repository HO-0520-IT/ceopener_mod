#include "inputwindow.h"

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
    INPUT_AREA_MARGIN = 2,
    INPUT_AREA_MAX_UNITS = 15,
};

static LRESULT CALLBACK inputWndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
static void onCreate(HWND hWnd, CREATESTRUCT *createStruct);
static void onDestroy(HWND hWnd);
static void onPaint(HWND hWnd, HDC hDC);
static void updateCharacterSize(HWND hWnd);
static void changeWindowLayout(HWND hWnd);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;

struct InputWindowData {
    CandidateController *candidateController;
    EditController *editController;
    HFONT hFont;
    int characterWidth;
    int characterHeight;
    int controlType;
};

InputWindowParams::InputWindowParams() {
    m_candidateController = NULL;
    m_editController = NULL;
}

InputWindowParams::~InputWindowParams() {
}

void registerInputWindowClass() {
    WNDCLASS windowClass;
    memset(&windowClass, 0, sizeof(WNDCLASS));
    windowClass.lpfnWndProc = inputWndProc;
    windowClass.hInstance = g_hInstance;
    windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    windowClass.lpszClassName = _T("InputSwitchInputWindow");
    RegisterClass(&windowClass);
}

void unregisterInputWindowClass() {
    UnregisterClass(_T("InputSwitchInputWindow"), g_hInstance);
}

HWND createInputWindow(InputWindowParams &params) {
    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST,
        _T("InputSwitchInputWindow"), _T("InputSwitch Input Window"),
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, g_hInstance,
        (void *)&params);

    //ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    return hWnd;
}

void destroyInputWindow(HWND hWnd) {
    SendMessage(hWnd, WM_CLOSE, 0, 0);
}

void showInputWindow(HWND hWnd) {
    changeWindowLayout(hWnd);

    ShowWindow(hWnd, SW_SHOWNA);
}

void hideInputWindow(HWND hWnd) {
    ShowWindow(hWnd, SW_HIDE);
}

int getInputWindowFontSize(HWND hWnd) {
    InputWindowData *data =
        (InputWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    tstring fontName;
    int pointSize = 0;
    bool isBold = false;
    bool isItalic = false;

    KnceUtil::getFontAttributes(data->hFont, fontName, pointSize, isBold,
        isItalic);

    return pointSize;
}

void setInputWindowFontSize(HWND hWnd, int size) {
    InputWindowData *data =
        (InputWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    tstring fontName;
    int pointSize = 0;
    bool isBold = false;
    bool isItalic = false;

    KnceUtil::getFontAttributes(data->hFont, fontName, pointSize, isBold,
        isItalic);

    DeleteObject(data->hFont);

    data->hFont = KnceUtil::createFont(fontName, size, isBold, isItalic);

    data->candidateController->setFont(data->hFont);
    data->editController->setFont(data->hFont);

    updateCharacterSize(hWnd);
}

int getInputWindowControlType(HWND hWnd) {
    InputWindowData *data =
        (InputWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    return data->controlType;
}

void setInputWindowControlType(HWND hWnd, int type) {
    InputWindowData *data =
        (InputWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    data->controlType = type;
}

static LRESULT CALLBACK inputWndProc(HWND hWnd, UINT msg, WPARAM wParam,
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
	}

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void onCreate(HWND hWnd, CREATESTRUCT *createStruct) {
    InputWindowData *data = new InputWindowData();
    SetWindowLong(hWnd, GWL_USERDATA, (long)data);

    InputWindowParams *createParams =
        (InputWindowParams *)createStruct->lpCreateParams;

    data->candidateController = createParams->getCandidateController();
    data->editController = createParams->getEditController();
    data->controlType = 0;

    data->hFont = KnceUtil::createFont(_T(""), 100, false, false,
        KNCE_FONT_PITCH_FIXED);

    data->candidateController->setFont(data->hFont);
    data->editController->setFont(data->hFont);

    updateCharacterSize(hWnd);

    changeWindowLayout(hWnd);
}

static void onDestroy(HWND hWnd) {
    InputWindowData *data =
        (InputWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    DeleteObject(data->hFont);

    SendMessage(hWnd, WM_CLOSE, 0, 0);
}

static void onPaint(HWND hWnd, HDC hDC) {
    InputWindowData *data =
        (InputWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->controlType == INPUT_WINDOW_CONTROL_CANDIDATE)
        data->candidateController->draw(hDC);
    else if (data->controlType == INPUT_WINDOW_CONTROL_EDIT)
        data->editController->draw(hDC);

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);

    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(77, 109, 243));
    HPEN hPrevPen = (HPEN)SelectObject(hDC, hPen);

    MoveToEx(hDC, clientRect.right - 1, 0, NULL);
    LineTo(hDC, 0, 0);
    LineTo(hDC, 0, clientRect.bottom - 1);
    LineTo(hDC, clientRect.right - 1, clientRect.bottom - 1);
    LineTo(hDC, clientRect.right - 1, 0);

    SelectObject(hDC, hPrevPen);
    DeleteObject(hPen);
}

static void updateCharacterSize(HWND hWnd) {
    InputWindowData *data =
        (InputWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    HDC hDC = GetDC(hWnd);

    HFONT hPrevFont = (HFONT)SelectObject(hDC, data->hFont);

    SIZE textSize;
    GetTextExtentExPointW(hDC, _T("‚ "), 1, 0, NULL, NULL, &textSize);

    data->characterWidth = textSize.cx;
    data->characterHeight = textSize.cy;

    SelectObject(hDC, hPrevFont);

    ReleaseDC(hWnd, hDC);

    data->candidateController->setCharacterSize(data->characterWidth,
        data->characterHeight);
    data->editController->setCharacterSize(data->characterWidth,
        data->characterHeight);
}

static void changeWindowLayout(HWND hWnd) {
    InputWindowData *data =
        (InputWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

    int windowWidth = data->characterWidth * INPUT_AREA_MAX_UNITS +
        INPUT_AREA_MARGIN * 2;
    int windowHeight = data->characterHeight + INPUT_AREA_MARGIN * 2;

    RECT workAreaRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);

    MoveWindow(hWnd, workAreaRect.right - windowWidth,
        workAreaRect.bottom - windowHeight, windowWidth, windowHeight, false);
}
