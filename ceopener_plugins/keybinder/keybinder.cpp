#include <map>
#include <algorithm>
#include <string>
#include <windows.h>
#include <keybd.h>  // key input
#include <knceutil.h>
#include <ceopener_plugin.h>
#include "binddlg.h"

/* HOOK CODES
#define WH_JOURNALRECORD	0
#define WH_JOURNALPLAYBACK  1
#define WH_KEYBOARD_LL	  20



// Hook Codes
 
#define HC_ACTION		   0
#define HC_GETNEXT		  1
#define HC_SKIP			 2
#define HC_NOREMOVE		 3
#define HC_NOREM			HC_NOREMOVE
#define HC_SYSMODALON	   4
#define HC_SYSMODALOFF	  5

typedef LRESULT (CALLBACK* HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);

typedef struct tagKBDLLHOOKSTRUCT {
	DWORD vkCode;		// virtual key code
	DWORD scanCode;		// scan code	DWORD flags;	   // flags
	DWORD flags;		// unused
	DWORD time;			// time stamp for this message
	DWORD dwExtraInfo;	// extra info from the driver or keybd_event
} KBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;

typedef HHOOK (WINAPI *SETWINDOWSHOOKEX)(int, HOOKPROC, HINSTANCE, DWORD);
typedef BOOL (WINAPI *UNHOOKWINDOWSHOOKEX)(HHOOK);
typedef LRESULT (WINAPI *CALLNEXTHOOKEX)(HHOOK, int, WPARAM, LPARAM);

SETWINDOWSHOOKEX SetWindowsHookExW;
#define SetWindowsHookEx  SetWindowsHookExW

UNHOOKWINDOWSHOOKEX UnhookWindowsHookEx;

CALLNEXTHOOKEX CallNextHookEx;
*/

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

#ifdef KEYBINDER_EXPORTS
#define KEYBINDER_API __declspec(dllexport)
#else
#define KEYBINDER_API __declspec(dllimport)
#endif

enum {
};

static void onPluginTrayIconItemClicked(HWND hCont, int id);
static void onPluginMenuItemSelected(HWND hCont, HWND hMenu, int id);
/* HOOK CODES
static bool onHookedKeyDown(DWORD vkCode, UINT msg);
*/
static int onBindedKeyPress(HWND hCont, int id);
static void onTaskSwitched(HWND hCont, unsigned long params);
static void showBindDialog(HWND hCont);
static void setupKeyBindings(HWND hCont);
static void releaseKeyBindings(HWND hCont);
static void adjustVolume(int inc);
static void sendTextToTarget(const tstring &text);
static void loadSettings();
static void saveSettings();
static void loadRegistry();
static void saveRegistry();

HINSTANCE g_hInstance = NULL;
HFONT g_hMainFont = NULL;
static vector<BindItem *> g_bindItems;
static map<int, BindItem *> g_bindingTable;
static vector<int> g_volumeLevels;
static void adjustVolume(int inc);
static bool g_isDisableAtEd = false;
static bool g_isStoreData = false;

/* HOOK CODES
#pragma data_seg("BINDER_DATA")
HHOOK g_hHook = 0;
#pragma data_seg()
*/

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH)
		g_hInstance = (HINSTANCE)hInst;

	return true;
}

/* HOOK CODES
extern "C" KEYBINDER_API LRESULT CALLBACK BinderHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	KBDLLHOOKSTRUCT *keybdHook = (PKBDLLHOOKSTRUCT)lParam;

	if(nCode != 0)
		return CallNextHookEx(g_hHook, nCode, wParam, lParam);

	if(!onHookedKeyDown(keybdHook->vkCode, (UINT)wParam))
		return CallNextHookEx(g_hHook, nCode, wParam, lParam);

	return TRUE;
}
*/

extern "C" KEYBINDER_API int pluginInit(HWND hCont) {
	int i;

	PluginTrayIconItem trayItem = {0};
	trayItem.hIcon = (HICON)LoadImage(g_hInstance, _T("TRAY"), IMAGE_ICON,
		0, 0, 0);
	trayItem.onClicked = onPluginTrayIconItemClicked;
	trayItem.uID = 100;

	int id = pluginRegisterTrayIconItem(hCont, g_hInstance, &trayItem);

	HWND hPluginMenu = pluginGetPluginMenu(hCont);

	PluginMenuItem menuItem = {0};
	_tcscpy(menuItem.caption, _T("ÉLÅ[ÇÃäÑÇËìñÇƒ..."));
	menuItem.onItemSelected = onPluginMenuItemSelected;

	pluginAppendMenuItem(hCont, hPluginMenu, &menuItem);

	int vols[] = {0x0421, 0x35ad, 0x6739, 0x8841, 0xa107, 0xb9cd, 0xca51,
		0xdad5, 0xeb59, 0xfbdd};
	for (i = 0; i < 10; i++)
		g_volumeLevels.push_back(vols[i]);

	loadSettings();

	setupKeyBindings(hCont);

	pluginRegisterCallback(hCont, PLUGIN_CALLBACK_TASK_SWITCHED,
		onTaskSwitched);

	/* HOOK CODES
	HMODULE hCoreMod = LoadLibrary(_T("COREDLL"));
	SetWindowsHookExW = (SETWINDOWSHOOKEX)GetProcAddress(hCoreMod, _T("SetWindowsHookExW"));
	UnhookWindowsHookEx = (UNHOOKWINDOWSHOOKEX)GetProcAddress(hCoreMod, _T("UnhookWindowsHookEx"));
	CallNextHookEx = (CALLNEXTHOOKEX)GetProcAddress(hCoreMod, _T("CallNextHookEx"));

	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, BinderHookProc, g_hInstance, 0);
	*/

	return true;
}

extern "C" KEYBINDER_API void pluginTerminate(HWND hCont) {
	int i;

	/* HOOK CODES
	UnhookWindowsHookEx(g_hHook);
	*/

	releaseKeyBindings(hCont);

	int numItems = g_bindItems.size();
	for (i = 0; i < numItems; i++)
		delete g_bindItems[i];

	DeleteObject(g_hMainFont);
}

static void onPluginMenuItemSelected(HWND hCont, HWND hMenu, int id) {
	showBindDialog(hCont);
}

static void onPluginTrayIconItemClicked(HWND hCont, int id) {
	showBindDialog(hCont);
}

/* HOOK CODES
static bool onHookedKeyDown(DWORD vkCode, UINT msg) {
	if (g_bindingTable.find(vkCode) == g_bindingTable.end())
		return 0;

	BindItem *bindItem = g_bindingTable[vkCode];

	int actType = bindItem->getActionType();

	if(msg == WM_KEYDOWN) {
		if (actType == BindItem::ACTION_MODIFY) {
			int modCode = bindItem->getModKeyCode();
			if (modCode == BindItem::MODIFY_ALT)
				keybd_event(VK_MENU, 0, 0, NULL);
			else if(modCode == BindItem::MODIFY_CTRL)
				keybd_event(VK_CONTROL, 0, 0, NULL);
			else
				keybd_event(VK_SHIFT, 0, 0, NULL);
		}
		else
			return false;
	}
	else if(msg == WM_KEYUP) {
		if (actType == BindItem::ACTION_MODIFY) {
			int modCode = bindItem->getModKeyCode();
			if (modCode == BindItem::MODIFY_ALT)
				keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, NULL);
			else if(modCode == BindItem::MODIFY_CTRL)
				keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, NULL);
			else
				keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, NULL);
		}
		else
			return false;
	}

	return true;
}
*/

static int onBindedKeyPress(HWND hCont, int id) {
	if (g_bindingTable.find(id) == g_bindingTable.end())
		return 0;

	BindItem *bindItem = g_bindingTable[id];

	int actType = bindItem->getActionType();
	if (actType == BindItem::ACTION_VIRTUAL_KEY) {
		int virtKeyMods = bindItem->getVirtualKeyModifiers();
		int virtKeyCode = bindItem->getVirtualKeyCode();

		if ((virtKeyMods & MOD_ALT) != 0)
			keybd_event(VK_MENU, 0, 0, NULL);

		if ((virtKeyMods & MOD_CONTROL) != 0)
			keybd_event(VK_CONTROL, 0, 0, NULL);

		if ((virtKeyMods & MOD_SHIFT) != 0)
			keybd_event(VK_SHIFT, 0, 0, NULL);

		keybd_event(virtKeyCode, 0, 0, NULL);
		keybd_event(virtKeyCode, 0, KEYEVENTF_KEYUP, NULL);

		if ((virtKeyMods & MOD_ALT) != 0)
			keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, NULL);

		if ((virtKeyMods & MOD_CONTROL) != 0)
			keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, NULL);

		if ((virtKeyMods & MOD_SHIFT) != 0)
			keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, NULL);
	}
	else if (actType == BindItem::ACTION_PROGRAM) {
		SHELLEXECUTEINFO execInfo = {0};
		LPTSTR lpFileName = new TCHAR[MAX_PATH];
		execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		execInfo.fMask = 0;
		execInfo.hwnd = hCont;
		execInfo.lpVerb = _T("open");
		lstrcpy(lpFileName, bindItem->getProgramFileName().c_str());
		execInfo.lpFile = lpFileName;
		execInfo.lpParameters = _T("");
		execInfo.nShow = SW_SHOWNORMAL;
		execInfo.hInstApp = g_hInstance;

		ShellExecuteEx(&execInfo);
		delete lpFileName;
	}
	else {
		int specialAct = bindItem->getSpecialAction();

		if (specialAct == BindItem::SPECIAL_ACTION_FOREGROUND_ED_WINDOW) {
			HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);
			if (hEdWindow != NULL)
				SetForegroundWindow(hEdWindow);
		}
		else if (specialAct == BindItem::SPECIAL_ACTION_VOLUME_DOWN)
			adjustVolume(-1);
		else if (specialAct == BindItem::SPECIAL_ACTION_VOLUME_UP)
			adjustVolume(1);
	}

	return true;
}

static void onTaskSwitched(HWND hCont, unsigned long params) {
	PluginCallbackTaskSwitchedParams *data =
		(PluginCallbackTaskSwitchedParams*)params;

	if(g_isDisableAtEd) {
		HWND hEdWnd = FindWindow(_T("SHARP SIM"), NULL);

		if(data->hWindowTo == hEdWnd)
			releaseKeyBindings(hCont);
		else if(data->hWindowFrom == hEdWnd)
			setupKeyBindings(hCont);
	}
}

static void showBindDialog(HWND hCont) {
	int i;

	releaseKeyBindings(hCont);

	pluginSuspendKeyBindings(hCont);

	BindDialogParams params;
	vector<BindItem *> &bindItems = params.getBindItems();
	params.setDisableAtEd(g_isDisableAtEd);
	params.setStoreData(g_isStoreData);

	int numItems = g_bindItems.size();
	for (i = 0; i < numItems; i++)
		bindItems.push_back(g_bindItems[i]);

	if (showBindDialog(hCont, params)) {
		g_bindItems.clear();

		numItems = bindItems.size();
		for (i = 0; i < numItems; i++)
			g_bindItems.push_back(bindItems[i]);
	}

	pluginResumeKeyBindings(hCont);

	setupKeyBindings(hCont);
	g_isDisableAtEd = params.isDisableAtEd();
	g_isStoreData = params.isStoreData();
		saveSettings();
	if(!g_isStoreData)
		saveRegistry();
}

static void setupKeyBindings(HWND hCont) {
	int i;

	g_bindingTable.clear();

	PluginKeyBinding bind = {0};
	bind.isEnabled = true;
	bind.onKeyPress = onBindedKeyPress;
	bind.isActive = true;

	int numItems = g_bindItems.size();
	for (i = 0; i < numItems; i++) {
		BindItem *bindItem = g_bindItems[i];

		bind.defaultFn = bindItem->getTargetKeyFn();
		bind.defaultKeyCode = bindItem->getTargetKeyCode();
		int id = pluginRegisterKeyBinding(hCont, &bind);

		g_bindingTable[id] = bindItem;
	}
}

static void releaseKeyBindings(HWND hCont) {
	map<int, BindItem *>::const_iterator iter = g_bindingTable.begin();
	for ( ; iter != g_bindingTable.end(); iter++)
		pluginUnregisterKeyBinding(hCont, iter->first);
}

static void adjustVolume(int inc) {
	DWORD vol = 0;
	waveOutGetVolume(NULL, &vol);
	vol &= 0xffff;

	vector<int>::iterator iter = find_if(g_volumeLevels.begin(),
		g_volumeLevels.end(), bind2nd(greater<int>(), vol));
	int level = distance(g_volumeLevels.begin(), iter) - 1;

	level += inc;
	level = level < 0 ? 0 : (level > 9 ? 9 : level);

	vol = g_volumeLevels[level];
	vol |= vol << 16;
	waveOutSetVolume(NULL, vol);

	MessageBeep(-1);
}

static void loadSettings() {
	int i;

	tstring persistDir = _T("\\Nand2\\.ceopener");
	tstring propFileName = persistDir + _T("\\keybinder.dat");
	if (GetFileAttributes(propFileName.c_str()) == -1) {
		g_isDisableAtEd = true;
		g_isStoreData = true;
	}

	map<tstring, tstring> props;
	KnceUtil::readPropertyFile(props, propFileName);

	if(props.find(_T("disableAtEdApp")) == props.end())
		g_isDisableAtEd = true;
	else
		g_isDisableAtEd = props[_T("disableAtEdApp")] == _T("1") ? true : false;

	if(props.find(_T("storeData")) == props.end())
		g_isStoreData = true;
	else
		g_isStoreData = props[_T("storeData")] == _T("1") ? true : false;


	TCHAR keyCStr[256];
	TCHAR end[1] = { 0 };


	if(g_isStoreData) {

		int numItems = _tcstol(props[_T("NumBindings")].c_str(), (TCHAR**)&end, 10);

		for (i = 0; i < numItems; i++) {
			BindItem *bindItem = new BindItem();

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dName"), i);

		
			bindItem->setName(props[keyCStr]);

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dTargetKeyFn"), i);

			int targetKeyFn = _tcstol(props[keyCStr].c_str(), (TCHAR**)&end, 10);
			bindItem->setTargetKeyFn(targetKeyFn != 0);

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dTargetKeyCode"), i);

			int targetKeyCode = _tcstol(props[keyCStr].c_str(), (TCHAR**)&end, 10);
			bindItem->setTargetKeyCode(targetKeyCode);

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dActionType"), i);

			int actType = _tcstol(props[keyCStr].c_str(), (TCHAR**)&end, 10);;
			bindItem->setActionType(actType);

			_sntprintf(keyCStr, sizeof(keyCStr),
				_T("Binding%dVirtualKeyModifiers"), i);

			int virtKeyMods = _tcstol(props[keyCStr].c_str(), (TCHAR**)&end, 10);
			bindItem->setVirtualKeyModifiers(virtKeyMods);

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dVirtualKeyCode"), i);

			int virtKeyCode = _tcstol(props[keyCStr].c_str(), (TCHAR**)&end, 10);
			bindItem->setVirtualKeyCode(virtKeyCode);

			_sntprintf(keyCStr, sizeof(keyCStr),
				_T("Binding%dProgramFileName"), i);

			bindItem->setProgramFileName(props[keyCStr]);

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dSpecialAction"), i);

			int specialAct = _tcstol(props[keyCStr].c_str(), (TCHAR**)&end, 10);
			bindItem->setSpecialAction(specialAct);
		
			g_bindItems.push_back(bindItem);
		}
	}
	else
		loadRegistry();
}

static void saveSettings() {
	map<tstring, tstring> props;
	TCHAR buf[256];
	int i;

	TCHAR keyCStr[256];

	if(g_isStoreData) {

		int numItems = g_bindItems.size();
		_itot(numItems, buf, 10);
		props[_T("NumBindings")] = buf;

		for (i = 0; i < numItems; i++) {
			BindItem *bindItem = g_bindItems[i];

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dName"), i);

			tstring name = bindItem->getName();
			props[keyCStr] = name;

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dTargetKeyFn"), i);

			int targetKeyFn = bindItem->getTargetKeyFn();
			_itot(targetKeyFn, buf, 10);
			props[keyCStr] = buf;

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dTargetKeyCode"), i);

			int targetKeyCode = bindItem->getTargetKeyCode();
			_itot(targetKeyCode, buf, 10);
			props[keyCStr] = buf;

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dActionType"), i);

			int actType = bindItem->getActionType();
			_itot(actType, buf, 10);
			props[keyCStr] = buf;
		
			_sntprintf(keyCStr, sizeof(keyCStr),
				_T("Binding%dVirtualKeyModifiers"), i);

			int virtKeyMods = bindItem->getVirtualKeyModifiers();
			_itot(virtKeyMods, buf, 10);
			props[keyCStr] = buf;

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dVirtualKeyCode"), i);

			int virtKeyCode = bindItem->getVirtualKeyCode();
			_itot(virtKeyCode, buf, 10);
			props[keyCStr] = buf;

			_sntprintf(keyCStr, sizeof(keyCStr),
				_T("Binding%dProgramFileName"), i);

			tstring progFileName = bindItem->getProgramFileName();
			props[keyCStr] = progFileName;

			_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dSpecialAction"), i);

			int specialAct = bindItem->getSpecialAction();
			_itot(specialAct, buf, 10);
			props[keyCStr] = buf;
		}
	}

	props[_T("disableAtEdApp")] = g_isDisableAtEd ? _T("1") : _T("0");
	props[_T("storeData")] = g_isStoreData ? _T("1") : _T("0");

	tstring persistDir = _T("\\NAND2\\.ceopener");
	if (GetFileAttributes(persistDir.c_str()) == -1)
		CreateDirectory(persistDir.c_str(), NULL);

	tstring propFileName = persistDir + _T("\\keybinder.dat");
	KnceUtil::writePropertyFile(propFileName, props);
}

static void loadRegistry() {
	int i;

	HKEY hKey = NULL;
	RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\knatech\\keybinder"), 0,
		NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	TCHAR keyCStr[256];

	DWORD size = 256;
	int numItems = 0;
	RegQueryValueEx(hKey, _T("NumBindings"), NULL, NULL, (BYTE *)&numItems,
		&size);

	for (i = 0; i < numItems; i++) {
		BindItem *bindItem = new BindItem();

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dName"), i);

		TCHAR nameCStr[256];
		size = 256;
		RegQueryValueEx(hKey, keyCStr, NULL, NULL, (BYTE *)nameCStr, &size);
		bindItem->setName(nameCStr);

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dTargetKeyFn"), i);

		int targetKeyFn = 0;
		size = 4;
		RegQueryValueEx(hKey, keyCStr, NULL, NULL, (BYTE *)&targetKeyFn,
			&size);
		bindItem->setTargetKeyFn(targetKeyFn != 0);

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dTargetKeyCode"), i);

		int targetKeyCode = 0;
		size = 4;
		RegQueryValueEx(hKey, keyCStr, NULL, NULL, (BYTE *)&targetKeyCode,
			&size);
		bindItem->setTargetKeyCode(targetKeyCode);

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dActionType"), i);

		int actType = 0;
		size = 4;
		RegQueryValueEx(hKey, keyCStr, NULL, NULL, (BYTE *)&actType, &size);
		bindItem->setActionType(actType);

		_sntprintf(keyCStr, sizeof(keyCStr),
			_T("Binding%dVirtualKeyModifiers"), i);

		int virtKeyMods = 0;
		size = 4;
		RegQueryValueEx(hKey, keyCStr, NULL, NULL, (BYTE *)&virtKeyMods,
			&size);
		bindItem->setVirtualKeyModifiers(virtKeyMods);

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dVirtualKeyCode"), i);

		int virtKeyCode = 0;
		size = 4;
		RegQueryValueEx(hKey, keyCStr, NULL, NULL, (BYTE *)&virtKeyCode,
			&size);
		bindItem->setVirtualKeyCode(virtKeyCode);

		_sntprintf(keyCStr, sizeof(keyCStr),
			_T("Binding%dProgramFileName"), i);

		TCHAR progFileNameCStr[MAX_PATH];
		size = MAX_PATH;
		RegQueryValueEx(hKey, keyCStr, NULL, NULL, (BYTE *)progFileNameCStr,
			&size);
		bindItem->setProgramFileName(progFileNameCStr);

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dSpecialAction"), i);

		int specialAct = 0;
		size = 4;
		RegQueryValueEx(hKey, keyCStr, NULL, NULL, (BYTE *)&specialAct, &size);
		bindItem->setSpecialAction(specialAct);
		
		g_bindItems.push_back(bindItem);
	}

	RegCloseKey(hKey);
}

static void saveRegistry() {
	int i;

	HKEY hKey = NULL;
	RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\knatech\\keybinder"), 0,
		NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	TCHAR keyCStr[256];

	int numItems = g_bindItems.size();
	RegSetValueEx(hKey, _T("NumBindings"), 0, REG_DWORD,
		(const BYTE *)&numItems, 4);

	for (i = 0; i < numItems; i++) {
		BindItem *bindItem = g_bindItems[i];

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dName"), i);

		tstring name = bindItem->getName();
		RegSetValueEx(hKey, keyCStr, 0, REG_SZ, (const BYTE *)name.c_str(),
			(name.length() + 1) * sizeof(TCHAR));

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dTargetKeyFn"), i);

		int targetKeyFn = bindItem->getTargetKeyFn();
		RegSetValueEx(hKey, keyCStr, 0, REG_DWORD,
			(const BYTE *)&targetKeyFn, 4);

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dTargetKeyCode"), i);

		int targetKeyCode = bindItem->getTargetKeyCode();
		RegSetValueEx(hKey, keyCStr, 0, REG_DWORD,
			(const BYTE *)&targetKeyCode, 4);

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dActionType"), i);

		int actType = bindItem->getActionType();
		RegSetValueEx(hKey, keyCStr, 0, REG_DWORD, (const BYTE *)&actType, 4);

		_sntprintf(keyCStr, sizeof(keyCStr),
			_T("Binding%dVirtualKeyModifiers"), i);

		int virtKeyMods = bindItem->getVirtualKeyModifiers();
		RegSetValueEx(hKey, keyCStr, 0, REG_DWORD,
			(const BYTE *)&virtKeyMods, 4);

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dVirtualKeyCode"), i);

		int virtKeyCode = bindItem->getVirtualKeyCode();
		RegSetValueEx(hKey, keyCStr, 0, REG_DWORD,
			(const BYTE *)&virtKeyCode, 4);

		_sntprintf(keyCStr, sizeof(keyCStr),
			_T("Binding%dProgramFileName"), i);

		tstring progFileName = bindItem->getProgramFileName();
		RegSetValueEx(hKey, keyCStr, 0, REG_SZ,
			(const BYTE *)progFileName.c_str(),
			(progFileName.length() + 1) * sizeof(TCHAR));

		_sntprintf(keyCStr, sizeof(keyCStr), _T("Binding%dSpecialAction"), i);

		int specialAct = bindItem->getSpecialAction();
		RegSetValueEx(hKey, keyCStr, 0, REG_DWORD,
			(const BYTE *)&specialAct, 4);
	}

	RegCloseKey(hKey);
}