#include <string>
#include <algorithm>
#include <map>
#include <windows.h>
#include <knceutil.h>
#include <ceopener_plugin.h>
#include "rundlg.h"

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

enum {
    MENU_INDEX_RUN = 0,
    MENU_INDEX_CONTROL_PANEL = 1,
    MENU_INDEX_EXPLORER = 2
};

static void onPluginMainMenuItemSelected(HWND hCont, HWND hMenu, int id);
static void onPluginLauncherMenuShow(HWND hCont, HWND hMenu);
static void onPluginLauncherMenuItemSelected(HWND hCont, HWND hMenu, int id);
static void addToBucket(vector<pair<tstring, tstring> > &captToPath,
    const tstring &dir);

HINSTANCE g_hInstance = NULL;
HFONT g_hMainFont = NULL;
static vector<tstring> g_programHistory;
static map<int, int> g_menuIdTable;
static map<int, tstring> g_applicationPathTable;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hInstance = (HINSTANCE)hInst;

    return true;
}

extern "C" OPENERSHOT_API int pluginInit(HWND hCont) {
    g_hMainFont = (HFONT)GetStockObject(SYSTEM_FONT);

    PluginMenu subMenu = {0};
    subMenu.onShow = onPluginLauncherMenuShow;

    HWND hSubMenu = pluginCreateSubMenu(hCont, &subMenu);

    PluginMenuItem menuItem = {0};
    menuItem.onItemSelected = onPluginMainMenuItemSelected;

    menuItem.hSubMenu = hSubMenu;
    _tcscpy(menuItem.caption, _T("すべてのプログラム"));
    pluginAppendMenuItem(hCont, NULL, &menuItem);

    menuItem.hSubMenu = NULL;
    _tcscpy(menuItem.caption, _T("実行..."));
    g_menuIdTable[MENU_INDEX_RUN] =
        pluginAppendMenuItem(hCont, NULL, &menuItem);

    _tcscpy(menuItem.caption, _T("コントロール パネル"));
    g_menuIdTable[MENU_INDEX_CONTROL_PANEL] =
        pluginAppendMenuItem(hCont, NULL, &menuItem);

    _tcscpy(menuItem.caption, _T("マイ デバイス"));
    g_menuIdTable[MENU_INDEX_EXPLORER] =
        pluginAppendMenuItem(hCont, NULL, &menuItem);

    return true;
}

extern "C" OPENERSHOT_API void pluginTerminate(HWND hCont) {
    DeleteObject(g_hMainFont);
}

static void onPluginMainMenuItemSelected(HWND hCont, HWND hMenu, int id) {
    int i;

    // [Run...]
    if (id == g_menuIdTable[MENU_INDEX_RUN]) {
        RunParams params;
        vector<tstring> &inProgs = params.getProgramHistory();
        int numProgs = g_programHistory.size();

        for (i = 0; i < numProgs; i++)
            inProgs.push_back(g_programHistory[i]);

        if (!showRunDialog(hCont, params))
            return;

        g_programHistory.clear();
        vector<tstring> &outProgs = params.getProgramHistory();
        numProgs = outProgs.size();

        for (i = 0; i < numProgs; i++)
            g_programHistory.push_back(outProgs[i]);
    }

    // [Control Panel]
    else if (id == g_menuIdTable[MENU_INDEX_CONTROL_PANEL]) {
        CreateProcess(_T("\\Windows\\control.exe"), _T(""), NULL, NULL, false,
            0, NULL, NULL, NULL, NULL);
    }

    // [Explorer]
    else if (id == g_menuIdTable[MENU_INDEX_EXPLORER]) {
        CreateProcess(_T("\\Windows\\explorer.exe"), _T(""), NULL, NULL, false,
            0, NULL, NULL, NULL, NULL);
    }
}

static void onPluginLauncherMenuShow(HWND hCont, HWND hMenu) {
    int i;

    vector<pair<tstring, tstring> > captToPath;

    tstring dir = _T("\\NAND3\\アプリ");
    tstring findPath = dir + _T("\\*.*");

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(findPath.c_str(), &fd);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            addToBucket(captToPath, dir + _T("\\") + fd.cFileName);

        while (FindNextFile(hFind, &fd)) {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                addToBucket(captToPath, dir + _T("\\") + fd.cFileName);
        }
    }

    FindClose(hFind);

    dir = _T("\\Storage Card\\アプリ");
    findPath = dir + _T("\\*.*");

    hFind = FindFirstFile(findPath.c_str(), &fd);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
            addToBucket(captToPath, dir + _T("\\") + fd.cFileName);

        while (FindNextFile(hFind, &fd)) {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                addToBucket(captToPath, dir + _T("\\") + fd.cFileName);
        }
    }

    FindClose(hFind);

    pluginClearSubMenu(hCont, hMenu);

    g_applicationPathTable.clear();

    PluginMenuItem menuItem = {0};
    int numApps = captToPath.size();

    for (i = 0; i < numApps; i++) {
        _tcscpy(menuItem.caption, captToPath[i].first.c_str());
        menuItem.onItemSelected = onPluginLauncherMenuItemSelected;

        int id = pluginAppendMenuItem(hCont, hMenu, &menuItem);

        g_applicationPathTable[id] = captToPath[i].second;
    }
}

static void onPluginLauncherMenuItemSelected(HWND hCont, HWND hMenu, int id) {
    SHELLEXECUTEINFO execInfo = {0};
    execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    execInfo.fMask = 0;
    execInfo.hwnd = hCont;
    execInfo.lpVerb = _T("open");
    execInfo.lpFile = g_applicationPathTable[id].c_str();
    execInfo.lpParameters = _T("");
    execInfo.nShow = SW_SHOWNORMAL;
    execInfo.hInstApp = g_hInstance;

    ShellExecuteEx(&execInfo);
}

static void addToBucket(vector<pair<tstring, tstring> > &captToPath,
    const tstring &dir) {

    tstring indexFileName = dir + _T("\\index.din");
    tstring appFileName = dir + _T("\\AppMain.exe");

    if (GetFileAttributes(indexFileName.c_str()) == -1 ||
        GetFileAttributes(appFileName.c_str()) == -1) {

        return;
    }

    tstring capt = dir.substr(dir.rfind('\\') + 1);

    vector<pair<tstring, tstring> >::iterator iter = captToPath.begin();
    for ( ; iter != captToPath.end(); iter++) {
        if (_tcsicmp(iter->first.c_str(), capt.c_str()) > 0)
            break;
    }

    captToPath.insert(iter, make_pair(capt, appFileName));
}
