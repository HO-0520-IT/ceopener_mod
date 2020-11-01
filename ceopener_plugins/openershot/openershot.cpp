#include <string>
#include <windows.h>
#include <knceutil.h>
#include <kncedlg.h>
#include <ceopener_plugin.h>

#include "settingdlg.h"

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

#ifdef OPENERSHOT_EXPORTS
#define OPENERSHOT_API __declspec(dllexport)
#else
#define OPENERSHOT_API __declspec(dllimport)
#endif

static void onPluginMenuItemSelected(HWND hCont, HWND hMenu, int id);
static int onBindedKeyPress(HWND hCont, int id);
static void showSettings(HWND hCont);
static bool captureScreen(HWND hCont, const tstring &fileName);

HINSTANCE g_hInstance = NULL;
static int g_captureHotKeyId = 0;
static tstring g_storeDirectory;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hInstance = (HINSTANCE)hInst;

    return true;
}

extern "C" OPENERSHOT_API int pluginInit(HWND hCont) {
    HWND hPluginMenu = pluginGetPluginMenu(hCont);

    PluginMenuItem menuItem = {0};
    _tcscpy(menuItem.caption, _T("OpenerShotの設定..."));
    menuItem.onItemSelected = onPluginMenuItemSelected;

    int menuItemId = pluginAppendMenuItem(hCont, hPluginMenu, &menuItem);

    PluginKeyBinding bind = {0};
    _tcscpy(bind.category, _T("OpenerShot"));
    _tcscpy(bind.name, _T("画面のキャプチャ"));
    bind.isEnabled = false;
    bind.defaultKeyCode = HARD_KEY_SWITCH;
    bind.onKeyPress = onBindedKeyPress;
    bind.isActive = true;

    g_captureHotKeyId = pluginRegisterKeyBinding(hCont, &bind);

    tstring persistDir = _T("\\Nand2\\.ceopener");
    tstring propFileName = persistDir + _T("\\openershot.dat");

    if (GetFileAttributes(propFileName.c_str()) == -1)
        g_storeDirectory = _T("\\Storage Card\\screens");
    else {
        map<tstring, tstring> props;
        KnceUtil::readPropertyFile(props, propFileName);

        g_storeDirectory = props[_T("storeDirectory")];
    }

    return true;
}

extern "C" OPENERSHOT_API void pluginTerminate(HWND hCont) {
    pluginUnregisterKeyBinding(hCont, g_captureHotKeyId);

    map<tstring, tstring> props;

    props[_T("storeDirectory")] = g_storeDirectory;

    tstring persistDir = _T("\\Nand2\\.ceopener");
    if (GetFileAttributes(persistDir.c_str()) == -1)
        CreateDirectory(persistDir.c_str(), NULL);

    tstring propFileName = persistDir + _T("\\openershot.dat");
    KnceUtil::writePropertyFile(propFileName, props);
}

static void onPluginMenuItemSelected(HWND hCont, HWND hMenu, int id) {
    showSettings(hCont);
}

static int onBindedKeyPress(HWND hCont, int id) {
    SYSTEMTIME time;
    GetLocalTime(&time);

    TCHAR fileNameCStr[32];
    _sntprintf(fileNameCStr, 32, _T("%04d%02d%02d%02d%02d%02d.bmp"),
        time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
        time.wSecond);

    bool result = captureScreen(hCont, g_storeDirectory + _T("\\") +
        fileNameCStr);

    if (result) {
        MessageBox(hCont, _T("画面をキャプチャしました。"), _T("情報"),
            MB_ICONINFORMATION | MB_SETFOREGROUND);
    }
    else {
        MessageBox(hCont, _T("画面のキャプチャに失敗しました。"), _T("エラー"),
            MB_ICONEXCLAMATION | MB_SETFOREGROUND);
    }

    return true;
}

static void showSettings(HWND hCont) {
    SettingParams params;
    params.setStoreDirectory(g_storeDirectory);

    if (!showSettingDialog(hCont, params))
        return;

    g_storeDirectory = params.getStoreDirectory();
}

static bool captureScreen(HWND hWnd, const tstring &fileName) {
	HANDLE hBmpFile = CreateFile(fileName.c_str(), GENERIC_WRITE, 0,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBmpFile == INVALID_HANDLE_VALUE)
		return false;

    RECT desktopRect;
    HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktopRect); 

    int width = desktopRect.right;
    int height = desktopRect.bottom;

    BITMAPINFO bmpInfo = {0};

    BITMAPINFOHEADER &bmpInfoHeader = bmpInfo.bmiHeader;
    bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfoHeader.biWidth = width;
    bmpInfoHeader.biHeight = height;
    bmpInfoHeader.biPlanes = 1;
    bmpInfoHeader.biBitCount = 24;
    bmpInfoHeader.biCompression = BI_RGB;
    bmpInfoHeader.biSizeImage =
        ((width * bmpInfoHeader.biBitCount + 31) & (~31)) / 8 * height; 

    HDC hDC = GetDC(NULL);

    void *pixelBuf = NULL;
    HBITMAP hBmp = CreateDIBSection(hDC, &bmpInfo, DIB_RGB_COLORS,
        (void **)&pixelBuf, NULL, 0);
	 
	HDC hOffscrDC = CreateCompatibleDC(hDC);
	HANDLE hPrevBmp = SelectObject(hOffscrDC, hBmp);

    BitBlt(hOffscrDC, 0, 0, width, height, hDC, 0, 0, SRCCOPY);

    HWND hWaitBox = knceCreateWaitBox(hWnd, _T("画面をキャプチャしています..."));

	BITMAPFILEHEADER bmpFileHeader;
	bmpFileHeader.bfType = 0x4d42;
	bmpFileHeader.bfReserved1 = bmpFileHeader.bfReserved2 = 0;

	DWORD written;
	WriteFile(hBmpFile, &bmpFileHeader, sizeof(BITMAPFILEHEADER), &written,
        NULL);

	WriteFile(hBmpFile, &bmpInfoHeader, sizeof(BITMAPINFOHEADER), &written,
        NULL);

	bmpFileHeader.bfOffBits = SetFilePointer(hBmpFile, 0, 0, FILE_CURRENT);

	WriteFile(hBmpFile, pixelBuf, bmpInfoHeader.biSizeImage, &written, NULL);

	bmpFileHeader.bfSize = SetFilePointer(hBmpFile, 0, 0, FILE_CURRENT);

	SetFilePointer(hBmpFile, 0, 0, FILE_BEGIN);
	WriteFile(hBmpFile, &bmpFileHeader, sizeof(BITMAPFILEHEADER), &written,
        NULL);

    knceDestroyWaitBox(hWaitBox);

    SelectObject(hOffscrDC, hPrevBmp);

	DeleteObject(hBmp);
    DeleteObject(hOffscrDC);
    ReleaseDC(NULL, hDC);

    CloseHandle(hBmpFile);

	return true;
}
