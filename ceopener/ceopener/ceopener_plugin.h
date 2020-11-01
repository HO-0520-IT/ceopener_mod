#ifndef CEOPENER_PLUGIN_H
#define CEOPENER_PLUGIN_H

#define PLUGIN_TRAY_ITEM_TEXT 0
#define PLUGIN_TRAY_ITEM_ICON 1
#define PLUGIN_TRAY_ITEM_BITMAP 2

#define PLUGIN_DESKTOP_ITEM_NO_MOVE 1
#define PLUGIN_DESKTOP_ITEM_NO_RESIZE 2

#define PLUGIN_SUB_TOUCH_PRIORITY_LOWEST -1
#define PLUGIN_SUB_TOUCH_PRIORITY_NORMAL 0

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

struct PluginInfo {
};

typedef void (*PluginMenuShowCallback)(HWND hCont, HWND hMenu);
typedef void (*PluginMenuItemSelectedCallback)(HWND hCont, HWND hMenu, int id);
typedef void (*PluginTrayIconItemClickedCallback)(HWND hCont, int id);
typedef void (*PluginDesktopItemFocusCallback)(HWND hCont, int id);
typedef void (*PluginDesktopItemBlurCallback)(HWND hCont, int id);
typedef void (*PluginDesktopItemTapDownCallback)(HWND hCont, int id,
    int x, int y, RECT *place);
typedef void (*PluginDesktopItemTapUpCallback)(HWND hCont, int id,
    int x, int y, RECT *place);
typedef void (*PluginDesktopItemTapMoveCallback)(HWND hCont, int id,
    int x, int y, RECT *place);
typedef void (*PluginDesktopItemTapDoubleCallback)(HWND hCont, int id,
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
	PluginTrayItemTransparencyChangedCallback onTransChanged;
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
    PluginDesktopItemTapDownCallback onTapDown;
    PluginDesktopItemTapUpCallback onTapUp;
    PluginDesktopItemTapMoveCallback onTapMove;
    PluginDesktopItemTapDoubleCallback onTapDouble;
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

struct PluginSetMenuItemCaptionParams {
    int id;
    TCHAR caption[256];
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

inline HWND pluginGetContainer() {
    return FindWindow(_T("ceOpener"), NULL);
}

inline HWND pluginGetDesktopWindow() {
    return FindWindow(_T("ceOpenerDesktop"), NULL);
}

inline void pluginGetFontInfo(HWND hCont, PluginFontInfo *info) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginGetFontInfo")), 0, (LPARAM)info);
}

inline COLORREF pluginGetColor(HWND hCont, const TCHAR *name) {
    return SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginGetColor")), 0, (LPARAM)name);
}

inline int pluginAppendMenuItem(HWND hCont, HWND hMenu, PluginMenuItem *item) {
    return SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginAppendMenuItem")), (WPARAM)hMenu, (LPARAM)item);
}

inline void pluginRemoveMenuItem(HWND hCont, HWND hMenu, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRemoveMenuItem")), (WPARAM)hMenu, id);
}

inline HWND pluginCreateSubMenu(HWND hCont, PluginMenu *menu) {
    return (HWND)SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginCreateSubMenu")), 0, (LPARAM)menu);
}

inline void pluginDestroySubMenu(HWND hCont, HWND hMenu) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginDestroySubMenu")), (WPARAM)hMenu, 0);
}

inline void pluginClearSubMenu(HWND hCont, HWND hMenu) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginClearSubMenu")), (WPARAM)hMenu, 0);
}

inline void pluginShowPopupMenu(HWND hCont, HWND hMenu, int x, int y) {
    POINT pt = {x, y};
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginShowPopupMenu")), (WPARAM)hMenu, (LPARAM)&pt);
}

inline HWND pluginGetPluginMenu(HWND hCont) {
    return (HWND)SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginGetPluginMenu")), 0, 0);
}

inline void pluginSetMenuItemCaption(HWND hCont, HWND hMenu, int id,
    const TCHAR *caption) {

    PluginSetMenuItemCaptionParams params;
    params.id = id;
    _tcsncpy(params.caption, caption, 256);

    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginSetMenuItemCaption")), (WPARAM)hMenu, (LPARAM)&params);
}

inline HWND pluginRegisterTrayItem(HWND hCont, HMODULE hModule, PluginTrayItem *item) {
    return (HWND)SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRegisterTrayItem")), (WPARAM)hModule, (LPARAM)item);
}

inline int pluginRegisterTrayIconItem(HWND hCont, HMODULE hModule, PluginTrayIconItem *item) {
    return SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRegisterTrayIconItem")), (WPARAM)hModule, (LPARAM)item);
}

inline void pluginUnregisterTrayIconItem(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginUnregisterTrayIconItem")), id, 0);
}

inline HWND pluginGetTrayIconControl(HWND hCont, int id) {
    return (HWND)SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginGetTrayIconControl")), id, 0);
}

inline void pluginSetTrayIcon(HWND hCont, int id, HICON hIcon) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginSetTrayIcon")), id, (LPARAM)hIcon);
}

inline HWND pluginGetDesktopMenu(HWND hCont) {
    return (HWND)SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginGetDesktopMenu")), 0, 0);
}

inline void pluginDesktopToScreen(HWND hCont, POINT *pt) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginDesktopToScreen")), 0, (LPARAM)pt);
}

inline int pluginRegisterDesktopItem(HWND hCont, PluginDesktopItem *item) {
    return SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRegisterDesktopItem")), 0, (LPARAM)item);
}

inline void pluginUnregisterDesktopItem(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginUnregisterDesktopItem")), id, 0);
}

inline void pluginGetDesktopItemPlacement(HWND hCont, int id, RECT *place) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginGetDesktopItemPlacement")), id, (LPARAM)place);
}

inline void pluginSetDesktopItemPosition(HWND hCont, int id, int flags,
    int left, int top, int width, int height) {

    PluginSetDesktopItemPositionParams params;
    params.flags = flags;
    params.left = left;
    params.top = top;
    params.width = width;
    params.height = height;

    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginSetDesktopItemPosition")), id, (LPARAM)&params);
}

inline void pluginRedrawDesktopItem(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRedrawDesktopItem")), id, 0);
}

inline void pluginFocusDesktopItem(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginFocusDesktopItem")), id, 0);
}

inline void pluginSetDesktopMouseCapture(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginSetDesktopMouseCapture")), id, 0);
}

inline void pluginReleaseDesktopMouseCapture(HWND hCont) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginReleaseDesktopMouseCapture")), 0, 0);
}

inline int pluginRegisterKeyBinding(HWND hCont, PluginKeyBinding *bind) {
    return SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRegisterKeyBinding")), 0, (LPARAM)bind);
}

inline void pluginUnregisterKeyBinding(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginUnregisterKeyBinding")), id, NULL);
}

inline void pluginActivateKeyBinding(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginActivateKeyBinding")), id, 1);
}

inline void pluginInactivateKeyBinding(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginActivateKeyBinding")), id, 0);
}

inline void pluginSuspendKeyBindings(HWND hCont) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginSuspendKeyBindings")), 0, 0);
}

inline void pluginResumeKeyBindings(HWND hCont) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginResumeKeyBindings")), 0, 0);
}

inline int pluginRegisterSubTouch(HWND hCont, PluginSubTouch *touch) {
    return SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRegisterSubTouch")), 0, (LPARAM)touch);
}

inline void pluginUnregisterSubTouch(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginUnregisterSubTouch")), id, 0);
}

inline void pluginEnableSubTouch(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginEnableSubTouch")), id, 1);
}

inline void pluginDisableSubTouch(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginEnableSubTouch")), id, 0);
}

inline int pluginHardKeyToDicKey(HWND hCont, int fn, int keyCode) {
    return SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginHardKeyToDicKey")), MAKEWORD(keyCode, fn), 0);
}

inline int pluginRegisterTimer(HWND hCont, int interval,
    PluginTimerCallback callback) {

    return SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRegisterTimer")), interval, (LPARAM)callback);
}

inline HWND pluginUnregisterTimer(HWND hCont, int id) {
    SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginUnregisterTimer")), id, 0);
}

inline HWND pluginRegisterCallback(HWND hCont, int type,
    PluginCallback callback) {

    return (HWND)SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginRegisterCallback")), type, (LPARAM)callback);
}

inline HWND pluginUnregisterCallback(HWND hCont, int type,
    PluginCallback callback) {

    return (HWND)SendMessage(hCont, RegisterWindowMessage(
        _T("OpenerPluginUnregisterCallback")), type, (LPARAM)callback);
}

#endif  // CEOPENER_PLUGIN_H
