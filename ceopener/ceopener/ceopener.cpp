#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <sstream>
#include <windows.h>
#include <commctrl.h>
#include <knceutil.h>
#include <kncedlg.h>
#include <brainapi.h>
#include "menu.h"
#include "desktop.h"
#include "dlg/settingdlg.h"
#include "dlg/keybinddlg.h"
#include "ColorProfile.h"
#include "PluginManager.h"
#include "KeyBinder.h"
#include "SubTouchManager.h"
#include "DrawUtil.h"
#include "SEController.h"

#ifdef UNICODE
namespace std { typedef wstring tstring; typedef wstringstream tstringstream; }
#else
namespace std { typedef string tstring; typedef stringstream tstringstream; }
#endif

using namespace std;

#define PLUGIN_TRAY_ITEM_TEXT 0
#define PLUGIN_TRAY_ITEM_ICON 1
#define PLUGIN_TRAY_ITEM_BITMAP 2

#define PLUGIN_DESKTOP_ITEM_NO_MOVE = 1,
#define PLUGIN_DESKTOP_ITEM_NO_RESIZE = 2

#define PLUGIN_CALLBACK_TASK_SWITCHED 0
#define PLUGIN_CALLBACK_FOLDED 1
#define PLUGIN_CALLBACK_ED_CLOSED_OPENED 2
#define PLUGIN_CALLBACK_POWER_ON_OFF 3
#define PLUGIN_CALLBACK_SETTING_CHANGED 4

#define PLUGIN_ED_BEFORE_CLOSED 0
#define PLUGIN_ED_AFTER_CLOSED 1
#define PLUGIN_ED_BEFORE_OPENED 2
#define PLUGIN_ED_AFTER_OPENED 3
#define PLUGIN_POWER_OFF 1
#define PLUGIN_POWER_ON 2

enum {
	IDC_MAIN_MENU_BUTTON = 101,
	IDC_FOLDER_BUTTON = 102,
	IDC_TASK_ITEM_BUTTON_BASE = 1001,
	IDC_TRAY_ICON_CONTROL_BASE = 2001,
	//画面サイズ調整書き換え済み
	MAIN_MENU_BUTTON_WIDTH = 50,
	TASKBAR_BAND_GAP = 2,
	MAX_TASK_ITEM_WIDTH = 125,
	FOLDER_BUTTON_WIDTH = 15,
	TIMER_MAIN = 1,
	TIMER_POWER_ON = 2,
	TIMER_HOLDING = 3,
	TAP_HOLD_TIME = 500,
	KEY_BINDING_SHOW_MAIN_MENU = 1,
	KEY_BINDING_FOLD_TASKBAR = 2,
	KEY_BINDING_SHOW_DICTIONARY_SCREEN = 3,
	KEY_BINDING_SWITCH_TASK = 4,
};

typedef void (*PluginMenuShowCallback)(HWND hCont, HWND hMenu);
typedef void (*PluginMenuItemSelectedCallback)(HWND hCont, HWND hMenu, int id);
typedef void (*PluginTrayIconItemClickedCallback)(HWND hCont, int id);
typedef void (*PluginDesktopItemFocusCallback)(HWND hCont, int id);
typedef void (*PluginDesktopItemBlurCallback)(HWND hCont, int id);
typedef void (*PluginDesktopItemMouseButtonDownCallback)(HWND hCont, int id,
	int x, int y, RECT *place);
typedef void (*PluginDesktopItemMouseButtonUpCallback)(HWND hCont, int id,
	int x, int y, RECT *place);
typedef void (*PluginDesktopItemMouseMoveCallback)(HWND hCont, int id,
	int x, int y, RECT *place);
typedef void (*PluginDesktopItemDoubleClickCallback)(HWND hCont, int id,
	int x, int y, RECT *place);
typedef void (*PluginDesktopItemTapHoldCallback)(HWND hCont, int id,
	int x, int y, RECT *place);
typedef void (*PluginDesktopItemKeyDownCallback)(HWND hCont, int id, int keyCode);
typedef void (*PluginDesktopItemKeyUpCallback)(HWND hCont, int id, int keyCode);
typedef void (*PluginDesktopItemPaintCallback)(HWND hCont, int id, HDC hDC,
	RECT *place, int focused);
typedef int (*PluginKeyBindingCallback)(HWND hCont, int id);
typedef int (*PluginHotKeyCallback)(HWND hCont, int id);
typedef void (*PluginSubTouchCallback)(HWND hCont, int id);
typedef void (*PluginSubTouchDicKeyDownCallback)(HWND hCont, int id, int dicKey);
typedef void (*PluginTimerCallback)(HWND hCont, int id);
typedef void (*PluginCallback)(HWND hCont, unsigned long params);

struct PluginFontInfo {
	TCHAR faceName[LF_FACESIZE];
	int pointSize;
	int isBold;
	int isItalic;
};

struct PluginMenu {
	PluginMenuShowCallback onShow;
};

struct PluginMenuItem {
	TCHAR caption[256];
	PluginMenuItemSelectedCallback onItemSelected;
	HWND hSubMenu;
};

struct PluginTrayItem {
	int type;
	int width;
	int height;
	UINT uID;
};

struct PluginTrayIconItem {
	HICON hIcon;
	PluginTrayIconItemClickedCallback onClicked;
	UINT uID;
};

struct PluginDesktopItem {
	int isFocusable;
	int left;
	int top;
	int width;
	int height;
	int zOrder;
	PluginDesktopItemFocusCallback onFocus;
	PluginDesktopItemBlurCallback onBlur;
	PluginDesktopItemMouseButtonDownCallback onMouseButtonDown;
	PluginDesktopItemMouseButtonUpCallback onMouseButtonUp;
	PluginDesktopItemMouseMoveCallback onMouseMove;
	PluginDesktopItemDoubleClickCallback onDoubleClick;
	PluginDesktopItemTapHoldCallback onTapHold;
	PluginDesktopItemKeyDownCallback onKeyDown;
	PluginDesktopItemKeyUpCallback onKeyUp;
	PluginDesktopItemPaintCallback onPaint;
};

struct PluginSetDesktopItemPositionParams {
	int flags;
	int left;
	int top;
	int width;
	int height;
};

struct PluginKeyBinding {
	TCHAR category[256];
	TCHAR name[256];
	int isEnabled;
	int defaultFn;
	int defaultKeyCode;
	PluginKeyBindingCallback onKeyPress;
	int isActive;
};

struct PluginHotKey {
	unsigned int keyCode;
	int priority;
	PluginHotKeyCallback onHotKey;
};

struct PluginSubTouch {
	int priority;
	bool isEnabled;
	PluginSubTouchCallback onActive;
	PluginSubTouchCallback onInactive;
	PluginSubTouchDicKeyDownCallback onDicKeyDown;
};

struct PluginCallbackTaskSwitchedParams {
	HWND hWindowFrom;
	HWND hWindowTo;
};

class TaskItem {
public:
	TaskItem();
	virtual ~TaskItem();
	HWND getWindow() const { return m_hWindow; }
	void setWindow(HWND hWindow) { m_hWindow = hWindow; }
	HWND getButton() const { return m_hButton; }
	void setButton(HWND hButton) { m_hButton = hButton; }
	int getButtonId() const { return m_buttonId; }
	void setButtonId(int id) { m_buttonId = id; }
	bool isMarked() const { return m_isMarked; }
	void setMarked(bool isMarked) { m_isMarked = isMarked; }

private:
	HWND m_hWindow;
	HWND m_hButton;
	int m_buttonId;
	bool m_isMarked;
};

class TrayIconItem {
public:
	TrayIconItem();
	virtual ~TrayIconItem();
	HWND getControl() const { return m_hControl; }
	void setControl(HWND hCtrl) { m_hControl = hCtrl; }
	HICON getIcon() const { return m_hIcon; }
	void setIcon(HICON hIcon) { m_hIcon = hIcon; }
	UINT getType() const { return m_type; }
	void setType(UINT type) { m_type = type; }
	UINT getID() const { return m_uID; }
	void setID(UINT uID) { m_uID = uID; }
	UINT getTransparency() const { return *m_Transparency; }
	void setTransparency(UINT *Transparency) { m_Transparency = Transparency; }

private:
	HWND m_hControl;
	HICON m_hIcon;
	UINT m_type;
	UINT m_uID;
	UINT *m_Transparency;
};

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam);
static LRESULT CALLBACK trayIconControlProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam);
static LRESULT CALLBACK trayItemProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam);
static LRESULT CALLBACK taskItemProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam);
static LRESULT CALLBACK menuSubWindowProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam);
static BOOL CALLBACK showDesktopProc(HWND hWnd, LPARAM lParam);
static void onCreate(HWND hWnd);
static void onDestroy(HWND hWnd);
static void onClose(HWND hWnd);
static void onTaskItem(HWND hWnd, int id);
static void onMainMenu(HWND hWnd);
static void onFolder(HWND hWnd);
static void onPaint(HWND hWnd, HDC hDC);
static HBRUSH onCtlColorStatic(HWND hWnd, HDC hDC, HWND hStatic);
static void onDrawItem(HWND hWnd, DRAWITEMSTRUCT *drawItem);
static void onTimer(HWND hWnd, int idEvent);
static void onHotKey(HWND hWnd, int id);
static void onDicKeyDown(HWND hWnd, int dicKey);
static void onEdPowerOnOff(HWND hWnd, int status);
static void onMenuShow(HWND hWnd, HWND hMenu);
static void onMenuItemSelected(HWND hWnd, HWND hMenu, int id);
static bool onBindedKeyPress(HWND hWnd, int id);
static void onPluginGetFontInfo(HWND hWnd, PluginFontInfo *info);
static COLORREF onPluginGetColor(HWND hWnd, const TCHAR *name);
static int onPluginAppendMenuItem(HWND hWnd, HWND hMenu, PluginMenuItem *item);
static void onPluginRemoveMenuItem(HWND hWnd, HWND hMenu, int id);
static HWND onPluginCreateSubMenu(HWND hWnd, PluginMenu *menu);
static void onPluginClearSubMenu(HWND hWnd, HWND hMenu);
static void onPluginShowPopupMenu(HWND hWnd, HWND hMenu, POINT *pt);
static HWND onPluginGetMenu(HWND hWnd);
static HWND onPluginRegisterTrayItem(HWND hWnd, HMODULE hModule, PluginTrayItem *item);
static int onPluginRegisterTrayIconItem(HWND hWnd, HMODULE hModule, PluginTrayIconItem *item);
static void onPluginUnregisterTrayIconItem(HWND hWnd, int id);
static HWND onPluginGetTrayIconControl(HWND hWnd, int id);
static void onPluginSetTrayIcon(HWND hWnd, int id, HICON hIcon);
static HWND onPluginGetDesktopMenu(HWND hWnd);
static void onPluginDesktopToScreen(HWND hWnd, POINT *pt);
static int onPluginRegisterDesktopItem(HWND hWnd,
	PluginDesktopItem *item);
static void onPluginUnregisterDesktopItem(HWND hWnd, int id);
static void onPluginGetDesktopItemPlacement(HWND hWnd, int id, RECT *place);
static void onPluginSetDesktopItemPosition(HWND hWnd, int id,
	PluginSetDesktopItemPositionParams *params);
static void onPluginRedrawDesktopItem(HWND hWnd, int id);
static void onPluginFocusDesktopItem(HWND hWnd, int id);
static void onPluginSetDesktopMouseCapture(HWND hWnd, int id);
static void onPluginReleaseDesktopMouseCapture(HWND hWnd);
static int onPluginRegisterKeyBinding(HWND hWnd, PluginKeyBinding *bind);
static void onPluginUnregisterKeyBinding(HWND hWnd, int id);
static void onPluginActivateKeyBinding(HWND hWnd, int id, int active);
static void onPluginSuspendKeyBindings(HWND hWnd);
static void onPluginResumeKeyBindings(HWND hWnd);
static int onPluginRegisterSubTouch(HWND hWnd, PluginSubTouch *touch);
static void onPluginUnregisterSubTouch(HWND hWnd, int id);
static void onPluginEnableSubTouch(HWND hWnd, int id, int enable);
static int onPluginHardKeyToDicKey(HWND hWnd, bool fn, int keyCode);
static int onPluginRegisterTimer(HWND hWnd, int interval,
	PluginTimerCallback callback);
static void onPluginUnregisterTimer(HWND hWnd, int id);
static void onPluginRegisterCallback(HWND hWnd, int type,
	PluginCallback callback);
static void onPluginUnregisterCallback(HWND hWnd, int type,
	PluginCallback callback);
static void launchFontLoader();
static void setupColorProfile();
static void setupKeyBindings(HWND hWnd);
static void registerKeyBinding(HWND hWnd, int id, const tstring &category,
	const tstring &name, bool fn, int keyCode);
static void setupDicKeyTable(HWND hWnd);
static void fold(HWND hWnd, bool needFold);
static void updateTasks(HWND hWnd);
static void switchTask(HWND hWnd, TaskItem *targetTask,
	HWND hTargetWindow = NULL);
static BOOL CALLBACK findWindowProcNoPopup(HWND hWnd, LPARAM lParam);
static BOOL CALLBACK findWindowProc(HWND hWnd, LPARAM lParam);
static bool checkTaskableNoPopup(HWND hWnd);
static bool checkTaskable(HWND hWnd);
static void switchTask(HWND hWnd, HWND hTargetWindow);
static int obtainNewTaskItemId(HWND hWnd);
static int obtainNewTrayIconControlId(HWND hWnd);
static int obtainNewTimerId(HWND hWnd);
static void sendPowerOnMessageToEd();
static void showSettingDialog(HWND hWnd);
static void showKeyBindingDialog(HWND hWnd);
static void changeWindowLayout(HWND hWnd);
static void saveCurrentSettings(HWND hWnd);
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR lpCmd,
	int nShow);

HINSTANCE g_hInstance = NULL;
HFONT g_hMainFont = NULL;
ColorProfile g_colorProfile;
HDC g_hBackDC = NULL;
LPDWORD g_lpBPix = NULL;
RECT g_EdRect = {0};
int TASKBAR_HEIGHT = 24;

static unsigned int g_dicKeyDownMessage =
	RegisterWindowMessage(_T("DicKeyDown"));
static unsigned int g_edPowerOnOffMessage =
	RegisterWindowMessage(_T("EdPowerOnOff"));
static unsigned int g_openerMenuShow =
	RegisterWindowMessage(_T("OpenerMenuShow"));
static unsigned int g_openerMenuItemSelected =
	RegisterWindowMessage(_T("OpenerMenuItemSelected"));
static unsigned int g_openerPluginGetFontInfo =
	RegisterWindowMessage(_T("OpenerPluginGetFontInfo"));
static unsigned int g_openerPluginGetColor =
	RegisterWindowMessage(_T("OpenerPluginGetColor"));
static unsigned int g_openerPluginAppendMenuItem =
	RegisterWindowMessage(_T("OpenerPluginAppendMenuItem"));
static unsigned int g_openerPluginRemoveMenuItem =
	RegisterWindowMessage(_T("OpenerPluginRemoveMenuItem"));
static unsigned int g_openerPluginCreateSubMenu =
	RegisterWindowMessage(_T("OpenerPluginCreateSubMenu"));
static unsigned int g_openerPluginClearSubMenu =
	RegisterWindowMessage(_T("OpenerPluginClearSubMenu"));
static unsigned int g_openerPluginShowPopupMenu =
	RegisterWindowMessage(_T("OpenerPluginShowPopupMenu"));
static unsigned int g_openerPluginGetPluginMenu =
	RegisterWindowMessage(_T("OpenerPluginGetPluginMenu"));
static unsigned int g_openerPluginRegisterTrayItem =
	RegisterWindowMessage(_T("OpenerPluginRegisterTrayItem"));
static unsigned int g_openerPluginRegisterTrayIconItem =
	RegisterWindowMessage(_T("OpenerPluginRegisterTrayIconItem"));
static unsigned int g_openerPluginUnregisterTrayIconItem =
	RegisterWindowMessage(_T("OpenerPluginUnregisterTrayIconItem"));
static unsigned int g_openerPluginGetTrayIconControl =
	RegisterWindowMessage(_T("OpenerPluginGetTrayIconControl"));
static unsigned int g_openerPluginSetTrayIcon =
	RegisterWindowMessage(_T("OpenerPluginSetTrayIcon"));
static unsigned int g_openerPluginGetDesktopMenu =
	RegisterWindowMessage(_T("OpenerPluginGetDesktopMenu"));
static unsigned int g_openerPluginDesktopToScreen =
	RegisterWindowMessage(_T("OpenerPluginDesktopToScreen"));
static unsigned int g_openerPluginRegisterDesktopItem =
	RegisterWindowMessage(_T("OpenerPluginRegisterDesktopItem"));
static unsigned int g_openerPluginUnregisterDesktopItem =
	RegisterWindowMessage(_T("OpenerPluginUnregisterDesktopItem"));
static unsigned int g_openerPluginGetDesktopItemPlacement =
	RegisterWindowMessage(_T("OpenerPluginGetDesktopItemPlacement"));
static unsigned int g_openerPluginSetDesktopItemPosition =
	RegisterWindowMessage(_T("OpenerPluginSetDesktopItemPosition"));
static unsigned int g_openerPluginRedrawDesktopItem =
	RegisterWindowMessage(_T("OpenerPluginRedrawDesktopItem"));
static unsigned int g_openerPluginFocusDesktopItem =
	RegisterWindowMessage(_T("OpenerPluginFocusDesktopItem"));
static unsigned int g_openerPluginSetDesktopMouseCapture =
	RegisterWindowMessage(_T("OpenerPluginSetDesktopMouseCapture"));
static unsigned int g_openerPluginReleaseDesktopMouseCapture =
	RegisterWindowMessage(_T("OpenerPluginReleaseDesktopMouseCapture"));
static unsigned int g_openerPluginRegisterKeyBinding =
	RegisterWindowMessage(_T("OpenerPluginRegisterKeyBinding"));
static unsigned int g_openerPluginUnregisterKeyBinding =
	RegisterWindowMessage(_T("OpenerPluginUnregisterKeyBinding"));
static unsigned int g_openerPluginActivateKeyBinding =
	RegisterWindowMessage(_T("OpenerPluginActivateKeyBinding"));
static unsigned int g_openerPluginSuspendKeyBindings =
	RegisterWindowMessage(_T("OpenerPluginSuspendKeyBindings"));
static unsigned int g_openerPluginResumeKeyBindings =
	RegisterWindowMessage(_T("OpenerPluginResumeKeyBindings"));
static unsigned int g_openerPluginRegisterSubTouch =
	RegisterWindowMessage(_T("OpenerPluginRegisterSubTouch"));
static unsigned int g_openerPluginUnregisterSubTouch =
	RegisterWindowMessage(_T("OpenerPluginUnregisterSubTouch"));
static unsigned int g_openerPluginEnableSubTouch =
	RegisterWindowMessage(_T("OpenerPluginEnableSubTouch"));
static unsigned int g_openerPluginHardKeyToDicKey =
	RegisterWindowMessage(_T("OpenerPluginHardKeyToDicKey"));
static unsigned int g_openerPluginRegisterTimer =
	RegisterWindowMessage(_T("OpenerPluginRegisterTimer"));
static unsigned int g_openerPluginUnregisterTimer =
	RegisterWindowMessage(_T("OpenerPluginUnregisterTimer"));
static unsigned int g_openerPluginRegisterCallback =
	RegisterWindowMessage(_T("OpenerPluginRegisterCallback"));
static unsigned int g_openerPluginUnregisterCallback =
	RegisterWindowMessage(_T("OpenerPluginUnregisterCallback"));

struct OpenerMainData {
	PluginManager pluginManager;
	KeyBinder keyBinder;
	SubTouchManager subTouchManager;
	HWND hTaskbarWindow;
	HWND hMainMenuWindow;
	HWND hDesktopWindow;
	OpenerDIBSection32 *hDesktopDIB;
	int exitMenuItemId;
	int settingsMenuItemId;
	int closeEdMenuItemId;
	int pluginsMenuItemId;
	int globalSettingsMenuItemId;
	int keyBindingMenuItemId;
	int closeTaskitemItemId;
	int forceKillTaskitemItemId;
	int minimizeTaskitemItemId;
	HWND hSettingsMenuWindow;
	HWND hPluginMenuWindow;
	HWND hWndCtrlMenuWindow;
	vector<TaskItem *> tasks;
	HWND hPrevActiveWindow;
	TaskItem *currentTask;
	HWND hHoldingTaskItem;
	vector<HWND> hSubMenuWindows;
	vector<TrayIconItem *> trayIconItems;
	TaskItemTransparency taskItemTransparency;
	map<HWND, PluginMenuShowCallback> pluginMenuShowTable;
	map<HWND, map<int, PluginMenuItemSelectedCallback> >
		pluginMenuItemSelectedTable;
	map<HWND, WNDPROC> prevWndProcTable;
	map<int, PluginTrayIconItemClickedCallback> pluginTrayIconItemClickedTable;
	map<int, vector<PluginCallback> > pluginCallbackTable;
	map<int, int> keyBindingTable;
	vector<int> dicKeyTable;
	vector<int> fnDicKeyTable;
	map<int, PluginTimerCallback> timerCallbackTable;
	bool isFolded;
	HBRUSH hTaskbarBrush;
	HFONT hNewFont;
	tstring colorProfileName;
	tstring wallpaperName;
	bool isShowPopupsTaskbar;
	bool isShowingMenuItem;
	bool isTaskbarTransparent;
	bool isChangeTaskbarHeight;
	int tapHoldingX;
	int tapHoldingY;
	SEController seController;
};

OpenerMainData *g_Maindata = NULL;

TaskItem::TaskItem() {
	m_hWindow = NULL;
	m_hButton = NULL;
	m_buttonId = 0;
}

TaskItem::~TaskItem() {
}

TrayIconItem::TrayIconItem() {
	m_hControl = NULL;
	m_hIcon = NULL;
}

TrayIconItem::~TrayIconItem() {

}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam) {

	if (msg == g_dicKeyDownMessage) {
		onDicKeyDown(hWnd, wParam);
		return 0;
	}
	else if (msg == g_edPowerOnOffMessage) {
		onEdPowerOnOff(hWnd, wParam);
		return 0;
	}
	else if (msg == g_openerMenuShow) {
		onMenuShow(hWnd, (HWND)wParam);
		return 0;
	}
	else if (msg == g_openerMenuItemSelected) {
		onMenuItemSelected(hWnd, (HWND)wParam, lParam);
		return 0;
	}
	else if (msg == g_openerPluginGetFontInfo) {
		onPluginGetFontInfo(hWnd, (PluginFontInfo *)lParam);
		return 0;
	}
	else if (msg == g_openerPluginGetColor)
		return onPluginGetColor(hWnd, (const TCHAR *)lParam);
	else if (msg == g_openerPluginAppendMenuItem) {
		return onPluginAppendMenuItem(hWnd, (HWND)wParam,
			(PluginMenuItem *)lParam);
	}
	else if (msg == g_openerPluginRemoveMenuItem)
		onPluginRemoveMenuItem(hWnd, (HWND)wParam, lParam);
	else if (msg == g_openerPluginCreateSubMenu)
		return (LRESULT)onPluginCreateSubMenu(hWnd, (PluginMenu *)lParam);
	else if (msg == g_openerPluginClearSubMenu) {
		onPluginClearSubMenu(hWnd, (HWND)wParam);
		return 0;
	}
	else if (msg == g_openerPluginShowPopupMenu) {
		onPluginShowPopupMenu(hWnd, (HWND)wParam, (POINT *)lParam);
		return 0;
	}
	else if (msg == g_openerPluginGetPluginMenu)
		return (LRESULT)onPluginGetMenu(hWnd);
	else if (msg == g_openerPluginRegisterTrayItem) {
		return (LRESULT)onPluginRegisterTrayItem(hWnd, (HMODULE)wParam,
			(PluginTrayItem *)lParam);
	}
	else if (msg == g_openerPluginRegisterTrayIconItem) {
		return (LRESULT)onPluginRegisterTrayIconItem(hWnd, (HMODULE)wParam,
			(PluginTrayIconItem *)lParam);
	}
	else if (msg == g_openerPluginUnregisterTrayIconItem)
		onPluginUnregisterTrayIconItem(hWnd, wParam);
	else if (msg == g_openerPluginGetTrayIconControl)
		return (LRESULT)onPluginGetTrayIconControl(hWnd, wParam);
	else if (msg == g_openerPluginSetTrayIcon) {
		onPluginSetTrayIcon(hWnd, wParam, (HICON)lParam);
		return 0;
	}
	else if (msg == g_openerPluginGetDesktopMenu)
		return (LRESULT)onPluginGetDesktopMenu(hWnd);
	else if (msg == g_openerPluginDesktopToScreen) {
		onPluginDesktopToScreen(hWnd, (POINT *)lParam);
		return 0;
	}
	else if (msg == g_openerPluginRegisterDesktopItem) {
		return onPluginRegisterDesktopItem(hWnd,
			(PluginDesktopItem *)lParam);
	}
	else if (msg == g_openerPluginUnregisterDesktopItem) {
		onPluginUnregisterDesktopItem(hWnd, wParam);
		return 0;
	}
	else if (msg == g_openerPluginGetDesktopItemPlacement) {
		onPluginGetDesktopItemPlacement(hWnd, wParam, (RECT *)lParam);
		return 0;
	}
	else if (msg == g_openerPluginSetDesktopItemPosition) {
		onPluginSetDesktopItemPosition(hWnd, wParam,
			(PluginSetDesktopItemPositionParams *)lParam);
		return 0;
	}
	else if (msg == g_openerPluginRedrawDesktopItem) {
		onPluginRedrawDesktopItem(hWnd, wParam);
		return 0;
	}
	else if (msg == g_openerPluginFocusDesktopItem) {
		onPluginFocusDesktopItem(hWnd, wParam);
		return 0;
	}
	else if (msg == g_openerPluginSetDesktopMouseCapture) {
		onPluginSetDesktopMouseCapture(hWnd, wParam);
		return 0;
	}
	else if (msg == g_openerPluginReleaseDesktopMouseCapture) {
		onPluginReleaseDesktopMouseCapture(hWnd);
		return 0;
	}
	else if (msg == g_openerPluginRegisterKeyBinding)
		return onPluginRegisterKeyBinding(hWnd, (PluginKeyBinding *)lParam);
	else if (msg == g_openerPluginUnregisterKeyBinding) {
		onPluginUnregisterKeyBinding(hWnd, wParam);
		return 0;
	}
	else if (msg == g_openerPluginActivateKeyBinding) {
		onPluginActivateKeyBinding(hWnd, wParam, lParam);
		return 0;
	}
	else if (msg == g_openerPluginSuspendKeyBindings) {
		onPluginSuspendKeyBindings(hWnd);
		return 0;
	}
	else if (msg == g_openerPluginResumeKeyBindings) {
		onPluginResumeKeyBindings(hWnd);
		return 0;
	}
	else if (msg == g_openerPluginRegisterSubTouch)
		return onPluginRegisterSubTouch(hWnd, (PluginSubTouch *)lParam);
	else if (msg == g_openerPluginUnregisterSubTouch) {
		onPluginUnregisterSubTouch(hWnd, wParam);
		return 0;
	}
	else if (msg == g_openerPluginEnableSubTouch) {
		onPluginEnableSubTouch(hWnd, wParam, lParam);
		return 0;
	}
	else if (msg == g_openerPluginHardKeyToDicKey)
		return onPluginHardKeyToDicKey(hWnd, HIWORD(wParam) != 0,
			LOWORD(wParam));
	else if (msg == g_openerPluginRegisterTimer)
		return onPluginRegisterTimer(hWnd, wParam, (PluginTimerCallback)lParam);
	else if (msg == g_openerPluginUnregisterTimer) {
		onPluginUnregisterTimer(hWnd, wParam);
		return 0;
	}
	else if (msg == g_openerPluginRegisterCallback) {
		onPluginRegisterCallback(hWnd, wParam, (PluginCallback)lParam);
		return 0;
	}
	else if (msg == g_openerPluginUnregisterCallback) {
		onPluginUnregisterCallback(hWnd, wParam, (PluginCallback)lParam);
		return 0;
	}

	switch (msg) {
	case WM_CREATE:
		onCreate(hWnd);
		return 0;
	case WM_DESTROY:
		onDestroy(hWnd);
		return 0;
	case WM_CLOSE:
		onClose(hWnd);
		return 0;
	case WM_COMMAND:
	{
		if(!((OpenerMainData*)GetWindowLong(hWnd, GWL_USERDATA))->isShowingMenuItem){
		int id	= LOWORD(wParam);
		int event = HIWORD(wParam);

		if (id >= IDC_TASK_ITEM_BUTTON_BASE)
			onTaskItem(hWnd, id);

		switch (id) {
		case IDC_MAIN_MENU_BUTTON:
			onMainMenu(hWnd);
			break;
		case IDC_FOLDER_BUTTON:
			onFolder(hWnd);
			break;
		}
		}

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
	case WM_CTLCOLORSTATIC:
	{
		HBRUSH hBrush = onCtlColorStatic(hWnd, (HDC)wParam, (HWND)lParam);
		if (hBrush != NULL)
			return (BOOL)hBrush;
		break;
	}
	case WM_DRAWITEM:
		onDrawItem(hWnd, (DRAWITEMSTRUCT *)lParam);
		return 0;
	case WM_TIMER:
		onTimer(hWnd, wParam);
		return 0;
	case WM_HOTKEY:
		onHotKey(hWnd, wParam);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK trayIconControlProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam) {

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(GetParent(hWnd),
		GWL_USERDATA);

	switch (msg) {
	case WM_LBUTTONDOWN:
	{
		int id = GetDlgCtrlID(hWnd);

		if (data->pluginTrayIconItemClickedTable.find(id) !=
			data->pluginTrayIconItemClickedTable.end()) {

			data->pluginTrayIconItemClickedTable[id](GetParent(hWnd), id);
		}

		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT paintStruct;
		HDC hDC = BeginPaint(hWnd, &paintStruct);
		RECT wrect;
		POINT pt;
		GetWindowRect(hWnd, &wrect);
		OpenerDIBSection32 hBackBuf(hDC, wrect.right - wrect.left, wrect.bottom - wrect.top);
		OpenerDIBSection32 hIconBuf(hDC, wrect.right - wrect.left, wrect.bottom - wrect.top);
		OpenerMonoDDB hIconMask(hDC, wrect.right - wrect.left, wrect.bottom - wrect.top);
		pt.x = wrect.left;
		pt.y = g_EdRect.bottom - wrect.bottom;

		HBRUSH hBrush = (HBRUSH)SendMessage(GetParent(hWnd), WM_CTLCOLORSTATIC,
			(WPARAM)hDC, (LPARAM)hWnd);
 
		RECT rect;
		GetClipBox(hBackBuf, &rect);
		FillRect(hBackBuf, &rect, hBrush);

		if(*(data->taskItemTransparency[_T("タスクバー")].second[0]))
			DrawUtil::AlphaBlend(&hBackBuf, data->hDesktopDIB, pt, *(data->taskItemTransparency[_T("タスクバー")].second[0]));

		TrayIconItem *item = (TrayIconItem *)GetWindowLong(hWnd, GWL_USERDATA);

		HICON hIcon = item->getIcon();
		if (hIcon != NULL) {
			if(item->getTransparency()) {
				HBRUSH hMaskBrush = CreateSolidBrush(RGB(254, 0, 254));
				FillRect(hIconBuf, &rect, hMaskBrush);
				DrawIcon(hIconBuf, 0, 0, item->getIcon());

				SetBkColor(hIconBuf, RGB(254, 0, 254));
				BitBlt(hIconMask, 0, 0, rect.right, rect.bottom, hIconBuf, 0, 0, SRCCOPY);
				OpenerDDB hFullMask(hDC, rect.right, rect.bottom);
				BitBlt(hFullMask, 0, 0, rect.right, rect.bottom, hIconMask, 0, 0, SRCCOPY);

				POINT ptIcon;
				ptIcon.x = 0;
				ptIcon.y = 0;
				DrawUtil::AlphaBlend(&hIconBuf, &hBackBuf, ptIcon, item->getTransparency());

				BitBlt(hBackBuf, 0, 0, rect.right, rect.bottom, hFullMask, 0, 0, SRCAND);
				PatBlt(hFullMask, 0, 0, rect.right, rect.bottom, DSTINVERT);

				BitBlt(hIconBuf, 0, 0, rect.right, rect.bottom, hFullMask, 0, 0, SRCAND);
				BitBlt(hBackBuf, 0, 0, rect.right, rect.bottom, hIconBuf, 0, 0, SRCPAINT);

				DeleteObject(hMaskBrush);
			}
			else {
				DrawIcon(hBackBuf, 0, 0, hIcon);
			}
		}

		BitBlt(hDC, 0, 0, hBackBuf.xsize(), hBackBuf.ysize(), hBackBuf, 0, 0, SRCCOPY);

		EndPaint(hWnd, &paintStruct);
		return 0;
	}
	}


	return CallWindowProc(data->prevWndProcTable[hWnd], hWnd, msg, wParam,
		lParam);
}

LRESULT CALLBACK trayItemProc(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam) {

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(GetParent(hWnd),
		GWL_USERDATA);

	TrayIconItem *item = (TrayIconItem *)GetWindowLong(hWnd, GWL_USERDATA);

	switch (msg) {
	case WM_PAINT:
	{
		PAINTSTRUCT paintStruct;
		HDC hDC = BeginPaint(hWnd, &paintStruct);

		switch (item->getType()) {
			case PLUGIN_TRAY_ITEM_BITMAP:{
				BITMAP bminfo;
				RECT rect;
				POINT pt;
				HBITMAP hBmp = (HBITMAP)SNDMSG(hWnd, STM_GETIMAGE, IMAGE_BITMAP, 0);
				GetObject(hBmp, sizeof(BITMAP), &bminfo);

				GetWindowRect(hWnd, &rect);
				OpenerDIBSection32 hBackBuf(hDC, rect.right - rect.left, rect.bottom - rect.top);
				OpenerDIBSection32 hItemBuf(hDC, bminfo.bmWidth, bminfo.bmHeight);
				pt.x = rect.left;
				pt.y = rect.bottom;

				rect.left = rect.top = 0;
				rect.right = hBackBuf.xsize();
				rect.bottom = hBackBuf.ysize();
				FillRect(hBackBuf, &rect, data->hTaskbarBrush);

				SelectObject(hItemBuf, hBmp);
				if(item->getTransparency())
					DrawUtil::AlphaBlend(&hItemBuf, data->hDesktopDIB, pt, item->getTransparency());
				BitBlt(hBackBuf, 0, 0, bminfo.bmWidth, bminfo.bmHeight, hItemBuf, 0, 0, SRCCOPY);

				BitBlt(hDC, 0, 0, hItemBuf.xsize(), hItemBuf.ysize(), hBackBuf, 0, 0, SRCCOPY);
					break;}
			case PLUGIN_TRAY_ITEM_ICON:{
				RECT wrect;
				POINT pt;

				GetWindowRect(hWnd, &wrect);
				OpenerDIBSection32 hIconBuf(hDC, wrect.right - wrect.left, wrect.bottom - wrect.top);
				OpenerDIBSection32 hItemBuf(hDC, wrect.right - wrect.left, wrect.bottom - wrect.top);
				OpenerMonoDDB hIconMask(hDC, wrect.right - wrect.left, wrect.bottom - wrect.top);
				pt.x = wrect.left;
				pt.y = g_EdRect.bottom - wrect.bottom;

				HBRUSH hBrush = (HBRUSH)SendMessage(GetParent(hWnd), WM_CTLCOLORSTATIC,
					(WPARAM)hDC, (LPARAM)hWnd);
 
				RECT rect;
				GetClipBox(hItemBuf, &rect);
				FillRect(hItemBuf, &rect, hBrush);

				HICON hIcon = (HICON)SendMessage(hWnd, STM_GETIMAGE, IMAGE_ICON, 0);
				if(*(data->taskItemTransparency[_T("タスクバー")].second[0])) {
					DrawUtil::AlphaBlend(&hItemBuf, data->hDesktopDIB, pt, *(data->taskItemTransparency[_T("タスクバー")].second[0]));
				}

				if (hIcon != NULL) {
					if(item->getTransparency()) {
						HBRUSH hMaskBrush = CreateSolidBrush(RGB(254, 0, 254));
						FillRect(hIconBuf, &rect, hMaskBrush);
								DrawIcon(hIconBuf, 0, 0, item->getIcon());
						DrawIcon(hIconBuf, 0, 0, hIcon);
						SetBkColor(hIconBuf, RGB(254, 0, 254));
						BitBlt(hIconMask, 0, 0, rect.right, rect.bottom, hIconBuf, 0, 0, SRCCOPY);
						OpenerDDB hFullMask(hDC, rect.right, rect.bottom);
						BitBlt(hFullMask, 0, 0, rect.right, rect.bottom, hIconMask, 0, 0, SRCCOPY);

						POINT ptIcon;
						ptIcon.x = 0;
						ptIcon.y = 0;
						DrawUtil::AlphaBlend(&hIconBuf, &hItemBuf, ptIcon, item->getTransparency());

						BitBlt(hItemBuf, 0, 0, rect.right, rect.bottom, hFullMask, 0, 0, SRCAND);
						PatBlt(hFullMask, 0, 0, rect.right, rect.bottom, DSTINVERT);

						BitBlt(hIconBuf, 0, 0, rect.right, rect.bottom, hFullMask, 0, 0, SRCAND);
						BitBlt(hItemBuf, 0, 0, rect.right, rect.bottom, hIconBuf, 0, 0, SRCPAINT);

						DeleteObject(hMaskBrush);
					}
					else {
						DrawIcon(hItemBuf, 0, 0, hIcon);
					}
				}
				BitBlt(hDC, 0, 0, hItemBuf.xsize(), hItemBuf.ysize(), hItemBuf, 0, 0, SRCCOPY);
				break;}
			case PLUGIN_TRAY_ITEM_TEXT:{
				RECT wrect;
				POINT pt;
				TCHAR lpText[256];

				GetWindowRect(hWnd, &wrect);
				OpenerDIBSection32 hTextBuf(hDC, wrect.right - wrect.left, wrect.bottom - wrect.top);
				OpenerDIBSection32 hItemBuf(hDC, wrect.right - wrect.left, wrect.bottom - wrect.top);
				pt.x = wrect.left;
				pt.y = g_EdRect.bottom - wrect.bottom;

				COLORREF taskbarTextColor = g_colorProfile.getColor(_T("taskbarText"));
				SetTextColor(hItemBuf, taskbarTextColor);
				SetTextColor(hTextBuf, taskbarTextColor);

				COLORREF taskbarColor = g_colorProfile.getColor(_T("taskbar"));
				HBRUSH hBrush = CreateSolidBrush(taskbarColor);
 
				RECT rect;
				GetClipBox(hItemBuf, &rect);
				FillRect(hItemBuf, &rect, hBrush);
				DeleteObject(hBrush);

				GetWindowText(hWnd, lpText, 255);
				if(*(data->taskItemTransparency[_T("タスクバー")].second[0])) {
					DrawUtil::AlphaBlend(&hItemBuf, data->hDesktopDIB, pt, *(data->taskItemTransparency[_T("タスクバー")].second[0]));
				}

				if(item->getTransparency()) {
					HBITMAP hMonoBit = CreateBitmap(rect.right, rect.bottom, 1, 1, NULL);
					HDC hTextMask = CreateCompatibleDC(hDC);
					DeleteObject(SelectObject(hTextMask, hMonoBit));

					COLORREF utxColor = ~taskbarTextColor;
					HBRUSH hMaskBrush = CreateSolidBrush(utxColor);
					FillRect(hTextBuf, &rect, hMaskBrush);
					DeleteObject(hMaskBrush);

					rect.left = rect.top = 0;
					rect.right = hTextBuf.xsize();
					rect.bottom = hTextBuf.ysize();
					SetBkMode(hTextBuf, TRANSPARENT);
					SetBkColor(hItemBuf, utxColor);
					DrawText(hTextBuf, lpText, -1, &rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

					BitBlt(hTextMask, 0, 0, rect.right, rect.bottom, hTextBuf, 0, 0, SRCCOPY);
					OpenerDDB hFullMask(hDC, rect.right, rect.bottom);
					BitBlt(hFullMask, 0, 0, rect.right, rect.bottom, hTextMask, 0, 0, NOTSRCCOPY);

					POINT ptText;
					ptText.x = 0;
					ptText.y = 0;
					DrawUtil::AlphaBlend(&hTextBuf, &hItemBuf, ptText, item->getTransparency());

					BitBlt(hItemBuf, 0, 0, rect.right, rect.bottom, hFullMask, 0, 0, SRCAND);
					PatBlt(hFullMask, 0, 0, rect.right, rect.bottom, DSTINVERT);

					BitBlt(hTextBuf, 0, 0, rect.right, rect.bottom, hFullMask, 0, 0, SRCAND);
					BitBlt(hItemBuf, 0, 0, rect.right, rect.bottom, hTextBuf, 0, 0, SRCPAINT);

					DeleteObject(hMaskBrush);
					DeleteObject(hMonoBit);
					DeleteDC(hTextMask);
				}
				else{
					SetBkMode(hItemBuf, TRANSPARENT);
					DrawText(hItemBuf, lpText, -1, &rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
				}
				BitBlt(hDC, 0, 0, hItemBuf.xsize(), hItemBuf.ysize(), hItemBuf, 0, 0, SRCCOPY);
				break;}
		}

		EndPaint(hWnd, &paintStruct);

		return 0;
	}
	}

	return CallWindowProc(data->prevWndProcTable[hWnd], hWnd, msg, wParam,
		lParam);
}


LRESULT CALLBACK taskItemProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(GetParent(hWnd),
		GWL_USERDATA);

	WNDPROC lpfnWndProc = data->prevWndProcTable[hWnd];

	switch (msg) {
	case WM_LBUTTONDOWN:
	{
		POINT tpt;
		tpt.x = LOWORD(lParam);
		tpt.y = HIWORD(lParam);
		ClientToScreen(hWnd, &tpt);
		data->tapHoldingX = tpt.x;
		data->tapHoldingY = tpt.y;

		break;
	}
	}

	return CallWindowProc(lpfnWndProc, hWnd, msg, wParam,
		lParam);
}

static LRESULT CALLBACK menuSubWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	OpenerMainData *data = g_Maindata;
	RECT irc;

	WNDPROC lpfnWndProc = data->prevWndProcTable[hWnd];

	switch (msg) {
		case WM_SHOWWINDOW:
			break;
	case WM_ACTIVATE:
	{
		if((wParam & 0xFFFF) == WA_INACTIVE){
			data->isShowingMenuItem = false;
			SetTimer(data->hTaskbarWindow, TIMER_MAIN, 500, NULL);
			GetWindowRect(hWnd, &irc);
			InvalidateRect(data->hHoldingTaskItem, NULL, true);
			changeWindowLayout(data->hTaskbarWindow);
		}
		else {
			if(wParam)
				data->seController.playSE(_T("メニュー展開"));
		}
		break;
	}
	}

	return CallWindowProc(lpfnWndProc, hWnd, msg, wParam,
		lParam);
}

static BOOL CALLBACK showDesktopProc(HWND hWnd, LPARAM lParam) {
	OpenerMainData *data = (OpenerMainData*)lParam;

	if((hWnd != data->hTaskbarWindow) && (hWnd != data->hDesktopWindow))
		ShowWindow(hWnd, SW_MINIMIZE);

	return TRUE;
}

FILE *g_logFile = NULL;

void writeLog(const tstring &line) {
	tstring lineWithNl = line + _T("\n");

	char mbLineBuf[1024 * 2];
	WideCharToMultiByte(932, 0, lineWithNl.c_str(), -1, mbLineBuf,
		sizeof(mbLineBuf), NULL, NULL);

	fputs(mbLineBuf, g_logFile);
	fflush(g_logFile);
}

static void onCreate(HWND hWnd) {
	OpenerMainData *data = new OpenerMainData();
	SetWindowLong(hWnd, GWL_USERDATA, (long)data);
	g_Maindata = data;

	data->hTaskbarWindow = hWnd;
	data->hMainMenuWindow = NULL;
	data->hPrevActiveWindow = NULL;
	data->currentTask = NULL;
	data->isFolded = false;
	data->isShowPopupsTaskbar = false;

	InitCommonControls();

	tstring logFileName = KnceUtil::getCurrentDirectory() + _T("\\ceopener.log");
	g_logFile = _tfopen(logFileName.c_str(), _T("w"));

	writeLog(_T("*** ceOpener startup process ***"));

	HWND hWaitBox = knceCreateWaitBox(hWnd, _T(""));

	writeLog(_T("Prepare the back screen..."));

	HDC hdc = GetDC(hWnd);
	data->hDesktopDIB = new OpenerDIBSection32(hdc, g_EdRect.right, g_EdRect.bottom);
	ReleaseDC(hWnd, hdc);

	writeLog(_T("Initialize ED tools..."));

	if (brainApisEnabled()) {
		Res_Open_win(_T("Nand\\EDNewRes_Color.bin"));
		Font_Open_win(_T("Nand\\EDNewFont.bin"));

		SHDicToolsInit(hWnd);
	}

	tstring curDir = KnceUtil::getCurrentDirectory();
	tstring pluginDir = curDir + _T("\\plugins");

	data->pluginManager.setMainWindow(hWnd);
	data->pluginManager.setDirectory(pluginDir);

	TCHAR pathCstr[MAX_PATH];
	GetModuleFileName(NULL,pathCstr,MAX_PATH);

	writeLog(_T("Load localize patch (if any)..."));

	tstring localizeFileName = pluginDir + _T("\\localizepatch.dll");
	data->pluginManager.loadPlugin(localizeFileName);
	data->pluginManager.addExcludedFileName(localizeFileName);

	writeLog(_T("Launch font loader..."));

	launchFontLoader();

	writeLog(_T("Setup color profile..."));

	setupColorProfile();

	tstring persistDir = _T("\\Nand2\\.ceopener");
	tstring propFileName = persistDir + _T("\\ceopener.dat");

	bool propLoaded = false;
	map<tstring, tstring> props;

	writeLog(_T("Read property file..."));

	if (GetFileAttributes(propFileName.c_str()) != -1) {
		KnceUtil::readPropertyFile(props, propFileName);
		propLoaded = true;
	}

	if(propLoaded) {
		data->isChangeTaskbarHeight =
			_ttoi(props[_T("global.changeTaskbarHeight")].c_str()) != 0;
			//props[_T("global.changeTaskbarHeight")].c_str() == _T("1");
		if(data->isChangeTaskbarHeight) {
			float tskb = 0;
			if (g_EdRect.bottom < g_EdRect.right) {
				tskb = (float)g_EdRect.bottom * 24 / 320;
			} else {
				tskb = (float)g_EdRect.right * 24 / 320;
			}
			TASKBAR_HEIGHT = (int)tskb;
		}
	}
	else
		data->isChangeTaskbarHeight = false;

	writeLog(_T("Get Item Transparent state..."));

	if(propLoaded){
		TCHAR modName[MAX_PATH];
		LPTSTR endCode = _T("\0");
		map<tstring, tstring>::iterator iter = props.begin();

		for(; iter != props.end(); iter++) {
			if(_tcsncmp(iter->first.c_str(), _T("titems."), 7) == 0) {
				TCHAR *p = (TCHAR*)(iter->first.c_str() + 7);
				_tcscpy(modName, p);
				TCHAR *q = _tcschr(modName, _T('.'));
				*q = _T('\0');
				p += lstrlen(modName) + 1;
				int uID = _tcstol(p, &endCode, 10);
				UINT *transparency = new UINT;
				*transparency = _tcstol(iter->second.c_str(), &endCode, 10);
				data->taskItemTransparency[tstring(modName)].second[uID] = transparency;
			}
		}
		if (data->taskItemTransparency.find_ts(_T("タスクバー")) == data->taskItemTransparency.end()) {
			UINT *transparency = new UINT;
			*transparency = DrawUtil::DRAW_RATIO_100;
			data->taskItemTransparency[tstring(_T("タスクバー"))].second[0] = transparency;
		}
		if (data->taskItemTransparency.find_ts(_T("テキスト")) == data->taskItemTransparency.end()) {
			UINT *transparency = new UINT;
			*transparency = DrawUtil::DRAW_RATIO_100;
			data->taskItemTransparency[tstring(_T("テキスト"))].second[0] = transparency;
		}
		if (data->taskItemTransparency.find_ts(_T("スタートボタン")) == data->taskItemTransparency.end()) {
			UINT *transparency = new UINT;
			*transparency = DrawUtil::DRAW_RATIO_100;
			data->taskItemTransparency[tstring(_T("スタートボタン"))].second[0] = transparency;
		}
	}
	else {
		UINT *transparency = new UINT;
		*transparency = DrawUtil::DRAW_RATIO_100;
		data->taskItemTransparency[tstring(_T("タスクバー"))].second[0] = transparency;
		UINT *transparency2 = new UINT;
		*transparency2 = DrawUtil::DRAW_RATIO_100;
		data->taskItemTransparency[tstring(_T("テキスト"))].second[0] = transparency2;
		UINT *transparency3 = new UINT;
		*transparency3 = DrawUtil::DRAW_RATIO_100;
		data->taskItemTransparency[tstring(_T("スタートボタン"))].second[0] = transparency3;
	}

	writeLog(_T("Prepare sound effects..."));

	tstring sndDir = KnceUtil::getCurrentDirectory() + _T("\\sounds");

	LPTSTR verbs[] = {
		_T("起動"),
		_T("終了"),
		_T("ウィンドウ展開"),
		_T("ウィンドウ最小化"),
		_T("メニュー展開"),
		_T("メニュークリック")
	};

	LPTSTR defFiles[] = {
		_T("\\start.wav"),
		_T("\\end.wav"),
		_T("\\wndexpand.wav"),
		_T("\\wndminimize.wav"),
		_T("\\menuexpand.wav"),
		_T("\\menuclick.wav")
	};

	int verbCount = sizeof(verbs) / sizeof(verbs[0]);

	if(propLoaded) {
		for(int i = 0; i < verbCount; i++) {
			if(props.find(tstring(_T("sndEffect.fileName.")) + verbs[i]) == props.end())
				data->seController[verbs[i]] = sndDir + defFiles[i];
			else {
				data->seController[verbs[i]] = props[tstring(_T("sndEffect.fileName.")) + verbs[i]];
				data->seController[verbs[i]].setEnabled(
					props[tstring(_T("sndEffect.enabled.")) + verbs[i]] == _T("1") ? true : false);
			}
		}
	}
	else {
		for(int i = 0; i < verbCount; i++)
			data->seController[tstring(verbs[i])] = sndDir + tstring(defFiles[i]);
		data->seController[_T("起動")].setEnabled(true);
	}

	data->seController[_T("終了")].setPlayMode(SND_ASYNC | SND_FILENAME);
	
	writeLog(_T("Prepare fonts and color profile..."));

	if (propLoaded) {
		tstring fontName = props[_T("global.fontName")];
		int pointSize = _ttoi(props[_T("global.fontSize")].c_str());

		g_hMainFont = KnceUtil::createFont(fontName, pointSize, false, false);
		data->hNewFont = KnceUtil::createFont(fontName, pointSize, false,
			false);

		data->colorProfileName = props[_T("global.colorProfileName")];
		g_colorProfile.loadProfile(data->colorProfileName);
	}
	else {
		g_hMainFont = KnceUtil::createFont(_T(""), 90, false, false);
		data->hNewFont = KnceUtil::createFont(_T(""), 90, false, false);
	}

	writeLog(_T("Bring ce desktop to front..."));

	HWND hCeDesktopWindow =
		FindWindow(_T("DesktopExplorerWindow"), NULL);
	SetForegroundWindow(hCeDesktopWindow);

	writeLog(_T("Create desktop..."));

	data->hDesktopWindow = createDesktop(hWnd);

	writeLog(_T("Bring ce desktop to front (2nd)..."));

	SetForegroundWindow(hCeDesktopWindow);  // again

	MenuParams menuParams;
	menuParams.setMain(true);

	writeLog(_T("Create menus..."));

	data->hMainMenuWindow = createMenu(hWnd, menuParams);

	data->exitMenuItemId = appendMenuItem(data->hMainMenuWindow,
		_T("ceOpenerの終了"), NULL);
	data->closeEdMenuItemId = appendMenuItem(data->hMainMenuWindow,
		_T("辞書アプリを閉じる"), NULL);

	MenuParams subMenuParams;
	data->hSettingsMenuWindow = createMenu(hWnd, subMenuParams);

	data->settingsMenuItemId = appendMenuItem(data->hMainMenuWindow,
		_T("設定"), data->hSettingsMenuWindow);
	data->globalSettingsMenuItemId = appendMenuItem(data->hSettingsMenuWindow,
		_T("ceOpener の設定..."), NULL);
	data->keyBindingMenuItemId = appendMenuItem(data->hSettingsMenuWindow,
		_T("キーの割り当て..."), NULL);

	data->hPluginMenuWindow = createMenu(hWnd, subMenuParams);

	data->pluginsMenuItemId = appendMenuItem(data->hMainMenuWindow,
		_T("プラグイン"), data->hPluginMenuWindow);

	data->hWndCtrlMenuWindow = createMenu(hWnd, subMenuParams);
	data->prevWndProcTable[data->hWndCtrlMenuWindow] = (WNDPROC)GetWindowLong(data->hWndCtrlMenuWindow, GWL_WNDPROC);
	SetWindowLong(data->hWndCtrlMenuWindow, GWL_WNDPROC, (LONG)menuSubWindowProc);

	data->forceKillTaskitemItemId = appendMenuItem(data->hWndCtrlMenuWindow,
		_T("強制終了"), NULL);
	data->minimizeTaskitemItemId = appendMenuItem(data->hWndCtrlMenuWindow,
		_T("最小化"), NULL);
	data->closeTaskitemItemId = appendMenuItem(data->hWndCtrlMenuWindow,
		_T("閉じる"), NULL);

	data->seController.playSE(_T("起動"));

	writeLog(_T("Create buttons..."));

	SendMessage(hWnd, WM_SETFONT, (WPARAM)g_hMainFont, 0);

	HWND hMenuButton = CreateWindow(_T("BUTTON"), _T("スタート"),
		BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
		0, 1, MAIN_MENU_BUTTON_WIDTH, TASKBAR_HEIGHT - 2, hWnd,
		(HMENU)IDC_MAIN_MENU_BUTTON, g_hInstance, NULL);

	SendMessage(hMenuButton, WM_SETFONT, (WPARAM)g_hMainFont, 0);

	HWND hFolderButton = CreateWindow(_T("BUTTON"), _T(""),
		BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
		0, 1, FOLDER_BUTTON_WIDTH, TASKBAR_HEIGHT - 2, hWnd,
		(HMENU)IDC_FOLDER_BUTTON, g_hInstance, NULL);

	data->keyBinder.setOwnerWindow(hWnd);

	writeLog(_T("Initialize sub touch..."));

	data->subTouchManager.setOwnerWindow(hWnd);
	data->subTouchManager.init();

	writeLog(_T("Setup wallpaper and properties of key binder..."));

	if (propLoaded) {
		data->wallpaperName = props[_T("global.wallpaperName")];
		setDesktopWallpaper(data->hDesktopWindow, data->wallpaperName, data->hDesktopDIB);

		data->isShowPopupsTaskbar =
			_ttoi(props[_T("global.showPopupsTaskbar")].c_str()) != 0;

		data->keyBinder.loadProperties(props);
	}

	COLORREF taskbarColor = g_colorProfile.getColor(_T("taskbar"));
	data->hTaskbarBrush = CreateSolidBrush(taskbarColor);

	writeLog(_T("Load builtin plugins..."));

	data->pluginManager.loadPlugin(curDir + _T("\\openerlauncher.dll"));
	tstring pluginName = curDir + tstring(_T("\\openerclock.dll"));
	data->pluginManager.loadPlugin(pluginName);
	data->pluginManager.loadPlugin(curDir + tstring(_T("\\openervolume.dll")));
	data->pluginManager.loadPlugin(curDir + tstring(_T("\\desktopitemmgr.dll")));

	writeLog(_T("Load all plugins..."));

	data->pluginManager.loadAllPlugins();

	writeLog(_T("Setup default key bindings..."));

	setupKeyBindings(hWnd);

	writeLog(_T("Change window layout..."));

	changeWindowLayout(hWnd);

	knceDestroyWaitBox(hWaitBox);

	writeLog(_T("Setup initial work area..."));

	// setup initial work area
	fold(hWnd, false);

	writeLog(_T("Bring taskbar to front..."));

	SetForegroundWindow(hWnd);

	writeLog(_T("Setup dic key table..."));

	// after setting foreground window
	setupDicKeyTable(hWnd);

	writeLog(_T("Update taskbar..."));

	onTimer(hWnd, TIMER_MAIN);
	SetTimer(hWnd, TIMER_MAIN, 500, NULL);

	writeLog(_T("Startup process has finished."));
}

static void onDestroy(HWND hWnd) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->seController.playSE(_T("終了"));

	writeLog(_T("*** ceOpener shutdown process ***"));

	HWND hWaitBox = knceCreateWaitBox(hWnd, _T(""));

	DeleteObject(data->hTaskbarBrush);
	//DeleteObject(g_hBackDC);

	TaskItemTransparencyVector::iterator tItemIter = data->taskItemTransparency.begin();
	for(; tItemIter != data->taskItemTransparency.end(); tItemIter++){
		map<UINT, UINT*>::iterator subIter = tItemIter->second.begin();
		for(; subIter != tItemIter->second.end(); subIter++)
			delete subIter->second;
	}

	writeLog(_T("Unload all plugins..."));

	data->pluginManager.unloadAllPlugins();

	int numTasks = data->tasks.size();
	for (i = 0; i < numTasks; i++)
		delete data->tasks[i];

	writeLog(_T("Destroy sub menus..."));

	int numSubMenus = data->hSubMenuWindows.size();
	for (i = 0; i < numSubMenus; i++)
		destroyMenu(data->hSubMenuWindows[i]);

	writeLog(_T("Destroy desktop and main menus..."));

	destroyDesktop(data->hDesktopWindow);
	destroyMenu(data->hMainMenuWindow);
	destroyMenu(data->hWndCtrlMenuWindow);

	writeLog(_T("Destroy tray items..."));

	int numTrayItems = data->trayIconItems.size();
	for (i = 0; i < numTrayItems; i++)
		delete data->trayIconItems[i];

	writeLog(_T("Restore work area..."));

	RECT desktopRect;
	desktopRect = g_EdRect; 

	SystemParametersInfo(SPI_SETWORKAREA, 0, &desktopRect, SPIF_SENDCHANGE);

	DeleteObject(g_hMainFont);

	writeLog(_T("Unload localize patch (if any)..."));

	tstring pluginDir = KnceUtil::getCurrentDirectory() + _T("\\plugins");
	data->pluginManager.unloadPlugin(pluginDir + _T("\\localizepatch.dll"));

	writeLog(_T("Release ED tools..."));

	if (brainApisEnabled()) {
		Res_Close_win();
		Font_Close_win();
	}

	knceDestroyWaitBox(hWaitBox);

	writeLog(_T("Bring ED app to front..."));

	HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);
	if (hEdWindow != NULL)
		SetForegroundWindow(hEdWindow);

	writeLog(_T("Send power on message to ED..."));

	sendPowerOnMessageToEd();

	writeLog(_T("Shutdown process has finished."));

	PostQuitMessage(0);

	delete data;
}

static void onClose(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	vector<HWND> windows;
	if (data->isShowPopupsTaskbar)
		EnumWindows(findWindowProc, (LPARAM)&windows);
	else
		EnumWindows(findWindowProcNoPopup, (LPARAM)&windows);

	bool hasEdWindow = false;
	HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);

	if (hEdWindow != NULL) {
		if (find(windows.begin(), windows.end(), hEdWindow) != windows.end())
			hasEdWindow = true;
	}

	int numApps = windows.size() - hasEdWindow ? 1 : 0;
	if (numApps > 0) {
		if (MessageBox(hWnd, _T("ほかのアプリケーションが起動中です。\n")
			_T("終了してもよろしいですか?"), _T("確認"),
			MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) == IDNO) {

			return;
		}
	}

	DestroyWindow(hWnd);

	return;
}

static void onTaskItem(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);
	if(data->isShowingMenuItem)
		return;

	HWND hItemWindow = GetDlgItem(hWnd, id);
	TaskItem *task = (TaskItem *)GetWindowLong(hItemWindow, GWL_USERDATA);
	HWND hTargetWindow = task->getWindow();

	if (hTargetWindow == GetForegroundWindow()) {
		SetWindowPos(hTargetWindow, HWND_BOTTOM, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
		data->seController.playSE(_T("ウィンドウ最小化"));
		SetForegroundWindow(hWnd);
		switchTask(hWnd, NULL, hWnd);
	}
	else {
		data->seController.playSE(_T("ウィンドウ展開"));
		SetForegroundWindow(hTargetWindow);
		switchTask(hWnd, task);
	}
}

static void onMainMenu(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (IsWindowVisible(data->hMainMenuWindow))
		hideMenu(data->hMainMenuWindow);
	else {
		data->seController.playSE(_T("メニュー展開"));
		showMenu(data->hMainMenuWindow);
	}
}

static void onFolder(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	fold(hWnd, !data->isFolded);

	changeWindowLayout(hWnd);
}

static void onPaint(HWND hWnd, HDC hDC) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);
	RECT rect;
	GetWindowRect(hWnd, &rect);
	OpenerDIBSection32 hBackBuf(hDC, rect.right - rect.left, rect.bottom - rect.top);

	COLORREF taskbarColor = g_colorProfile.getColor(_T("taskbar"));

	RECT desktopRect;
	desktopRect = g_EdRect;

	HBRUSH hBrush = CreateSolidBrush(taskbarColor);
	HBRUSH hPrevBrush = (HBRUSH)SelectObject(hBackBuf, hBrush);

	PatBlt(hBackBuf, 0, 0, desktopRect.right, TASKBAR_HEIGHT, PATCOPY);

	SelectObject(hBackBuf, hPrevBrush);
	DeleteObject(hBrush);

	HPEN hPen1 = CreatePen(PS_SOLID, 1, DrawUtil::adjustLightness24(
		taskbarColor, 0.2));
	HPEN hPrevPen = (HPEN)SelectObject(hBackBuf, hPen1);

	MoveToEx(hBackBuf, 0, 0, NULL);
	LineTo(hBackBuf, desktopRect.right - 1, 0);

	HPEN hPen2 = CreatePen(PS_SOLID, 1, DrawUtil::adjustLightness24(
		taskbarColor, 0.1));
	SelectObject(hBackBuf, hPen2);

	MoveToEx(hBackBuf, 0, 1, NULL);
	LineTo(hBackBuf, desktopRect.right - 1, 1);

	SelectObject(hBackBuf, hPrevPen);
	DeleteObject(hPen1);
	DeleteObject(hPen2);

	int transparency = *(data->taskItemTransparency[tstring(_T("タスクバー"))].second[0]);
	if(transparency) {
		POINT pt = { 0, 0 };		

		DrawUtil::AlphaBlend(&hBackBuf, data->hDesktopDIB, pt, transparency);
	}
	
	BitBlt(hDC, 0, 0, hBackBuf.xsize(), hBackBuf.ysize(), hBackBuf, 0, 0, SRCCOPY);
}

static HBRUSH onCtlColorStatic(HWND hWnd, HDC hDC, HWND hStatic) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	COLORREF taskbarColor = g_colorProfile.getColor(_T("taskbar"));
	COLORREF taskbarTextColor = g_colorProfile.getColor(_T("taskbarText"));

	SetTextColor(hDC, taskbarTextColor);
	SetBkColor(hDC, taskbarColor);

	return data->hTaskbarBrush;
}

static void onDrawItem(HWND hWnd, DRAWITEMSTRUCT *drawItem) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	COLORREF taskbarColor = g_colorProfile.getColor(_T("taskbar"));
	COLORREF taskbarTextColor = g_colorProfile.getColor(_T("taskbarText"));
	COLORREF activeTaskItemColor =
		g_colorProfile.getColor(_T("activeTaskItem"));
	COLORREF activeTaskItemTextColor =
		g_colorProfile.getColor(_T("activeTaskItemText"));
	COLORREF menuButtonColor = g_colorProfile.getColor(_T("mainMenuButton"));
	COLORREF menuButtonTextColor =
		g_colorProfile.getColor(_T("mainMenuButtonText"));

	HDC hItemDC = drawItem->hDC;
	HWND hItemWindow = drawItem->hwndItem;
	int itemId = drawItem->CtlID;
	RECT &itemRect = drawItem->rcItem;

	bool pushed = (drawItem->itemState & ODS_SELECTED) != 0;

	TCHAR buf[256];
	GetWindowText(hItemWindow, buf, 256);

	if (itemId == IDC_MAIN_MENU_BUTTON) {

		if(*(data->taskItemTransparency[_T("スタートボタン")].second[0]) || *(data->taskItemTransparency[_T("テキスト")].second[0])){
			DrawUtil::drawButton((HWND)drawItem->hwndItem, hItemDC, itemRect, menuButtonColor, pushed,
				data->hDesktopDIB, buf, menuButtonTextColor, DrawUtil::DRAW_BUTTON_MAIN, *(data->taskItemTransparency[_T("スタートボタン")].second[0]), *(data->taskItemTransparency[_T("テキスト")].second[0]));
		}
		else{
			DrawUtil::drawButton(hItemDC, itemRect, menuButtonColor, pushed);

			COLORREF prevTextColor = SetTextColor(hItemDC, menuButtonTextColor);
			int prevBkMode = SetBkMode(hItemDC, TRANSPARENT);

			RECT captionRect = itemRect;
			InflateRect(&captionRect, -5, -1);
			DrawText(hItemDC, buf, -1, &captionRect, DT_CENTER | DT_VCENTER |
				DT_SINGLELINE);

			OffsetRect(&captionRect, 1, 0);
			DrawText(hItemDC, buf, -1, &captionRect, DT_CENTER | DT_VCENTER |
				DT_SINGLELINE);

			SetTextColor(hItemDC, prevTextColor);
			SetBkMode(hItemDC, prevBkMode);
		}
	}
	else if (itemId == IDC_FOLDER_BUTTON) {
		if(*(data->taskItemTransparency[_T("タスクバー")].second[0]) || *(data->taskItemTransparency[_T("テキスト")].second[0])){
			if (data->isFolded) {
				DrawUtil::drawButton(drawItem->hwndItem, hItemDC, itemRect,taskbarColor, false, data->hDesktopDIB, 
					(LPTSTR)DrawUtil::DRAW_TRIANGLE_WEST, taskbarTextColor, DrawUtil::DRAW_BUTTON_FOLD, *(data->taskItemTransparency[_T("タスクバー")].second[0]), *(data->taskItemTransparency[_T("テキスト")].second[0]));
			}
			else {
				DrawUtil::drawButton(drawItem->hwndItem, hItemDC, itemRect,taskbarColor, false, data->hDesktopDIB,
					(LPTSTR)DrawUtil::DRAW_TRIANGLE_EAST, taskbarTextColor, DrawUtil::DRAW_BUTTON_FOLD, *(data->taskItemTransparency[_T("タスクバー")].second[0]), *(data->taskItemTransparency[_T("テキスト")].second[0]));
			}
		}
		else{
			DrawUtil::drawButton(hItemDC, itemRect, taskbarColor, false);

			POINT center = {(itemRect.left + itemRect.right) / 2,
				(itemRect.top + itemRect.bottom) / 2};

			if (data->isFolded) {
				DrawUtil::drawTriangle(hItemDC, center,
					DrawUtil::DRAW_TRIANGLE_WEST, taskbarTextColor);
			}
			else {
				DrawUtil::drawTriangle(hItemDC, center,
					DrawUtil::DRAW_TRIANGLE_EAST, taskbarTextColor);
			}
		}
	}
	else if (itemId >= IDC_TASK_ITEM_BUTTON_BASE) {
		TaskItem *task = (TaskItem *)GetWindowLong(hItemWindow,
			GWL_USERDATA);
		bool active = task == data->currentTask;

		if(data->isShowingMenuItem){
			return;
		}
		else if(pushed == TRUE){
			//Start holding
			data->hHoldingTaskItem = hItemWindow;
			SetTimer(hWnd, TIMER_HOLDING, TAP_HOLD_TIME, NULL);
		}
		else{
			data->hHoldingTaskItem = NULL;
			KillTimer(hWnd, TIMER_HOLDING);
		}

		if(*(data->taskItemTransparency[_T("タスクバー")].second[0]) || *(data->taskItemTransparency[_T("テキスト")].second[0])) {
			DrawUtil::drawButton(drawItem->hwndItem, hItemDC, itemRect, active ?
				activeTaskItemColor : taskbarColor, pushed || active, data->hDesktopDIB, 
				  buf, active ? activeTaskItemTextColor : taskbarTextColor, DrawUtil::DRAW_BUTTON_TASKITEM, 
				  *(data->taskItemTransparency[_T("タスクバー")].second[0]), *(data->taskItemTransparency[_T("テキスト")].second[0]));
		}
		else {
			DrawUtil::drawButton(hItemDC, itemRect, active ?
				activeTaskItemColor : taskbarColor, pushed || active);

			COLORREF prevTextColor = SetTextColor(hItemDC, active ?
				activeTaskItemTextColor : taskbarTextColor);
			int prevBkMode = SetBkMode(hItemDC, TRANSPARENT);

			RECT captionRect = itemRect;
			InflateRect(&captionRect, -4, 0);
			DrawText(hItemDC, buf, -1, &captionRect, DT_LEFT | DT_VCENTER |
				DT_SINGLELINE);

			SetTextColor(hItemDC, prevTextColor);
			SetBkMode(hItemDC, prevBkMode);
		}
	}
}

static void onTimer(HWND hWnd, int idEvent) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (idEvent == TIMER_MAIN && data->isShowingMenuItem == FALSE) {
		updateTasks(hWnd);

		HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);

		HWND hActiveWindow = GetForegroundWindow();
		if (hActiveWindow != data->hPrevActiveWindow) {
			if (hActiveWindow == hEdWindow) {
				data->subTouchManager.suspend();

				sendPowerOnMessageToEd();
			}
			else
				data->subTouchManager.resume();

			PluginCallbackTaskSwitchedParams params;
			params.hWindowFrom = data->hPrevActiveWindow;
			params.hWindowTo = hActiveWindow;

			vector<PluginCallback> &callbacks =
				data->pluginCallbackTable[PLUGIN_CALLBACK_TASK_SWITCHED];
			int numCallbacks = callbacks.size();

			for (i = 0; i < numCallbacks; i++)
				callbacks[i](hWnd, (unsigned long)&params);

			data->hPrevActiveWindow = hActiveWindow;
		}

		// make PC connect window always on top
		HWND hEdConnWindow = FindWindow(_T("SHDIC_PCCNCT_C"), NULL);
		if (hEdConnWindow != NULL) {
			SetWindowPos(hEdConnWindow, HWND_TOPMOST, 0, 0, 0, 0,
				SWP_NOSIZE | SWP_NOMOVE);
		}
	}
	else if (idEvent == TIMER_POWER_ON) {
		data->subTouchManager.processPowerOnOff(SubTouchManager::POWER_ON);

		KillTimer(hWnd, TIMER_POWER_ON);
	}
	else if(idEvent == TIMER_HOLDING){
		RECT rc;

		data->isShowingMenuItem = TRUE;
		KillTimer(hWnd, TIMER_HOLDING);

		rc.left = rc.right = data->tapHoldingX;
		rc.top = rc.bottom = data->tapHoldingY;
		setMenuAttachedRect(data->hWndCtrlMenuWindow, &rc);

		showMenu(data->hWndCtrlMenuWindow);
	}
	else {
		if (data->timerCallbackTable.find(idEvent) !=
			data->timerCallbackTable.end()) {

			PluginTimerCallback callback = data->timerCallbackTable[idEvent];
			if (callback != NULL)
				callback(hWnd, idEvent);
		}
	}
}

static void onHotKey(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->keyBinder.processHotKey(id);
}

static void onDicKeyDown(HWND hWnd, int dicKey) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->subTouchManager.processDicKeyDown(dicKey);
}

static void onEdPowerOnOff(HWND hWnd, int status) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (status == 1){
		data->subTouchManager.processPowerOnOff(SubTouchManager::POWER_OFF);
		data->isShowingMenuItem = false;
		hideMenu(data->hWndCtrlMenuWindow);
		SetTimer(hWnd, TIMER_MAIN, 500, NULL);
	}
	else
		SetTimer(hWnd, TIMER_POWER_ON, 250, NULL);

	vector<PluginCallback> &callbacks =
		data->pluginCallbackTable[PLUGIN_CALLBACK_POWER_ON_OFF];
	int numCallbacks = callbacks.size();

	for (i = 0; i < numCallbacks; i++)
		callbacks[i](hWnd, status);
}

static void onMenuShow(HWND hWnd, HWND hMenu) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (hMenu == data->hMainMenuWindow) {
		tstring caption;
		HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);

		if (hEdWindow == NULL)
			caption = _T("辞書アプリを開く");
		else
			caption = _T("辞書アプリを閉じる");

		setMenuItemCaption(hMenu, data->closeEdMenuItemId, caption);
	}
	else {
		map<HWND, PluginMenuShowCallback>::const_iterator iter =
			data->pluginMenuShowTable.find(hMenu);

		if (iter != data->pluginMenuShowTable.end())
			iter->second(hWnd, hMenu);
	}
}

static void onMenuItemSelected(HWND hWnd, HWND hMenu, int id) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->seController.playSE(_T("メニュークリック"));

	bool processed = false;
	if (hMenu == data->hMainMenuWindow) {
		// [Exit ceOpener]
		if (id == data->exitMenuItemId) {
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			processed = true;
		}

		// [Close/Open Dictionary App]
		else if (id == data->closeEdMenuItemId) {
			HWND hWaitBox = knceCreateWaitBox(hWnd, _T(""));

			data->subTouchManager.suspend();

			HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);
			bool isOpen = hEdWindow != NULL;

			vector<PluginCallback> &callbacks =
				data->pluginCallbackTable[PLUGIN_CALLBACK_ED_CLOSED_OPENED];
			int numCallbacks = callbacks.size();

			for (i = 0; i < numCallbacks; i++) {
				callbacks[i](hWnd, isOpen ? PLUGIN_ED_BEFORE_CLOSED :
					PLUGIN_ED_BEFORE_OPENED);
			}

			Sleep(1000);

			if (isOpen) {
				//確認画面開始(たぶん完成)
				if (MessageBox(hWnd, _T("辞書アプリを終了します。\n")
					_T("よろしいですか?"), _T("確認"),
					MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {

					SendMessage(hEdWindow, WM_CLOSE, NULL, NULL);

					Sleep(1000);
				}
				//確認画面終了
			}
			else if (hEdWindow == NULL) {
				//確認画面開始(たぶん完成)
				if (MessageBox(hWnd, _T("辞書アプリを開始します。\n")
					_T("よろしいですか?"), _T("確認"),
					MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) == IDYES) {

					BOOL bSuccess =CreateProcess(_T("\\NAND\\wceprj.exe"), _T("1"), NULL, NULL,
						false, 0, NULL, NULL, NULL, NULL);
					if(!bSuccess && GetLastError() == ERROR_FILE_NOT_FOUND)
						bSuccess = CreateProcess(_T("\\NAND\\wceprj.exe"), _T("1"), NULL, NULL,
						false, 0, NULL, NULL, NULL, NULL);
					if(!bSuccess)
						MessageBox(hWnd, _T("辞書アプリを起動できませんでした。"), _T("エラー"), MB_OK | MB_ICONEXCLAMATION);
					else
						Sleep(10000);
				}
				//確認画面終了
			}

			for (i = 0; i < numCallbacks; i++) {
				callbacks[i](hWnd, isOpen ? PLUGIN_ED_AFTER_CLOSED :
					PLUGIN_ED_AFTER_OPENED);
			}

			knceDestroyWaitBox(hWaitBox);
		}
	}
	else if (hMenu == data->hSettingsMenuWindow) {
		// [Global Settings...]
		if (id == data->globalSettingsMenuItemId) {
			showSettingDialog(hWnd);
			processed = true;
		}

		// [Key Binding...]
		else if (id == data->keyBindingMenuItemId) {
			showKeyBindingDialog(hWnd);
			processed = true;
		}
	}
	else if (hMenu == data->hWndCtrlMenuWindow) {
		TaskItem *pHoldingTask = (TaskItem*)GetWindowLong(data->hHoldingTaskItem, GWL_USERDATA);
		// [閉じる]
		if (id == data->closeTaskitemItemId) {
			HWND hTargetWindow = pHoldingTask->getWindow();
			SendMessage(hTargetWindow, WM_CLOSE, NULL, NULL);
		}
		// [強制終了]
		else if(id == data->forceKillTaskitemItemId) {
			DWORD dwPid;
			HANDLE hProcess;

			HWND hTargetWindow = pHoldingTask->getWindow();
			GetWindowThreadProcessId(hTargetWindow, &dwPid);
			if((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid)) == NULL){
				MessageBox(hWnd, _T("強制終了に失敗しました。\nプロセスハンドルを開けませんでした。"), _T("エラー"), MB_OK | MB_ICONERROR);
				data->hHoldingTaskItem = NULL;
				data->isShowingMenuItem = FALSE;
				return;
			}
			if(TerminateProcess(hProcess, 1)==0){
				MessageBox(hWnd, _T("強制終了に失敗しました。\n関数の実行に失敗しました。"), _T("エラー"), MB_OK | MB_ICONERROR);
				CloseHandle(hProcess);
				data->hHoldingTaskItem = NULL;
				data->isShowingMenuItem = FALSE;
				return;
			}

			CloseHandle(hProcess);
		}
		// [最小化]...
		else if(id == data->minimizeTaskitemItemId){
			ShowWindow(pHoldingTask->getWindow() ,SW_MINIMIZE);
		}

		data->hHoldingTaskItem = NULL;
		data->isShowingMenuItem = FALSE;
	}

	if (processed)
		return;

	map<HWND, map<int, PluginMenuItemSelectedCallback> >::const_iterator
		menuIter = data->pluginMenuItemSelectedTable.find(hMenu);

	if (menuIter != data->pluginMenuItemSelectedTable.end()) {
		map<int, PluginMenuItemSelectedCallback>::const_iterator itemIter =
			menuIter->second.find(id);

		if (itemIter != menuIter->second.end())
			itemIter->second(hWnd, hMenu, id);
	}
}

static bool onBindedKeyPress(HWND hWnd, int id) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (id == data->keyBindingTable[KEY_BINDING_SHOW_MAIN_MENU]) {
		if (IsWindowVisible(data->hMainMenuWindow))
			hideMenu(data->hMainMenuWindow);
		else
			showMenu(data->hMainMenuWindow);
	}
	else if (id == data->keyBindingTable[KEY_BINDING_FOLD_TASKBAR]) {
		fold(hWnd, !data->isFolded);

		changeWindowLayout(hWnd);
	}
	else if (id == data->keyBindingTable[KEY_BINDING_SHOW_DICTIONARY_SCREEN]) {
		HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);
		if (hEdWindow != NULL) {
			if (hEdWindow == GetForegroundWindow()) {
				SetWindowPos(hEdWindow, HWND_BOTTOM, 0, 0, 0, 0,
					SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

				fold(hWnd, false);
				SetForegroundWindow(hWnd);
			}
			else {
				SetForegroundWindow(hEdWindow);
				fold(hWnd, true);
			}

			changeWindowLayout(hWnd);
		}
	}
	else if (id == data->keyBindingTable[KEY_BINDING_SWITCH_TASK]) {
		int foundIndex = -1;
		int numTasks = data->tasks.size();

		for (i = 0; i < numTasks; i++) {
			TaskItem *task = data->tasks[i];
			if (task == data->currentTask) {
				foundIndex = i;
				break;
			}
		}

		TaskItem *targetTask = NULL;
		if (foundIndex == -1) {
			if (numTasks > 0)
				targetTask = data->tasks[0];
		}
		else {
			targetTask = data->tasks[
				(foundIndex >= numTasks - 1) ? 0 : foundIndex + 1];
		}

		if (targetTask != NULL) {
			SetForegroundWindow(targetTask->getWindow());
			switchTask(hWnd, targetTask);
		}
	}

	return true;
}

static void onPluginGetFontInfo(HWND hWnd, PluginFontInfo *info) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	tstring fontName;
	int pointSize = 0;
	bool isBold = false;
	bool isItalic = false;

	KnceUtil::getFontAttributes(data->hNewFont, fontName, pointSize, isBold,
		isItalic);

	_tcsncpy(info->faceName, fontName.c_str(), LF_FACESIZE);
	info->pointSize = pointSize;
	info->isBold = isBold ? 1 : 0;
	info->isItalic = isItalic ? 1 : 0;
}

static COLORREF onPluginGetColor(HWND hWnd, const TCHAR *name) {
	return g_colorProfile.getColor(name);
}

static int onPluginAppendMenuItem(HWND hWnd, HWND hMenu,
	PluginMenuItem *item) {

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (hMenu == NULL)
		hMenu = data->hMainMenuWindow;

	int id = appendMenuItem(hMenu, item->caption, item->hSubMenu);

	if (item->onItemSelected != NULL)
		data->pluginMenuItemSelectedTable[hMenu][id] = item->onItemSelected;

	return id;
}

static void onPluginRemoveMenuItem(HWND hWnd, HWND hMenu, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (hMenu == NULL)
		hMenu = data->hMainMenuWindow;

	removeMenuItem(hMenu, id);

	map<HWND, map<int, PluginMenuItemSelectedCallback> >::iterator menuIter =
		data->pluginMenuItemSelectedTable.find(hMenu);

	if (menuIter != data->pluginMenuItemSelectedTable.end()) {
		map<int, PluginMenuItemSelectedCallback>::iterator itemIter =
			menuIter->second.find(id);

		if (itemIter != menuIter->second.end())
			data->pluginMenuItemSelectedTable[hMenu].erase(itemIter);
	}
}

static HWND onPluginCreateSubMenu(HWND hWnd, PluginMenu *menu) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	MenuParams menuParams;
	HWND hMenuWindow = createMenu(hWnd, menuParams);

	if (menu->onShow != NULL)
		data->pluginMenuShowTable[hMenuWindow] = menu->onShow;

	data->hSubMenuWindows.push_back(hMenuWindow);

	return hMenuWindow;
}

static void onPluginClearSubMenu(HWND hWnd, HWND hMenu) {
	clearMenu(hMenu);
}

static void onPluginShowPopupMenu(HWND hWnd, HWND hMenu, POINT *pt) {
	RECT rect = {pt->x, pt->y, pt->x, pt->y};
	setMenuAttachedRect(hMenu, &rect);

	showMenu(hMenu);
}

static HWND onPluginGetMenu(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	return data->hPluginMenuWindow;
}

static HWND onPluginRegisterTrayItem(HWND hWnd, HMODULE hModule, PluginTrayItem *item) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);
	TCHAR modName[MAX_PATH];

	DWORD style = WS_CHILD | WS_VISIBLE;

	int id = obtainNewTrayIconControlId(hWnd);

	HWND hTrayCtrl = CreateWindow(_T("STATIC"), _T(""), style,
		CW_USEDEFAULT, CW_USEDEFAULT, item->width, item->height,
		hWnd, (HMENU)id, g_hInstance, NULL);

	data->prevWndProcTable[hTrayCtrl] = (WNDPROC)SetWindowLong(hTrayCtrl,
		GWL_WNDPROC, (LONG)trayItemProc);

	SendMessage(hTrayCtrl, WM_SETFONT, (WPARAM)g_hMainFont, 0);

	TrayIconItem *trayItem = new TrayIconItem();
	trayItem->setControl(hTrayCtrl);

	trayItem->setType(item->type);
	trayItem->setID(item->uID);

	GetModuleFileName(hModule, modName, MAX_PATH);
	tstring modFile = KnceUtil::getFileName(modName);
	TaskItemTransparencyVector::iterator transparency = data->taskItemTransparency.find_ts(modFile);
	if(transparency == data->taskItemTransparency.end()){
		UINT *newtrans = new UINT;
		*newtrans = *(data->taskItemTransparency[_T("タスクバー")].second[0]);
		data->taskItemTransparency[modFile].second[item->uID] = newtrans;
		trayItem->setTransparency(newtrans);
	}
	else if(transparency->second.find(item->uID) == transparency->second.end()){
		UINT *newtrans = new UINT;
		*newtrans = *(data->taskItemTransparency[_T("タスクバー")].second[0]);
		transparency->second[item->uID] = newtrans;
		trayItem->setTransparency(newtrans);
	}
	else
		trayItem->setTransparency(transparency->second[item->uID]);

	SetWindowLong(hTrayCtrl, GWL_USERDATA, (LONG)trayItem);

	data->trayIconItems.push_back(trayItem);

	return hTrayCtrl;
}

static int onPluginRegisterTrayIconItem(HWND hWnd, HMODULE hModule, PluginTrayIconItem *item) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);
	TCHAR modName[MAX_PATH];

	int id = obtainNewTrayIconControlId(hWnd);

	HWND hTrayCtrl = CreateWindow(_T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, 16, 16, hWnd, (HMENU)id, g_hInstance,
		NULL);

	data->prevWndProcTable[hTrayCtrl] = (WNDPROC)SetWindowLong(hTrayCtrl,
		GWL_WNDPROC, (LONG)trayIconControlProc);

	SendMessage(hTrayCtrl, WM_SETFONT, (WPARAM)g_hMainFont, 0);

	TrayIconItem *trayItem = new TrayIconItem();
	trayItem->setControl(hTrayCtrl);
	trayItem->setIcon(item->hIcon);

	trayItem->setID(item->uID);

	GetModuleFileName(hModule, modName, MAX_PATH);
	tstring modFile = KnceUtil::getFileName(modName);
	TaskItemTransparencyVector::iterator transparency = data->taskItemTransparency.find_ts(modFile);
	if(transparency == data->taskItemTransparency.end()){
		UINT *newtrans = new UINT;
		*newtrans = *(data->taskItemTransparency[_T("タスクバー")].second[0]);
		data->taskItemTransparency[modFile].second[item->uID] = newtrans;
		trayItem->setTransparency(newtrans);
	}
	else if(transparency->second.find(item->uID) == transparency->second.end()){
		UINT *newtrans = new UINT;
		*newtrans = *(data->taskItemTransparency[_T("タスクバー")].second[0]);
		transparency->second[item->uID] = newtrans;
		trayItem->setTransparency(newtrans);
	}
	else
		trayItem->setTransparency(transparency->second[item->uID]);

	SetWindowLong(hTrayCtrl, GWL_USERDATA, (LONG)trayItem);

	data->trayIconItems.push_back(trayItem);

	if (item->onClicked != NULL)
		data->pluginTrayIconItemClickedTable[id] = item->onClicked;

	changeWindowLayout(hWnd);

	return id;
}

static void onPluginUnregisterTrayIconItem(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	HWND hTrayCtrl = GetDlgItem(hWnd, id);

	TrayIconItem *item = (TrayIconItem *)GetWindowLong(hTrayCtrl,
		GWL_USERDATA);
	delete item;

	DestroyWindow(hTrayCtrl);

	data->trayIconItems.erase(find(data->trayIconItems.begin(),
		data->trayIconItems.end(), item));

	if (data->pluginTrayIconItemClickedTable.find(id) !=
		data->pluginTrayIconItemClickedTable.end()) {

		data->pluginTrayIconItemClickedTable.erase(id);
	}

	changeWindowLayout(hWnd);
}

static HWND onPluginGetTrayIconControl(HWND hWnd, int id) {
	return GetDlgItem(hWnd, id);
}

static void onPluginSetTrayIcon(HWND hWnd, int id, HICON hIcon) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	HWND hTrayCtrl = GetDlgItem(hWnd, id);
	TrayIconItem *trayItem = (TrayIconItem *)GetWindowLong(hTrayCtrl,
		GWL_USERDATA);
	trayItem->setIcon(hIcon);

	InvalidateRect(hTrayCtrl, NULL, false);
}

static HWND onPluginGetDesktopMenu(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	return getDesktopMenuWindow(data->hDesktopWindow);
}

static void onPluginDesktopToScreen(HWND hWnd, POINT *pt) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	ClientToScreen(data->hDesktopWindow, pt);
}

static int onPluginRegisterDesktopItem(HWND hWnd, PluginDesktopItem *item) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	DesktopItemCallbackTable callbackTable;
	callbackTable.setFocusCallback(item->onFocus);
	callbackTable.setBlurCallback(item->onBlur);
	callbackTable.setPaintCallback(item->onPaint);
	callbackTable.setTapDownCallback(item->onMouseButtonDown);
	callbackTable.setTapUpCallback(item->onMouseButtonUp);
	callbackTable.setTapMoveCallback(item->onMouseMove);
	callbackTable.setTapDoubleCallback(item->onDoubleClick);
	callbackTable.setTapHoldCallback(item->onTapHold);
	callbackTable.setKeyDownCallback(item->onKeyDown);
	callbackTable.setKeyUpCallback(item->onKeyUp);

	int id = registerDesktopItem(data->hDesktopWindow, item->isFocusable != 0,
		item->left, item->top, item->width, item->height, callbackTable);

	return id;
}

static void onPluginUnregisterDesktopItem(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	unregisterDesktopItem(data->hDesktopWindow, id);
}

static void onPluginGetDesktopItemPlacement(HWND hWnd, int id, RECT *place) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	getDesktopItemPlacement(data->hDesktopWindow, id, *place);
}

static void onPluginSetDesktopItemPosition(HWND hWnd, int id,
	PluginSetDesktopItemPositionParams *params) {

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	setDesktopItemPosition(data->hDesktopWindow, id, params->flags,
		params->left, params->top, params->width, params->height);
}

static void onPluginRedrawDesktopItem(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	redrawDesktopItem(data->hDesktopWindow, id);
}

static void onPluginFocusDesktopItem(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	focusDesktopItem(data->hDesktopWindow, id);
}

static void onPluginSetDesktopMouseCapture(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	setDesktopMouseCapture(data->hDesktopWindow, id);
}

static void onPluginReleaseDesktopMouseCapture(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	releaseDesktopMouseCapture(data->hDesktopWindow);
}

static int onPluginRegisterKeyBinding(HWND hWnd, PluginKeyBinding *bind) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	int id = data->keyBinder.registerKeyBinding(bind->category, bind->name,
		bind->isEnabled ? 1 : 0, bind->defaultFn ? 1 : 0, bind->defaultKeyCode,
		(KeyBinderCallback)bind->onKeyPress, bind->isActive ? 1 : 0);

	return id;
}

static void onPluginUnregisterKeyBinding(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->keyBinder.unregisterKeyBinding(id);
}

static void onPluginActivateKeyBinding(HWND hWnd, int id, int active) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (active == 0)
		data->keyBinder.inactivateKeyBinding(id);
	else
		data->keyBinder.activateKeyBinding(id);
}

static void onPluginSuspendKeyBindings(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->keyBinder.suspend();
}

static void onPluginResumeKeyBindings(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->keyBinder.resume();
}

static int onPluginRegisterSubTouch(HWND hWnd, PluginSubTouch *touch) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	int id = data->subTouchManager.registerSubTouch(touch->priority,
		(SubTouchCallback)touch->onActive, (SubTouchCallback)touch->onInactive,
		(SubTouchDicKeyDownCallback)touch->onDicKeyDown, touch->isEnabled);

	return id;
}

static void onPluginUnregisterSubTouch(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->subTouchManager.unregisterSubTouch(id);
}

static void onPluginEnableSubTouch(HWND hWnd, int id, int enable) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (enable == 0)
		data->subTouchManager.disableSubTouch(id);
	else
		data->subTouchManager.enableSubTouch(id);
}

static int onPluginHardKeyToDicKey(HWND hWnd, bool fn, int keyCode) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (data->dicKeyTable.empty())
		return 0;
	else
		return fn ? data->fnDicKeyTable[keyCode] : data->dicKeyTable[keyCode];
}

static int onPluginRegisterTimer(HWND hWnd, int interval,
	PluginTimerCallback callback) {

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	int id = obtainNewTimerId(hWnd);
	SetTimer(hWnd, id, interval, NULL);

	data->timerCallbackTable[id] = callback;

	return id;
}

static void onPluginUnregisterTimer(HWND hWnd, int id) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (data->timerCallbackTable.find(id) == data->timerCallbackTable.end())
		return;

	KillTimer(hWnd, id);

	data->timerCallbackTable.erase(id);
}

static void onPluginRegisterCallback(HWND hWnd, int type,
	PluginCallback callback) {

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->pluginCallbackTable[type].push_back(callback);
}

static void onPluginUnregisterCallback(HWND hWnd, int type,
	PluginCallback callback) {

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	map<int, vector<PluginCallback> >::iterator typeIter =
		data->pluginCallbackTable.find(type);

	if (typeIter != data->pluginCallbackTable.end()) {
		vector<PluginCallback>::iterator callbackIter =
			find(typeIter->second.begin(), typeIter->second.end(), callback);

		if (callbackIter != typeIter->second.end())
			data->pluginCallbackTable[type].erase(callbackIter);
	}
}

static void launchFontLoader() {
	tstring fontfile,fontdir,loadfile;
	WIN32_FIND_DATA fdat;
	HANDLE hFind;
	BOOL bEnd = FALSE;
	HKEY hKey_ceopener;
	DWORD dwDisposion,dwBuf=0;

	RegCreateKeyEx((HKEY)HKEY_CURRENT_USER, _T("Software\\knatech\\ceopener"), NULL,
		NULL, REG_OPTION_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey_ceopener, &dwDisposion);

	//Find ttc file

	fontfile = KnceUtil::getCurrentDirectory() + _T("\\fonts\\*.ttc");
	fontdir = KnceUtil::getCurrentDirectory() + _T("\\fonts\\");
	hFind = FindFirstFile(fontfile.c_str(), &fdat);

	if(hFind == INVALID_HANDLE_VALUE){
		if(GetLastError() == ERROR_NO_MORE_FILES)
			bEnd = TRUE;
		else
			return;
	}
	
	while(!bEnd){
		loadfile = fontdir + fdat.cFileName;
		if(_tcsicmp(fdat.cFileName, _T("jptahoma.ttc")) != 0 && RegQueryValueEx(hKey_ceopener, fdat.cFileName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			AddFontResource(loadfile.c_str());
		if(FindNextFile(hFind, &fdat) == FALSE){
			if(GetLastError() == ERROR_NO_MORE_FILES)
				bEnd = TRUE;
			else
				return;
		}
		RegSetValueEx(hKey_ceopener, fdat.cFileName, NULL, REG_DWORD, (BYTE*)&dwBuf, 4);
	}

	bEnd = FALSE;

	//find ttf file

	fontfile = KnceUtil::getCurrentDirectory() + _T("\\fonts\\*.ttf");
	hFind = FindFirstFile(fontfile.c_str(), &fdat);

	if(hFind == INVALID_HANDLE_VALUE){
		if(GetLastError() == ERROR_NO_MORE_FILES)
			bEnd = TRUE;
		else
			return;
	}
	
	while(!bEnd){
		loadfile = fontdir + fdat.cFileName;
		if(RegQueryValueEx(hKey_ceopener, fdat.cFileName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
			AddFontResource(loadfile.c_str());
		if(FindNextFile(hFind, &fdat) == FALSE){
			if(GetLastError() == ERROR_NO_MORE_FILES)
				bEnd = TRUE;
			else
				return;
		}
		RegSetValueEx(hKey_ceopener, fdat.cFileName, NULL, REG_DWORD, (BYTE*)&dwBuf, 4);
	}

	return;
}

static void setupColorProfile() {
	map<tstring, COLORREF> colorTable;

	colorTable[_T("desktop")] = RGB(58, 110, 165);
	colorTable[_T("taskbar")] = RGB(0, 128, 192);
	colorTable[_T("taskbarText")] = RGB(255, 255, 255);
	colorTable[_T("activeTaskItem")] = RGB(0, 86, 128);
	colorTable[_T("activeTaskItemText")] = RGB(255, 255, 255);
	colorTable[_T("mainMenuButton")] = RGB(0, 170, 0);
	colorTable[_T("mainMenuButtonText")] = RGB(255, 255, 255);
	colorTable[_T("text")] = RGB(0, 0, 0);
	colorTable[_T("menuFace")] = RGB(255, 255, 255);
	colorTable[_T("menuShadow")] = RGB(0, 128, 192);
	colorTable[_T("selectedItem")] = RGB(0, 172, 192);
	colorTable[_T("selectedText")] = RGB(255, 255, 255);

	g_colorProfile.setDefaultProfile(colorTable);
}

static void setupKeyBindings(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	registerKeyBinding(hWnd, KEY_BINDING_SHOW_MAIN_MENU,
		_T("ceOpener"), _T("スタートメニューの表示"), false, HARD_KEY_SWITCH);
	registerKeyBinding(hWnd, KEY_BINDING_FOLD_TASKBAR,
		_T("ceOpener"), _T("タスクバーを隠す"), false, HARD_KEY_SWITCH);
	registerKeyBinding(hWnd, KEY_BINDING_SHOW_DICTIONARY_SCREEN,
		_T("ceOpener"), _T("辞書アプリを前面に表示"), false, HARD_KEY_SWITCH);
	registerKeyBinding(hWnd, KEY_BINDING_SWITCH_TASK,
		_T("ceOpener"), _T("タスクの切り替え"), false, HARD_KEY_SWITCH);
}

static void registerKeyBinding(HWND hWnd, int id, const tstring &category,
	const tstring &name, bool fn, int keyCode) {

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	data->keyBindingTable[id] = data->keyBinder.registerKeyBinding(category,
		name, false, fn, keyCode, onBindedKeyPress, true);
}

static void setupDicKeyTable(HWND hWnd) {
	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	if (!brainApisEnabled())
		return;

	MSG msg = {0};
	if (GetKeyState(HARD_KEY_FUNCTION) != 0) {
		// clear fn
		keybd_event(HARD_KEY_FUNCTION, 0, 0, NULL);
		keybd_event(HARD_KEY_FUNCTION, 0, KEYEVENTF_KEYUP, NULL);

		GetMessage(&msg, hWnd, WM_KEYDOWN, WM_KEYDOWN);
		GetMessage(&msg, hWnd, WM_KEYUP, WM_KEYUP);
	}

	int keyCode = 0;
	for ( ; keyCode < 256; keyCode++)
		data->dicKeyTable.push_back(Win2DicKey_NewPF_win(keyCode, 0));

	// use fn
	keybd_event(HARD_KEY_FUNCTION, 0, 0, NULL);
	keybd_event(HARD_KEY_FUNCTION, 0, KEYEVENTF_KEYUP, NULL);

	GetMessage(&msg, hWnd, WM_KEYDOWN, WM_KEYDOWN);
	GetMessage(&msg, hWnd, WM_KEYUP, WM_KEYUP);

	// clear fn
	keybd_event(HARD_KEY_FUNCTION, 0, 0, NULL);
	keybd_event(HARD_KEY_FUNCTION, 0, KEYEVENTF_KEYUP, NULL);

	for (keyCode = 0 ; keyCode < 256; keyCode++)
		data->fnDicKeyTable.push_back(Win2DicKey_NewPF_win(keyCode, 0));

	GetMessage(&msg, hWnd, WM_KEYDOWN, WM_KEYDOWN);
	GetMessage(&msg, hWnd, WM_KEYUP, WM_KEYUP);

	data->keyBinder.setDicKeyTables(data->dicKeyTable, data->fnDicKeyTable);
}

static void fold(HWND hWnd, bool needFold) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	RECT prevWorkAreaRect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &prevWorkAreaRect, 0);

	RECT desktopRect;
	desktopRect = g_EdRect;

	RECT workAreaRect;
	workAreaRect = desktopRect;
	workAreaRect.bottom -= needFold ? 0 : TASKBAR_HEIGHT;

	SystemParametersInfo(SPI_SETWORKAREA, 0, &workAreaRect, SPIF_SENDCHANGE);

	HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);

	vector<HWND> windows;
	if (data->isShowPopupsTaskbar)
		EnumWindows(findWindowProc, (LPARAM)&windows);
	else
		EnumWindows(findWindowProcNoPopup, (LPARAM)&windows);

	int numWindows = windows.size();
	for (i = 0; i < numWindows; i++) {
		HWND hTargetWindow = windows[i];

		if (hEdWindow != NULL && hTargetWindow == hEdWindow)
			continue;

		RECT windowRect;
		GetWindowRect(hTargetWindow, &windowRect);

		if (windowRect.left == prevWorkAreaRect.left &&
			windowRect.top == prevWorkAreaRect.top &&
			windowRect.right == prevWorkAreaRect.right &&
			windowRect.bottom == prevWorkAreaRect.bottom) {

			MoveWindow(hTargetWindow, workAreaRect.left, workAreaRect.top,
				workAreaRect.right, workAreaRect.bottom, false);
		}
	}

	if (needFold) {
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) |
			WS_EX_NOACTIVATE);
		ShowWindow(hWnd, SW_HIDE);
		ShowWindow(hWnd, SW_SHOWNA);
	}
	else {
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) &
			~WS_EX_NOACTIVATE);
	}

	vector<PluginCallback> &callbacks =
		data->pluginCallbackTable[PLUGIN_CALLBACK_FOLDED];
	int numCallbacks = callbacks.size();

	for (i = 0; i < numCallbacks; i++)
		callbacks[i](hWnd, needFold);

	data->isFolded = needFold;
}

static void updateTasks(HWND hWnd) {
	int i, j;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	vector<HWND> windows;
	if (data->isShowPopupsTaskbar)
		EnumWindows(findWindowProc, (LPARAM)&windows);
	else
		EnumWindows(findWindowProcNoPopup, (LPARAM)&windows);

	int numTasks = data->tasks.size();
	for (i = 0; i < numTasks; i++)
		data->tasks[i]->setMarked(false);

	vector<HWND> newWindows;

	int numWindows = windows.size();
	for (i = 0; i < numWindows; i++) {
		HWND hWnd = windows[i];
		bool found = false;

		for (j = 0; j < numTasks; j++) {
			TaskItem *task = data->tasks[j];
			if (task->getWindow() == hWnd) {
				task->setMarked(true);
				found = true;
				break;
			}
		}

		if (!found)
			newWindows.push_back(hWnd);
	}

	bool modified = false;

	vector<TaskItem *>::iterator iter = data->tasks.begin();
	while (iter != data->tasks.end()) {
		TaskItem *task = *iter;
		if (task->isMarked())
			iter++;
		else {
			DestroyWindow(task->getButton());
			delete task;
			iter = data->tasks.erase(iter);
			modified = true;
		}
	}

	int numNewWindows = newWindows.size();
	for (i = 0; i < numNewWindows; i++) {
		int itemId = obtainNewTaskItemId(hWnd);

		HWND hItemWindow = CreateWindowEx(WS_EX_NOACTIVATE,
			_T("BUTTON"), _T("Task Item"),
			BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			hWnd, (HMENU)itemId, g_hInstance, NULL);

		SendMessage(hItemWindow, WM_SETFONT, (WPARAM)g_hMainFont, 0);

		TaskItem *task = new TaskItem();
		task->setWindow(newWindows[i]);
		task->setButton(hItemWindow);
		task->setButtonId(itemId);

		SetWindowLong(hItemWindow, GWL_USERDATA, (LONG)task);

		WNDPROC lpfnWndProc = (WNDPROC)GetWindowLong(hItemWindow, GWL_WNDPROC);

		data->prevWndProcTable[hItemWindow] = lpfnWndProc;

		SetWindowLong(hItemWindow, GWL_WNDPROC, (LONG)taskItemProc);

		data->tasks.push_back(task);

		modified = true;
	}

	TCHAR curCaptionBuf[256];
	TCHAR newCaptionBuf[256];

	numTasks = data->tasks.size();
	for (i = 0; i < numTasks; i++) {
		TaskItem *task = data->tasks[i];
		HWND hItemWindow = task->getButton();

		GetWindowText(task->getWindow(), newCaptionBuf, 256);
		GetWindowText(hItemWindow, curCaptionBuf, 256);

		if (_tcscmp(newCaptionBuf, curCaptionBuf) != 0)
			SetWindowText(hItemWindow, newCaptionBuf);
	}

	if (modified)
		changeWindowLayout(hWnd);

	HWND hActiveWindow = GetForegroundWindow();
	if (data->currentTask == NULL ||
		hActiveWindow != data->currentTask->getWindow()) {

		switchTask(hWnd, NULL, hActiveWindow);
	}
}

static BOOL CALLBACK findWindowProcNoPopup(HWND hWnd, LPARAM lParam) {
	vector<HWND> *windows = (vector<HWND> *)lParam;

	if (checkTaskableNoPopup(hWnd))
		windows->push_back(hWnd);

	return true;
}

static BOOL CALLBACK findWindowProc(HWND hWnd, LPARAM lParam) {
	vector<HWND> *windows = (vector<HWND> *)lParam;

	if (checkTaskable(hWnd))
		windows->push_back(hWnd);

	return true;
}

static bool checkTaskableNoPopup(HWND hWnd) {
	if (IsWindowVisible(hWnd) && GetWindowTextLength(hWnd) > 0) {
		long style = GetWindowLong(hWnd, GWL_STYLE);
		return (style & WS_POPUP) == 0;
	}

	return false;
}

static bool checkTaskable(HWND hWnd) {
	static TCHAR buf[256];

	if (IsWindowVisible(hWnd) && GetWindowTextLength(hWnd) > 0) {
		GetWindowText(hWnd, buf, 256);
		if (_tcsnccmp(buf, _T("ceOpener"), 8) != 0)
			return true;
	}

	return false;
}

static void switchTask(HWND hWnd, TaskItem *targetTask, HWND hTargetWindow) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	// redraw previous task
	int numTasks = data->tasks.size();
	if (data->currentTask != NULL) {
		for (i = 0; i < numTasks; i++) {
			TaskItem *task = data->tasks[i];
			if (task == data->currentTask) {
				InvalidateRect(task->getButton(), NULL, false);
				break;
			}
		}
	}

	// find target task
	if (targetTask == NULL) {
		for (i = 0; i < numTasks; i++) {
			TaskItem *task = data->tasks[i];
			if (task->getWindow() == hTargetWindow) {
				targetTask = task;
				break;
			}
		}
	}

	// redraw target task
	if (targetTask != data->currentTask) {
		if (targetTask != NULL)
			InvalidateRect(targetTask->getButton(), NULL, false);

		data->currentTask = targetTask;
	}
}

static int obtainNewTaskItemId(HWND hWnd) {
	const int MIN_ID = IDC_TASK_ITEM_BUTTON_BASE;
	const int MAX_ID = MIN_ID + 1000;
	static int currentId = MIN_ID;

	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	int numTasks = data->tasks.size();

	while (true) {
		bool found = false;
		for (i = 0; i < numTasks; i++) {
			if (data->tasks[i]->getButtonId() == currentId) {
				found = true;
				break;
			}
		}

		if (!found)
			break;

		if (++currentId > MAX_ID)
			currentId = MIN_ID;
	}

	int retId = currentId;
	if (++currentId > MAX_ID)
		currentId = MIN_ID;

	return retId;
}

static int obtainNewTrayIconControlId(HWND hWnd) {
	const int MIN_ID = IDC_TRAY_ICON_CONTROL_BASE;
	const int MAX_ID = MIN_ID + 1000;
	static int currentId = MIN_ID;

	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	int numItems = data->trayIconItems.size();

	while (true) {
		bool found = false;
		for (i = 0; i < numItems; i++) {
			TrayIconItem *item = data->trayIconItems[i];
			if (GetDlgCtrlID(item->getControl()) == currentId) {
				found = true;
				break;
			}
		}

		if (!found)
			break;

		if (++currentId > MAX_ID)
			currentId = MIN_ID;
	}

	int retId = currentId;
	if (++currentId > MAX_ID)
		currentId = MIN_ID;

	return retId;
}

static int obtainNewTimerId(HWND hWnd) {
	const int MIN_ID = 101;
	const int MAX_ID = MIN_ID + 1000;
	static int currentId = MIN_ID;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	while (data->timerCallbackTable.find(currentId) !=
		data->timerCallbackTable.end()) {

		if (++currentId > MAX_ID)
			currentId = MIN_ID;
	}

	int retId = currentId;
	if (++currentId > MAX_ID)
		currentId = MIN_ID;

	return retId;
}

static void sendPowerOnMessageToEd() {
	HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);
	if (hEdWindow != NULL)
		SendMessage(hEdWindow, g_edPowerOnOffMessage, 2, 0);

	HWND hEdAppCtrl = FindWindow(_T("EdAppCtrl"), NULL);
	if (hEdAppCtrl != NULL)
		SendMessage(hEdAppCtrl, g_edPowerOnOffMessage, 2, 0);
}

static void showSettingDialog(HWND hWnd) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	SettingParams params;
	ViewingParams vparams;
	TransSettingParams tparams;
	SESettingParams sparams;

	params.setViewingParams(vparams);
	params.setTransSettingParams(tparams);
	params.setSESettingParams(sparams);

	vparams.setFont(data->hNewFont);
	vparams.setColorProfileName(data->colorProfileName);
	vparams.setWallpaperName(data->wallpaperName);
	vparams.setShowPopupsTaskbar(data->isShowPopupsTaskbar);
	vparams.setChangeTaskbarHeight(data->isChangeTaskbarHeight);

	tparams.setTaskItemTransparency(&data->taskItemTransparency);

	sparams.setSEController(data->seController);

	if (!showSettingDialog(hWnd, params))
		return;

	DeleteObject(data->hNewFont);
	data->hNewFont = vparams.getFont();

	data->colorProfileName = vparams.getColorProfileName();
	g_colorProfile.loadProfile(data->colorProfileName);

	DeleteObject(data->hTaskbarBrush);
	COLORREF taskbarColor = g_colorProfile.getColor(_T("taskbar"));
	data->hTaskbarBrush = CreateSolidBrush(taskbarColor);

	data->wallpaperName = vparams.getWallpaperName();
	setDesktopWallpaper(data->hDesktopWindow, data->wallpaperName, data->hDesktopDIB);

	RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW | 
		RDW_ALLCHILDREN);
	RedrawWindow(data->hDesktopWindow, NULL, NULL, RDW_ERASE | RDW_INVALIDATE |
		RDW_ERASENOW | RDW_ALLCHILDREN);

	data->isShowPopupsTaskbar = vparams.isShowPopupsTaskbar();
	data->isChangeTaskbarHeight = vparams.isChangeTaskbarHeight();

	vector<PluginCallback> &callbacks =
		data->pluginCallbackTable[PLUGIN_CALLBACK_SETTING_CHANGED];
	int numCallbacks = callbacks.size();

	for (i = 0; i < numCallbacks; i++)
		callbacks[i](hWnd, (unsigned long)&vparams);

	saveCurrentSettings(hWnd);
}

static void showKeyBindingDialog(HWND hWnd) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	map<KeyBindingDialogParams::Binding *, int> paramBindTable;

	KeyBindingDialogParams params;
	vector<KeyBindingDialogParams::Binding *> &paramBinds =
		params.getBindings();

	const map<int, KeyBinder::Binding *> &binds =
		data->keyBinder.getBindingTable();
	map<int, KeyBinder::Binding *>::const_iterator iter = binds.begin();

	for ( ; iter != binds.end(); iter++) {
		KeyBinder::Binding *bind = iter->second;

		if (!bind->getCategory().empty()) {
			KeyBindingDialogParams::Binding *paramBind =
				new KeyBindingDialogParams::Binding();
			paramBind->setCategory(bind->getCategory());
			paramBind->setName(bind->getName());
			paramBind->setEnabled(bind->isEnabled());
			paramBind->setDefaultFn(bind->getDefaultFn());
			paramBind->setDefaultKeyCode(bind->getDefaultKeyCode());
			paramBind->setBindedFn(bind->getBindedFn());
			paramBind->setBindedKeyCode(bind->getBindedKeyCode());

			paramBinds.push_back(paramBind);
			paramBindTable[paramBind] = iter->first;
		}
	}

	data->keyBinder.suspend();

	bool ret = showKeyBindingDialog(hWnd, params);

	data->keyBinder.resume();

	int numBinds = paramBinds.size();
	for (i = 0; i < numBinds; i++) {
		KeyBindingDialogParams::Binding *paramBind = paramBinds[i];

		if (ret) {
			int id = paramBindTable[paramBind];

			if (paramBind->isEnabled())
				data->keyBinder.enableKeyBinding(id);
			else
				data->keyBinder.disableKeyBinding(id);

			data->keyBinder.bindKey(id, paramBind->getBindedFn(),
				paramBind->getBindedKeyCode());
		}

		delete paramBind;
	}

	saveCurrentSettings(hWnd);

	return;
}

static void changeWindowLayout(HWND hWnd) {
	int i;

	OpenerMainData *data = (OpenerMainData *)GetWindowLong(hWnd, GWL_USERDATA);

	RECT desktopRect;
	desktopRect = g_EdRect;

	int taskbarLeft = data->isFolded ?
		desktopRect.right - FOLDER_BUTTON_WIDTH : 0;
	MoveWindow(hWnd, taskbarLeft, desktopRect.bottom - TASKBAR_HEIGHT,
		desktopRect.right, TASKBAR_HEIGHT, false);

	HWND hMenuButton = GetDlgItem(hWnd, IDC_MAIN_MENU_BUTTON);

	int menuButtonLeft = data->isFolded ? FOLDER_BUTTON_WIDTH : 0;
	MoveWindow(hMenuButton, menuButtonLeft, 1, MAIN_MENU_BUTTON_WIDTH,
		TASKBAR_HEIGHT - 2, false);

	HWND hFolderButton = GetDlgItem(hWnd, IDC_FOLDER_BUTTON);

	int folderButtonLeft = data->isFolded ? 0 :
		desktopRect.right - FOLDER_BUTTON_WIDTH;
	MoveWindow(hFolderButton, folderButtonLeft, 1, FOLDER_BUTTON_WIDTH,
		TASKBAR_HEIGHT - 2, false);

	RECT trayRect;
	int trayBandWidth = 2;

	int numTrayItems = data->trayIconItems.size();
	for (i = 0; i < numTrayItems; i++) {
		TrayIconItem *trayItem = data->trayIconItems[i];
		HWND trayCtrl = trayItem->getControl();

		GetClientRect(trayCtrl, &trayRect);
		trayBandWidth += trayRect.right;

		MoveWindow(trayCtrl, desktopRect.right - trayBandWidth -
			FOLDER_BUTTON_WIDTH, 4, trayRect.right, trayRect.bottom, false);

		trayBandWidth += 2;
	}

	int taskbarBandLeft = MAIN_MENU_BUTTON_WIDTH + TASKBAR_BAND_GAP;
	int taskbarBandWidth = desktopRect.right - taskbarBandLeft -
		trayBandWidth - FOLDER_BUTTON_WIDTH;

	int numTasks = data->tasks.size();

	int taskItemWidth = taskbarBandWidth / (numTasks == 0 ? 1 : numTasks);
	if (taskItemWidth > MAX_TASK_ITEM_WIDTH)
		taskItemWidth = MAX_TASK_ITEM_WIDTH;

	for (i = 0; i < numTasks; i++) {
		TaskItem *task = data->tasks[i];
		MoveWindow(task->getButton(),
			taskItemWidth * i + taskbarBandLeft, 2,
			taskItemWidth, TASKBAR_HEIGHT - 4, false);
	}
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR lpCmd, int nShow)
{
	const TCHAR *className = _T("ceOpener");

	g_hInstance = hInst;

	if (FindWindow(className, NULL) != NULL)
		return 1;

	if (LoadLibrary(_T("SHARPLIB"))!=NULL && _tcslen(lpCmd) == 0) {
		TCHAR path[MAX_PATH];
		GetModuleFileName(NULL, path, MAX_PATH);

		CreateProcess(path, _T("1"), NULL, NULL, false, 0, NULL, NULL, NULL,
			NULL);

		return 0;
	}
	else{
		HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);
		if(!hEdWindow){
			MessageBox(NULL,_T("Couldn't find a running DicApp.\nThe taskbar and the desktop may not displayed correctly."),_T("Caution"),MB_OK|MB_ICONEXCLAMATION);
			GetWindowRect(GetDesktopWindow(), &g_EdRect);
		}
		else{
			GetWindowRect(hEdWindow, &g_EdRect);
		}
	}

	loadBrainApis();

	WNDCLASS windowClass;
	memset(&windowClass, 0, sizeof(WNDCLASS));
	windowClass.lpfnWndProc = WndProc;
	windowClass.hInstance = hInst;
	windowClass.lpszClassName = className;
	RegisterClass(&windowClass);

	registerMenuClass();
	registerDesktopClass();

	HWND hWnd = CreateWindowEx(
		WS_EX_TOPMOST,
		className, _T("ceOpener Taskbar"),
		WS_POPUP /*| WS_BORDER*/,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInst, NULL);

	ShowWindow(hWnd, SW_SHOWNA);
	UpdateWindow(hWnd);
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnregisterClass(className, hInst);

	unregisterMenuClass();
	unregisterDesktopClass();

	if (brainApisEnabled())
		freeBrainApis();

	return msg.wParam;
}

static void saveCurrentSettings(HWND hWnd) {
	OpenerMainData *data =
		(OpenerMainData*)GetWindowLong(hWnd, GWL_USERDATA);

	writeLog(_T("Write current properties..."));

	map<tstring, tstring> props;
	TCHAR valCStr[256];

	tstring fontName;
	int pointSize = 0;
	bool isBold = false;
	bool isItalic = false;

	KnceUtil::getFontAttributes(data->hNewFont, fontName, pointSize, isBold,
		isItalic);

	props[_T("global.fontName")] = fontName;

	_sntprintf(valCStr, 256, _T("%d"), pointSize);
	props[_T("global.fontSize")] = valCStr;

	props[_T("global.colorProfileName")] = data->colorProfileName;
	props[_T("global.wallpaperName")] = data->wallpaperName;
	props[_T("global.showPopupsTaskbar")] = data->isShowPopupsTaskbar ?
		_T("1") : _T("0");
	props[_T("global.changeTaskbarHeight")] = data->isChangeTaskbarHeight ?
		_T("1") : _T("0");

	data->keyBinder.storeProperties(props);

	tstring persistDir = _T("\\Nand2\\.ceopener");
	if (GetFileAttributes(persistDir.c_str()) == -1)
		CreateDirectory(persistDir.c_str(), NULL);

	TaskItemTransparencyVector::iterator tItemIter = data->taskItemTransparency.begin();
	for(; tItemIter != data->taskItemTransparency.end(); tItemIter++){
		map<UINT, UINT*>::iterator subIter = tItemIter->second.begin();
		for(; subIter != tItemIter->second.end(); subIter++){
			tstringstream key, value;
			key << _T("titems.") << tItemIter->first << _T(".") << subIter->first;
			value << *(subIter->second);
			props[key.str()] = value.str();
		}
	}

	vector<SEController::SoundEffect*> soundEffects = data->seController.getSoundEffects();
	for(unsigned int i = 0; i < soundEffects.size(); i++) {
		props[_T("sndEffect.fileName.") + soundEffects[i]->verb()]
			= soundEffects[i]->fileName();
		props[_T("sndEffect.enabled.") + soundEffects[i]->verb()]
			= soundEffects[i]->isEnabled() ? _T("1") : _T("0");
	}

	tstring propFileName = persistDir + _T("\\ceopener.dat");
	KnceUtil::writePropertyFile(propFileName, props);
}
