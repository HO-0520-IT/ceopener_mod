#include <string>
#include <windows.h>
#include <knceutil.h>
#include <ceopener_plugin.h>
#include <DrawUtil.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

#ifdef OPENERCLOCK_EXPORTS
#define OPENERCLOCK_API __declspec(dllexport)
#else
#define OPENERCLOCK_API __declspec(dllimport)
#endif

static void onTimer(HWND hCont, int id);

HINSTANCE g_hInstance = NULL;
static HWND g_hTrayControl = NULL;
static WNDPROC g_prevTaskTrayProc = NULL;
static int g_prevHour = -1;
static int g_prevMinute = -1;


BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hInstance = (HINSTANCE)hInst;

    return true;
}

extern "C" OPENERCLOCK_API int pluginInit(HWND hCont) {
    PluginTrayItem trayItem = {0};
    trayItem.type = PLUGIN_TRAY_ITEM_TEXT;
    trayItem.width = 40;
    trayItem.height = 16;
	trayItem.uID = 100;

	g_hTrayControl = pluginRegisterTrayItem(hCont, g_hInstance, &trayItem);

    pluginRegisterTimer(hCont, 500, onTimer);

    return true;
}

extern "C" OPENERCLOCK_API void pluginTerminate(HWND hCont) {
}

static void onTimer(HWND hCont, int id) {
    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);

    if (sysTime.wHour != g_prevHour || sysTime.wMinute != g_prevMinute) {
        TCHAR timeCStr[16];
        _stprintf(timeCStr, _T("%d:%02d"), sysTime.wHour, sysTime.wMinute);
        SetWindowText(g_hTrayControl, timeCStr);

        InvalidateRect(g_hTrayControl, NULL, false);

        g_prevHour = sysTime.wHour;
        g_prevMinute = sysTime.wMinute;
    }
}
