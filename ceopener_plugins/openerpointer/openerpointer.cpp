#include <string>
#include <map>
#include <windows.h>
#include <knceutil.h>
#include <ceopener_plugin.h>
#include "cursorwindow.h"

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

#ifdef OPENERPOINTER_EXPORTS
#define OPENERPOINTER_API __declspec(dllexport)
#else
#define OPENERPOINTER_API __declspec(dllimport)
#endif

enum {
    POINTER_SPEED = 10,
    BINDED_ID_SWITCH = 0,
    BINDED_ID_CLOSE = 1,
    BINDED_ID_LEFT = 2,
    BINDED_ID_RIGHT = 3,
    BINDED_ID_UP = 4,
    BINDED_ID_DOWN = 5,
    BINDED_ID_CLICK = 6,
    BINDED_ID_TOGGLE = 7,
};

static int onBindedKeyPress(HWND hCont, int id);
static void loadConfig();
static int registerKeyBinding(HWND hCont, const tstring &name, bool fn,
    unsigned int keyCode, bool active);

HINSTANCE g_hInstance = NULL;
static HWND g_hCursorWindow = NULL;
static int g_trayId = 0;
static HICON g_pointerOffIcon = NULL;
static HICON g_pointerOnIcon = NULL;
static bool g_isPointingEnabled = false;
static map<int, int> g_bindedKeyCodeTable;
static map<int, int> g_bindedIdTable;
static int g_pointerX = 0;
static int g_pointerY = 0;
static bool g_isPressed = false;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hInstance = (HINSTANCE)hInst;

    return true;
}

extern "C" OPENERPOINTER_API int pluginInit(HWND hCont) {
    registerCursorWindowClass();

    g_pointerOffIcon = (HICON)LoadImage(g_hInstance, _T("POINTER_OFF"),
        IMAGE_ICON, 0, 0, 0);
    g_pointerOnIcon = (HICON)LoadImage(g_hInstance, _T("POINTER_ON"),
        IMAGE_ICON, 0, 0, 0);

    PluginTrayIconItem trayItem = {0};
    trayItem.hIcon = g_pointerOffIcon;
    //trayItem.onClicked = onPluginTrayIconItemClicked;
    trayItem.uID = 100;

    g_trayId = pluginRegisterTrayIconItem(hCont, g_hInstance, &trayItem);

    g_hCursorWindow = createCursorWindow();

    loadConfig();

    g_bindedIdTable[BINDED_ID_SWITCH] = registerKeyBinding(hCont,
        _T("切り替え"), true, g_bindedKeyCodeTable[BINDED_ID_SWITCH], true);

    g_bindedIdTable[BINDED_ID_LEFT] = registerKeyBinding(hCont,
        _T(""), false, HARD_KEY_LEFT, false);
    g_bindedIdTable[BINDED_ID_LEFT | 0x8000] = registerKeyBinding(hCont,
        _T(""), true, HARD_KEY_LEFT, false);

    g_bindedIdTable[BINDED_ID_RIGHT] = registerKeyBinding(hCont,
        _T(""), false, HARD_KEY_RIGHT, false);
    g_bindedIdTable[BINDED_ID_RIGHT | 0x8000] = registerKeyBinding(hCont,
        _T(""), true, HARD_KEY_RIGHT, false);

    g_bindedIdTable[BINDED_ID_DOWN] = registerKeyBinding(hCont,
        _T(""), false, HARD_KEY_DOWN, false);
    g_bindedIdTable[BINDED_ID_DOWN | 0x8000] = registerKeyBinding(hCont,
        _T(""), true, HARD_KEY_DOWN, false);

    g_bindedIdTable[BINDED_ID_UP] = registerKeyBinding(hCont,
        _T(""), false, HARD_KEY_UP, false);
    g_bindedIdTable[BINDED_ID_UP | 0x8000] = registerKeyBinding(hCont,
        _T(""), true, HARD_KEY_UP, false);

    g_bindedIdTable[BINDED_ID_CLICK] = registerKeyBinding(hCont,
        _T("クリック"), false, g_bindedKeyCodeTable[BINDED_ID_CLICK], false);

    g_bindedIdTable[BINDED_ID_TOGGLE] = registerKeyBinding(hCont,
        _T("トグル"), false, g_bindedKeyCodeTable[BINDED_ID_TOGGLE], false);

    RECT desktopRect;
    HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktopRect);

    g_pointerX = desktopRect.right / 2;
    g_pointerY = desktopRect.bottom / 2;

    return true;
}

extern "C" OPENERPOINTER_API void pluginTerminate(HWND hCont) {
    KnceUtil::restoreKeyRepeatSpeed();

    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_SWITCH]);

    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_LEFT]);
    pluginUnregisterKeyBinding(hCont,
        g_bindedIdTable[BINDED_ID_LEFT | 0x8000]);
    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_RIGHT]);
    pluginUnregisterKeyBinding(hCont,
        g_bindedIdTable[BINDED_ID_RIGHT | 0x8000]);
    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_DOWN]);
    pluginUnregisterKeyBinding(hCont,
        g_bindedIdTable[BINDED_ID_DOWN | 0x8000]);
    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_UP]);
    pluginUnregisterKeyBinding(hCont,
        g_bindedIdTable[BINDED_ID_UP | 0x8000]);

    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_CLICK]);
    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_CLICK]);
    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_TOGGLE]);
    pluginUnregisterKeyBinding(hCont, g_bindedIdTable[BINDED_ID_TOGGLE]);

    destroyCursorWindow(g_hCursorWindow);

    unregisterCursorWindowClass();
}

static int onBindedKeyPress(HWND hCont, int id) {
    if (id == g_bindedIdTable[BINDED_ID_SWITCH]) {
        g_isPointingEnabled = !g_isPointingEnabled;
        if (g_isPointingEnabled) {
            pluginActivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_LEFT]);
            pluginActivateKeyBinding(hCont,
                g_bindedIdTable[BINDED_ID_LEFT | 0x8000]);
            pluginActivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_RIGHT]);
            pluginActivateKeyBinding(hCont,
                g_bindedIdTable[BINDED_ID_RIGHT | 0x8000]);
            pluginActivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_DOWN]);
            pluginActivateKeyBinding(hCont,
                g_bindedIdTable[BINDED_ID_DOWN | 0x8000]);
            pluginActivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_UP]);
            pluginActivateKeyBinding(hCont,
                g_bindedIdTable[BINDED_ID_UP | 0x8000]);

            pluginActivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_CLICK]);
            pluginActivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_TOGGLE]);

            pluginSetTrayIcon(hCont, g_trayId, g_pointerOnIcon);

            RECT desktopRect;
            HWND hDesktop = GetDesktopWindow();
            GetWindowRect(hDesktop, &desktopRect); 

            mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
                65536 * g_pointerX / desktopRect.right,
                65535 * g_pointerY / desktopRect.bottom, 0, NULL);

            moveCursorWindow(g_hCursorWindow, g_pointerX, g_pointerY);
            showCursorWindow(g_hCursorWindow);

            KnceUtil::changeKeyRepeatSpeed(500, 10);

            g_isPressed = false;
        }
        else {
            pluginInactivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_LEFT]);
            pluginInactivateKeyBinding(hCont,
                g_bindedIdTable[BINDED_ID_LEFT | 0x8000]);
            pluginInactivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_RIGHT]);
            pluginInactivateKeyBinding(hCont,
                g_bindedIdTable[BINDED_ID_RIGHT | 0x8000]);
            pluginInactivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_DOWN]);
            pluginInactivateKeyBinding(hCont,
                g_bindedIdTable[BINDED_ID_DOWN | 0x8000]);
            pluginInactivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_UP]);
            pluginInactivateKeyBinding(hCont,
                g_bindedIdTable[BINDED_ID_UP | 0x8000]);

            pluginInactivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_CLICK]);
            pluginInactivateKeyBinding(hCont, g_bindedIdTable[BINDED_ID_TOGGLE]);

            pluginSetTrayIcon(hCont, g_trayId, g_pointerOffIcon);

            hideCursorWindow(g_hCursorWindow);

            KnceUtil::restoreKeyRepeatSpeed();
        }

        return 1;
    }
    else if (g_isPointingEnabled) {
        if (id == g_bindedIdTable[BINDED_ID_LEFT] ||
            id == g_bindedIdTable[BINDED_ID_LEFT | 0x8000] ||
            id == g_bindedIdTable[BINDED_ID_RIGHT] ||
            id == g_bindedIdTable[BINDED_ID_RIGHT | 0x8000] ||
            id == g_bindedIdTable[BINDED_ID_UP] ||
            id == g_bindedIdTable[BINDED_ID_UP | 0x8000] ||
            id == g_bindedIdTable[BINDED_ID_DOWN] ||
            id == g_bindedIdTable[BINDED_ID_DOWN | 0x8000]) {

            int speed = POINTER_SPEED;
            if (id == g_bindedIdTable[BINDED_ID_LEFT | 0x8000] ||
                id == g_bindedIdTable[BINDED_ID_RIGHT | 0x8000] ||
                id == g_bindedIdTable[BINDED_ID_UP | 0x8000] ||
                id == g_bindedIdTable[BINDED_ID_DOWN | 0x8000]) {

                speed *= 2;
            }

            RECT desktopRect;
            HWND hDesktop = GetDesktopWindow();
            GetWindowRect(hDesktop, &desktopRect); 

            if (id == g_bindedIdTable[BINDED_ID_LEFT] ||
                id == g_bindedIdTable[BINDED_ID_LEFT | 0x8000]) {

                g_pointerX -= speed;
                g_pointerX = g_pointerX < 0 ? 0 : g_pointerX;
            }
            else if (id == g_bindedIdTable[BINDED_ID_RIGHT] ||
                id == g_bindedIdTable[BINDED_ID_RIGHT | 0x8000]) {

                g_pointerX += speed;
                g_pointerX = g_pointerX >= desktopRect.right ?
                    desktopRect.right - 1 : g_pointerX;
            }
            else if (id == g_bindedIdTable[BINDED_ID_UP] ||
                id == g_bindedIdTable[BINDED_ID_UP | 0x8000]) {

                g_pointerY -= speed;
                g_pointerY = g_pointerY < 0 ? 0 : g_pointerY;
            }
            else if (id == g_bindedIdTable[BINDED_ID_DOWN] ||
                id == g_bindedIdTable[BINDED_ID_DOWN | 0x8000]) {

                g_pointerY += speed;
                g_pointerY = g_pointerY >= desktopRect.bottom ?
                    desktopRect.bottom - 1 : g_pointerY;
            }

            mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
                65536 * g_pointerX / desktopRect.right,
                65535 * g_pointerY / desktopRect.bottom, 0, NULL);

            moveCursorWindow(g_hCursorWindow, g_pointerX, g_pointerY);
        }
        else if (id == g_bindedIdTable[BINDED_ID_CLICK]) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, NULL);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, NULL);
        }
        else if (id == g_bindedIdTable[BINDED_ID_TOGGLE]) {
            if (g_isPressed)
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, NULL);
            else
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, NULL);

            g_isPressed = !g_isPressed;
        }

        return 1;
    }

    return 0;
}

static void loadConfig() {
    tstring pluginDir = KnceUtil::getCurrentDirectory() + _T("\\plugins");
    tstring propFileName = pluginDir + _T("\\openerpointer.dat");

    if (GetFileAttributes(propFileName.c_str()) == -1) {
        g_bindedKeyCodeTable[BINDED_ID_SWITCH] = HARD_KEY_LARGER;
        g_bindedKeyCodeTable[BINDED_ID_CLOSE] = HARD_KEY_SMALLER;
        g_bindedKeyCodeTable[BINDED_ID_CLICK] = 'Z';
        g_bindedKeyCodeTable[BINDED_ID_TOGGLE] = 'A';
    }
    else {
        map<tstring, tstring> props;
        KnceUtil::readPropertyFile(props, propFileName);

        g_bindedKeyCodeTable[BINDED_ID_SWITCH] =
            _ttoi(props[_T("keyBinding.switch")].c_str());
        g_bindedKeyCodeTable[BINDED_ID_CLOSE] =
            _ttoi(props[_T("keyBinding.close")].c_str());
        g_bindedKeyCodeTable[BINDED_ID_CLICK] =
            _ttoi(props[_T("keyBinding.click")].c_str());
        g_bindedKeyCodeTable[BINDED_ID_TOGGLE] =
            _ttoi(props[_T("keyBinding.toggle")].c_str());
    }
}

static int registerKeyBinding(HWND hCont, const tstring &name, bool fn,
    unsigned int keyCode, bool active) {

    PluginKeyBinding bind = {0};

    if (!name.empty()) {
        _tcscpy(bind.category, _T("OpenerPointer"));
        _tcsncpy(bind.name, name.c_str(), 256);
    }

    bind.isEnabled = true;
    bind.defaultFn = fn;
    bind.defaultKeyCode = keyCode;
    bind.onKeyPress = onBindedKeyPress;
    bind.isActive = active;

    return pluginRegisterKeyBinding(hCont, &bind);
}
