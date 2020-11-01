#ifndef DESKTOP_H
#define DESKTOP_H

#include <string>
#include <windows.h>
#include "DrawUtil.h"

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

enum {
    DESKTOP_ITEM_NO_MOVE = 1,
    DESKTOP_ITEM_NO_RESIZE = 2
};

typedef void (*DesktopItemFocusCallback)(HWND hWnd, int id);
typedef void (*DesktopItemBlurCallback)(HWND hWnd, int id);
typedef void (*DesktopItemTapDownCallback)(HWND hWnd, int id,
    int x, int y, RECT *place);
typedef void (*DesktopItemTapUpCallback)(HWND hWnd, int id,
    int x, int y, RECT *place);
typedef void (*DesktopItemTapMoveCallback)(HWND hWnd, int id, int x, int y,
    RECT *place);
typedef void (*DesktopItemTapDoubleCallback)(HWND hWnd, int id, int x, int y,
    RECT *place);
typedef void (*DesktopItemTapHoldCallback)(HWND hWnd, int id, int x, int y,
    RECT *place);
typedef void (*DesktopItemKeyDownCallback)(HWND hWnd, int id, int keyCode);
typedef void (*DesktopItemKeyUpCallback)(HWND hWnd, int id, int keyCode);
typedef void (*DesktopItemPaintCallback)(HWND hWnd, int id, HDC hDC, RECT *place,
    int focused);

class DesktopItemCallbackTable {
public:
    DesktopItemCallbackTable();
    virtual ~DesktopItemCallbackTable();
    DesktopItemFocusCallback getFocusCallback() const
        { return m_focusCallback; }
    void setFocusCallback(DesktopItemFocusCallback callback)
        { m_focusCallback = callback; }
    DesktopItemBlurCallback getBlurCallback() const
        { return m_blurCallback; }
    void setBlurCallback(DesktopItemBlurCallback callback)
        { m_blurCallback = callback; }
    DesktopItemTapDownCallback getTapDownCallback() const
        { return m_tapDownCallback; }
    void setTapDownCallback(DesktopItemTapDownCallback callback)
        { m_tapDownCallback = callback; }
    DesktopItemTapUpCallback getTapUpCallback() const
        { return m_tapUpCallback; }
    void setTapUpCallback(DesktopItemTapUpCallback callback)
        { m_tapUpCallback = callback; }
    DesktopItemTapMoveCallback getTapMoveCallback() const
        { return m_tapMoveCallback; }
    void setTapMoveCallback(DesktopItemTapMoveCallback callback)
        { m_tapMoveCallback = callback; }
    DesktopItemTapDoubleCallback getTapDoubleCallback() const
        { return m_tapDoubleCallback; }
    void setTapDoubleCallback(DesktopItemTapDoubleCallback callback)
        { m_tapDoubleCallback = callback; }
    DesktopItemTapHoldCallback getTapHoldCallback() const
        { return m_tapHoldCallback; }
    void setTapHoldCallback(DesktopItemTapHoldCallback callback)
        { m_tapHoldCallback = callback; }
    DesktopItemKeyDownCallback getKeyDownCallback() const
        { return m_keyDownCallback; }
    void setKeyDownCallback(DesktopItemKeyDownCallback callback)
        { m_keyDownCallback = callback; }
    DesktopItemKeyUpCallback getKeyUpCallback() const
        { return m_keyUpCallback; }
    void setKeyUpCallback(DesktopItemKeyUpCallback callback)
        { m_keyUpCallback = callback; }
    DesktopItemPaintCallback getPaintCallback() const
        { return m_paintCallback; }
    void setPaintCallback(DesktopItemPaintCallback callback)
        { m_paintCallback = callback; }

private:
    DesktopItemFocusCallback m_focusCallback;
    DesktopItemBlurCallback m_blurCallback;
    DesktopItemTapDownCallback m_tapDownCallback;
    DesktopItemTapUpCallback m_tapUpCallback;
    DesktopItemTapMoveCallback m_tapMoveCallback;
    DesktopItemTapDoubleCallback m_tapDoubleCallback;
    DesktopItemTapHoldCallback m_tapHoldCallback;
    DesktopItemKeyDownCallback m_keyDownCallback;
    DesktopItemKeyUpCallback m_keyUpCallback;
    DesktopItemPaintCallback m_paintCallback;
};

void registerDesktopClass();
void unregisterDesktopClass();
HWND createDesktop(HWND hMainWindow);
void destroyDesktop(HWND hWnd);
void setDesktopWallpaper(HWND hWnd, const std::tstring &name, OpenerDIBSection32 *hBack);
int registerDesktopItem(HWND hWnd, bool focusable, int left, int top,
    int width, int height, const DesktopItemCallbackTable &callbackTable);
void unregisterDesktopItem(HWND hWnd, int id);
void getDesktopItemPlacement(HWND hWnd, int id, RECT &place);
void setDesktopItemPosition(HWND hWnd, int id, int flags, int left, int top,
    int width, int height);
void redrawDesktopItem(HWND hWnd, int id);
void focusDesktopItem(HWND hWnd, int id);
void setDesktopMouseCapture(HWND hWnd, int id);
void releaseDesktopMouseCapture(HWND hWnd);
HWND getDesktopMenuWindow(HWND hWnd);

#endif  // DESKTOP_H

