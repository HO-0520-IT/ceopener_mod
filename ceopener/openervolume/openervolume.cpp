#include <string>
#include <windows.h>
#include <knceutil.h>
#include <ceopener_plugin.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

#ifdef OPENERVOLUME_EXPORTS
#define OPENERVOLUME_API __declspec(dllexport)
#else
#define OPENERVOLUME_API __declspec(dllimport)
#endif

enum {
};

HWND createPanel(HWND hOwnerWindow);
void destroyPanel(HWND hDlg);
void showPanel(HWND hDlg);
void hidePanel(HWND hDlg);
void setPanelAttachedRect(HWND hDlg, const RECT *rect);
void slidePanelVolumePosition(HWND hDlg, int offset);

static void onPluginTrayIconItemClicked(HWND hCont, int id);
static int onBindedKeyPress(HWND hWnd, int id);

HINSTANCE g_hInstance = NULL;
HFONT g_hMainFont = NULL;
BOOL g_bOpening = FALSE;
static HWND g_hPanel = NULL;
static int g_bindedVolumeDownId = 0;
static int g_bindedVolumeUpId = 0;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hInstance = (HINSTANCE)hInst;

    return true;
}

extern "C" OPENERVOLUME_API int pluginInit(HWND hCont) {
    g_hMainFont = (HFONT)GetStockObject(SYSTEM_FONT);

    PluginTrayIconItem trayItem = {0};
    trayItem.hIcon = (HICON)LoadImage(g_hInstance, _T("TRAY"), IMAGE_ICON,
        0, 0, 0);
    trayItem.onClicked = onPluginTrayIconItemClicked;
    trayItem.uID = 100;

    pluginRegisterTrayIconItem(hCont, g_hInstance, &trayItem);

    g_hPanel = createPanel(hCont);

    PluginKeyBinding bind = {0};
    _tcscpy(bind.category, _T("ceOpener"));
    _tcscpy(bind.name, _T("âπó Çâ∫Ç∞ÇÈ"));
    bind.isEnabled = false;
    bind.defaultKeyCode = HARD_KEY_VOLUMEDOWN;
    bind.onKeyPress = onBindedKeyPress;
    bind.isActive = true;

    g_bindedVolumeDownId = pluginRegisterKeyBinding(hCont, &bind);

    _tcscpy(bind.name, _T("âπó Çè„Ç∞ÇÈ"));
    bind.defaultKeyCode = HARD_KEY_VOLUMEUP;

    g_bindedVolumeUpId = pluginRegisterKeyBinding(hCont, &bind);

    return true;
}

extern "C" OPENERVOLUME_API void pluginTerminate(HWND hCont) {
    pluginUnregisterKeyBinding(hCont, g_bindedVolumeDownId);
    pluginUnregisterKeyBinding(hCont, g_bindedVolumeUpId);

    destroyPanel(g_hPanel);

    DeleteObject(g_hMainFont);
}

static void onPluginTrayIconItemClicked(HWND hCont, int id) {
	HWND hTrayCtrl = pluginGetTrayIconControl(hCont, id);

	if (!g_bOpening) {
		RECT windowRect;
		GetWindowRect(hTrayCtrl, &windowRect);

		setPanelAttachedRect(g_hPanel, &windowRect);

		showPanel(g_hPanel);
		g_bOpening = TRUE;
	}
	else {
		hidePanel(hTrayCtrl);
		g_bOpening = FALSE;
	}
}

static int onBindedKeyPress(HWND hWnd, int id) {
    if (id == g_bindedVolumeDownId)
        slidePanelVolumePosition(g_hPanel, -1);
    else if (id == g_bindedVolumeUpId)
        slidePanelVolumePosition(g_hPanel, 1);

    return true;
}

