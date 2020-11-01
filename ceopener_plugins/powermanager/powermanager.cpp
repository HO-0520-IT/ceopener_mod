#include <string>
#include <windows.h>
#include <knceutil.h>
#include <brainapi.h>
#include <ceopener_plugin.h>

#include "ctrldlg.h"

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

#ifdef POWERMANAGER_EXPORTS
#define POWERMANAGER_API __declspec(dllexport)
#else
#define POWERMANAGER_API __declspec(dllimport)
#endif

static void onPluginMenuItemSelected(HWND hCont, HWND hMenu, int id);
static void onPluginTrayIconItemClicked(HWND hCont, int id);
static void onPowerOnOff(HWND hCont, unsigned long params);
static void showControlDialog(HWND hWnd);
static int readBrainBrightness();
static void update();
static void updateBrightness(int bright);
static void updateClockGear(int gear);

HINSTANCE g_hInstance = NULL;
static HWND g_hTrayControl = NULL;
static WNDPROC g_prevTaskTrayProc = NULL;
static int g_brightness = 3;
static int g_clockGear = 0;
static bool g_isBacklightDimmingEnabled = true;
static bool g_isPowerDownEnabled = true;
static bool g_isMrSensorEnabled = true;
static bool g_timerRunning = false;

LRESULT CALLBACK taskTrayProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam) {

	switch (msg) {
    case WM_TIMER:
    {
        EdUpdateBacklightState();

        return 0;
    }
    }

    return CallWindowProc(g_prevTaskTrayProc, hWnd, msg, wParam, lParam);
}

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hInstance = (HINSTANCE)hInst;

    return true;
}

extern "C" POWERMANAGER_API int pluginInit(HWND hCont) {
    if (!loadBrainApis())
        return false;

    PluginTrayIconItem trayItem = {0};
    trayItem.hIcon = (HICON)LoadImage(g_hInstance, _T("TRAY"), IMAGE_ICON,
        0, 0, 0);
    trayItem.onClicked = onPluginTrayIconItemClicked;
    trayItem.uID = 100;

    int trayId = pluginRegisterTrayIconItem(hCont, g_hInstance, &trayItem);
    g_hTrayControl = pluginGetTrayIconControl(hCont, trayId);

    g_prevTaskTrayProc = (WNDPROC)SetWindowLong(g_hTrayControl, GWL_WNDPROC,
        (LONG)taskTrayProc);

    HWND hPluginMenu = pluginGetPluginMenu(hCont);

    PluginMenuItem menuItem = {0};
    _tcscpy(menuItem.caption, _T("電源オプション..."));
    menuItem.onItemSelected = onPluginMenuItemSelected;

    pluginAppendMenuItem(hCont, hPluginMenu, &menuItem);

    pluginRegisterCallback(hCont, PLUGIN_CALLBACK_POWER_ON_OFF,
        onPowerOnOff);

    g_brightness = readBrainBrightness();

    return true;
}

extern "C" POWERMANAGER_API void pluginTerminate(HWND hCont) {
    if (g_timerRunning)
        KillTimer(g_hTrayControl, 1);

    updateBrightness(readBrainBrightness());

    if (g_clockGear > 0)
        updateClockGear(0);

    EdSetDisablePowerOff(false);
    EdMrSensorEnable();

    pluginUnregisterCallback(hCont, PLUGIN_CALLBACK_POWER_ON_OFF,
        onPowerOnOff);

    freeBrainApis();
}

static void onPluginMenuItemSelected(HWND hCont, HWND hMenu, int id) {
    showControlDialog(hCont);
}

static void onPluginTrayIconItemClicked(HWND hCont, int id) {
    showControlDialog(hCont);
}

static void onPowerOnOff(HWND hCont, unsigned long params) {
    if (params == PLUGIN_POWER_ON)
        update();
}

static void showControlDialog(HWND hWnd) {
    ControlDialogParams params;
    params.setBrightness(g_brightness);
    params.setClockGear(g_clockGear);
    params.setBacklightDimmingEnabled(g_isBacklightDimmingEnabled);
    params.setPowerDownEnabled(g_isPowerDownEnabled);
    params.setMrSensorEnabled(g_isMrSensorEnabled);

    if (g_clockGear > 0)
        updateClockGear(0);

    if (!showControlDialog(hWnd, params)) {
        if (g_clockGear > 0)
            updateClockGear(g_clockGear);
        return;
    }

    g_brightness = params.getBrightness();
    g_clockGear = params.getClockGear();
    g_isBacklightDimmingEnabled = params.isBacklightDimmingEnabled();
    g_isPowerDownEnabled = params.isPowerDownEnabled();
    g_isMrSensorEnabled = params.isMrSensorEnabled();

    update();
}

static int readBrainBrightness() {
    tstring boxFileName = _T("\\Nand2\\SYSTEM.BOX");
    HANDLE hBoxFile = CreateFile(boxFileName.c_str(), GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hBoxFile == INVALID_HANDLE_VALUE)
        return 3;

    DWORD numRead = 0;

    SetFilePointer(hBoxFile, 0x0034, NULL, FILE_BEGIN);

    char bright = 0;
    ReadFile(hBoxFile, &bright, 1, &numRead, NULL);

    CloseHandle(hBoxFile);

    return bright;
}

static void update() {
    updateBrightness(g_brightness);

    if (g_clockGear > 0)
        updateClockGear(g_clockGear);

    if (g_isBacklightDimmingEnabled) {
        if (g_timerRunning) {
            KillTimer(g_hTrayControl, 1);
            g_timerRunning = false;
        }
    }
    else {
        EdUpdateBacklightState();

        if (!g_timerRunning) {
            SetTimer(g_hTrayControl, 1, 15 * 1000, NULL);
            g_timerRunning = true;
        }
    }

    if (g_isPowerDownEnabled)
        EdSetDisablePowerOff(false);
    else
        EdSetDisablePowerOff(true);

    if (g_isMrSensorEnabled)
        EdMrSensorEnable();
    else
        EdMrSensorDisable();
}

static void updateBrightness(int bright) {
    int edBright = 0;
    switch (bright) {
    case 1:
        edBright = 1;
        break;
    case 2:
        edBright = 25;
        break;
    case 3:
        edBright = 50;
        break;
    case 4:
        edBright = 75;
        break;
    case 5:
        edBright = 100;
        break;
    }

    EdSetBacklightBright(edBright);
}

static void updateClockGear(int gear) {
    void *virt = EdMmMapIoSpace(0xf0050000 + 0x0004, 4, true);
    if (virt == NULL)
        return;

    (*(unsigned char *)virt) = gear & 0x03;

    EdMmUnmapIoSpace((void *)virt, 4);
}
