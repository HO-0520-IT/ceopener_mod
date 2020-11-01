#include "desktop.h"

#include <list>
#include <algorithm>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <jpeglib.h>
#include <knceutil.h>
#include "menu.h"
#include "ColorProfile.h"

#define GET_X_LPARAM(lParam) ((int)(short)LOWORD(lParam))
#define GET_Y_LPARAM(lParam) ((int)(short)HIWORD(lParam))

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    TASKBAR_HEIGHT = 24,
    DOUBLE_CLICK_SPEED = 300,
    TAP_HOLD_TIME = 500,
    TAP_HOLD_RANGE = 5,
    TIMER_HOLD_CHECK = 1
};

class DesktopItem {
public:
    DesktopItem();
    virtual ~DesktopItem();
    bool isFocusable() const { return m_focusable; }
    void setFocusable(bool focusable) { m_focusable = focusable; }
    void getPlace(RECT &place) { place = m_place; }
    void setPlace(const RECT &place) { m_place = place; }
    const DesktopItemCallbackTable &getCallbackTable() const
        { return m_callbackTable; }
    void setCallbackTable(const DesktopItemCallbackTable &table)
        { m_callbackTable = table; }
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    bool m_focusable;
    RECT m_place;
    DesktopItemCallbackTable m_callbackTable;
    int m_id;
};

static LRESULT CALLBACK desktopWndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
static LRESULT CALLBACK iconControlProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
static void onCreate(HWND hWnd, CREATESTRUCT *createStruct);
static void onDestroy(HWND hWnd);
static void onLButtonDown(HWND hWnd, int x, int y);
static void onLButtonUp(HWND hWnd, int x, int y);
static void onMouseMove(HWND hWnd, int x, int y);
static void onActivate(HWND hWnd, int active);
static void onKeyDown(HWND hWnd, int keyCode);
static void onKeyUp(HWND hWnd, int keyCode);
static void onPaint(HWND hWnd, HDC hDC);
static HBRUSH onCtlColorStatic(HWND hWnd, HDC hDC, HWND hStatic);
static void onWindowPosChanged(HWND hWnd);
static void onTimer(HWND hWnd, int idEvent);
static int obtainNewItemId(HWND hWnd);
static void drawBackground(HWND hWnd, HDC hDC, const RECT &rect);
static void jpegErrorExit(j_common_ptr cinfo);
static HBITMAP readJpegFile(HWND hWnd, const tstring &fileName);
static void changeWindowLayout(HWND hWnd);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;
extern ColorProfile g_colorProfile;
extern RECT g_EdRect;

HWND g_hDesktopWindow;

static unsigned int g_openerDesktopIconItemOpenMessage =
    RegisterWindowMessage(_T("OpenerDesktopIconItemOpen"));
static unsigned int g_openerDesktopIconItemCloseMessage =
    RegisterWindowMessage(_T("OpenerDesktopIconItemClose"));

struct DesktopData {
    HWND hMainWindow;
    HWND hMenuWindow;
    HBITMAP hDesktopBitmap;
    HBITMAP hOffscreenBitmap;
    bool isChangingWindowPosition;
    map<int, DesktopItem *> itemTable;
    list<DesktopItem *> zOrderList;
    DesktopItem *capturedItem;
    DesktopItem *focusedItem;
    DWORD prevClickTime;
    bool isTapHold;
    int tapHoldOrigin[2];
};

DesktopItemCallbackTable::DesktopItemCallbackTable() {
    m_focusCallback = NULL;
    m_blurCallback = NULL;
    m_tapDownCallback = NULL;
    m_tapUpCallback = NULL;
    m_tapMoveCallback = NULL;
    m_tapDoubleCallback = NULL;
    m_tapHoldCallback = NULL;
    m_keyDownCallback = NULL;
    m_keyUpCallback = NULL;
    m_paintCallback = NULL;
}

DesktopItemCallbackTable::~DesktopItemCallbackTable() {
}

DesktopItem::DesktopItem() {
    m_focusable = false;
    memset(&m_place, 0, sizeof(RECT));
    m_id = 0;
}

DesktopItem::~DesktopItem() {
}

void registerDesktopClass() {
    WNDCLASS windowClass;
    memset(&windowClass, 0, sizeof(WNDCLASS));
    windowClass.lpfnWndProc = desktopWndProc;
    windowClass.hInstance = g_hInstance;
    windowClass.lpszClassName = _T("ceOpenerDesktop");
    RegisterClass(&windowClass);
}

void unregisterDesktopClass() {
    UnregisterClass(_T("ceOpenerDesktop"), g_hInstance);
}

HWND createDesktop(HWND hMainWindow) {
    HWND hWnd = CreateWindowEx(
        0,
        _T("ceOpenerDesktop"), _T("ceOpener Desktop"),
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, g_hInstance, (void *)hMainWindow);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

	g_hDesktopWindow = hWnd;

    return hWnd;
}

void destroyDesktop(HWND hWnd) {
    SendMessage(hWnd, WM_CLOSE, 0, 0);
}

void setDesktopWallpaper(HWND hWnd, const tstring &name, OpenerDIBSection32 *hBack) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->hDesktopBitmap != NULL)
        DeleteObject(data->hDesktopBitmap);

    if (name.empty())
        data->hDesktopBitmap = NULL;
    else {
        tstring curDir = KnceUtil::getCurrentDirectory();

        tstring wallpaperDir = curDir + _T("\\wallpapers");

        tstring bmpFileName = wallpaperDir + _T("\\") + name;

        if (KnceUtil::matchFileExtension(bmpFileName, _T("*.bmp"))) {
            data->hDesktopBitmap =
                (HBITMAP)SHLoadDIBitmap(bmpFileName.c_str());
        }
        else {
            data->hDesktopBitmap =
                (HBITMAP)readJpegFile(hWnd, bmpFileName.c_str());
        }
		HDC hdc = GetDC(hWnd);
		OpenerDDB tmpddb(hdc, hBack->xsize(), hBack->ysize());
		SelectObject(tmpddb, data->hDesktopBitmap);
		BitBlt(*hBack, 0, 0, hBack->xsize(), hBack->ysize(), tmpddb, 0, 0, SRCCOPY);
		SelectObject(tmpddb, tmpddb);
    }

    InvalidateRect(hWnd, NULL, true);
}

int registerDesktopItem(HWND hWnd, bool focusable, int left, int top,
    int width, int height, const DesktopItemCallbackTable &callbackTable) {

    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    RECT place = {left, top, left + width, top + height};

    DesktopItem *item = new DesktopItem();
    item->setFocusable(focusable);
    item->setPlace(place);
    item->setCallbackTable(callbackTable);

    int id = obtainNewItemId(hWnd);

    item->setId(id);

    data->itemTable[id] = item;
    data->zOrderList.push_back(item);

    InvalidateRect(hWnd, &place, true);

    return id;
}

void unregisterDesktopItem(HWND hWnd, int id) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->itemTable.find(id) == data->itemTable.end())
        return;

    DesktopItem *item = data->itemTable[id];

    list<DesktopItem *>::iterator iter = find(data->zOrderList.begin(),
        data->zOrderList.end(), item);
    data->zOrderList.erase(iter);

    if (data->focusedItem == item) {
        data->focusedItem = NULL;

        DesktopItemBlurCallback callback =
            item->getCallbackTable().getBlurCallback();
        if (callback != NULL)
            callback(data->hMainWindow, item->getId());
    }

    RECT place = {0};
    item->getPlace(place);
    InvalidateRect(hWnd, &place, true);

    delete item;

    data->itemTable.erase(id);
}

void getDesktopItemPlacement(HWND hWnd, int id, RECT &place) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->itemTable.find(id) == data->itemTable.end())
        return;

    DesktopItem *item = data->itemTable[id];

    item->getPlace(place);
}

void setDesktopItemPosition(HWND hWnd, int id, int flags, int left, int top,
    int width, int height) {

    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->itemTable.find(id) == data->itemTable.end())
        return;

    DesktopItem *item = data->itemTable[id];

    RECT orgPlace = {0};
    item->getPlace(orgPlace);

    RECT newPlace = {0};
    if ((flags & DESKTOP_ITEM_NO_MOVE) == 0) {
        newPlace.left = left;
        newPlace.top = top;
    }
    else {
        newPlace.left = orgPlace.left;
        newPlace.top = orgPlace.top;
    }

    if ((flags & DESKTOP_ITEM_NO_RESIZE) == 0) {
        newPlace.right = newPlace.left + width;
        newPlace.bottom = newPlace.top + height;
    }
    else {
        newPlace.right = newPlace.left + (orgPlace.right - orgPlace.left);
        newPlace.bottom = newPlace.top + (orgPlace.bottom - orgPlace.top);
    }

    item->setPlace(newPlace);

    InvalidateRect(hWnd, &orgPlace, true);
    InvalidateRect(hWnd, &newPlace, true);
}

void redrawDesktopItem(HWND hWnd, int id) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->itemTable.find(id) == data->itemTable.end())
        return;

    DesktopItem *item = data->itemTable[id];

    RECT place = {0};
    item->getPlace(place);
    InvalidateRect(hWnd, &place, true);
}

void focusDesktopItem(HWND hWnd, int id) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->itemTable.find(id) == data->itemTable.end())
        return;

    DesktopItem *item = data->itemTable[id];

    if (item->isFocusable()) {
        data->focusedItem = item;

        data->zOrderList.erase(find(data->zOrderList.begin(),
            data->zOrderList.end(), item));
        data->zOrderList.push_back(item);

        DesktopItemFocusCallback callback =
            item->getCallbackTable().getFocusCallback();
        if (callback != NULL)
            callback(data->hMainWindow, item->getId());

        RECT place = {0};
        item->getPlace(place);
        InvalidateRect(hWnd, &place, true);
    }
}

void setDesktopMouseCapture(HWND hWnd, int id) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->itemTable.find(id) == data->itemTable.end())
        return;

    data->capturedItem = data->itemTable[id];
}

void releaseDesktopMouseCapture(HWND hWnd) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    data->capturedItem = NULL;
}

HWND getDesktopMenuWindow(HWND hWnd) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    return data->hMenuWindow;
}

static LRESULT CALLBACK desktopWndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam) {

	switch (msg) {
    case WM_CREATE:
        onCreate(hWnd, (CREATESTRUCT *)lParam);
        return 0;
    case WM_DESTROY:
        onDestroy(hWnd);
		return 0;
    case WM_LBUTTONDOWN:
        onLButtonDown(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_LBUTTONUP:
        onLButtonUp(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_MOUSEMOVE:
        onMouseMove(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return 0;
    case WM_COMMAND:
    {
        int id    = LOWORD(wParam); 
        int event = HIWORD(wParam);

        //switch (id) {
        //}

        return 0;
    }
    case WM_ACTIVATE:
        onActivate(hWnd, wParam & 0xFFFF);
        return 0;
    case WM_KEYDOWN:
        onKeyDown(hWnd, wParam);
        return 0;
    case WM_KEYUP:
        onKeyUp(hWnd, wParam);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paintStruct;
        HDC hDC = BeginPaint(hWnd, &paintStruct);
        onPaint(hWnd, (HDC)hDC);
        EndPaint(hWnd, &paintStruct);

        return 0;
    }
    case WM_WINDOWPOSCHANGED:
        onWindowPosChanged(hWnd);
        return 0;
    case WM_TIMER:
        onTimer(hWnd, wParam);
        return 0;
	}

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void onCreate(HWND hWnd, CREATESTRUCT *createStruct) {
    DesktopData *data = new DesktopData();
    SetWindowLong(hWnd, GWL_USERDATA, (long)data);

    data->hMainWindow = (HWND)createStruct->lpCreateParams;
    data->hDesktopBitmap = NULL;
    data->hOffscreenBitmap = NULL;
    data->isChangingWindowPosition = false;
    data->capturedItem = NULL;
    data->focusedItem = NULL;
    data->prevClickTime = 0;
    data->isTapHold = false;
    data->tapHoldOrigin[0] = data->tapHoldOrigin[1] = 0;

    MenuParams menuParams;
    menuParams.setMain(false);

    data->hMenuWindow = createMenu(data->hMainWindow, menuParams);

    changeWindowLayout(hWnd);
}

static void onDestroy(HWND hWnd) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    list<DesktopItem *>::iterator iter = data->zOrderList.begin();
    for ( ; iter != data->zOrderList.end(); iter++)
        delete *iter;

    if (data->hDesktopBitmap != NULL)
        DeleteObject(data->hDesktopBitmap);

    if (data->hOffscreenBitmap != NULL)
        DeleteObject(data->hOffscreenBitmap);

    destroyMenu(data->hMenuWindow);

    delete data;
}

static void onLButtonDown(HWND hWnd, int x, int y) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    RECT place = {0};
    DesktopItem *foundItem = NULL;

    if (data->capturedItem == NULL) {
        list<DesktopItem *>::reverse_iterator riter = data->zOrderList.rbegin();

        for ( ; riter != data->zOrderList.rend(); riter++) {
            DesktopItem *item = *riter;
            item->getPlace(place);

            if (x >= place.left && x < place.right &&
                y >= place.top && y < place.bottom) {

                foundItem = item;
                break;
            }
        }
    }
    else
        foundItem = data->capturedItem;

    if (foundItem != data->focusedItem) {
        if (data->focusedItem != NULL) {
            DesktopItem *prevFocusedItem = data->focusedItem;
            data->focusedItem = NULL;

            DesktopItemBlurCallback callback =
                prevFocusedItem->getCallbackTable().getBlurCallback();
            if (callback != NULL)
                callback(data->hMainWindow, prevFocusedItem->getId());

            prevFocusedItem->getPlace(place);
            InvalidateRect(hWnd, &place, true);
        }

        if (foundItem != NULL && foundItem->isFocusable()) {
            data->focusedItem = foundItem;

            data->zOrderList.erase(find(data->zOrderList.begin(),
                data->zOrderList.end(), foundItem));
            data->zOrderList.push_back(foundItem);

            DesktopItemFocusCallback callback =
                foundItem->getCallbackTable().getFocusCallback();
            if (callback != NULL)
                callback(data->hMainWindow, foundItem->getId());

            foundItem->getPlace(place);
            InvalidateRect(hWnd, &place, true);
        }
    }

    DWORD clickTime = GetTickCount();
    bool doubleClicked = (signed)(clickTime - data->prevClickTime) <
        DOUBLE_CLICK_SPEED;

    if (foundItem == NULL) {
        if (doubleClicked)
            ;   // do nothing currently
        else
            data->prevClickTime = clickTime;
    }
    else {
        if (doubleClicked) {
            DesktopItemTapDoubleCallback callback =
                foundItem->getCallbackTable().getTapDoubleCallback();
            if (callback != NULL) {
                foundItem->getPlace(place);
                callback(data->hMainWindow, foundItem->getId(), x, y, &place);
            }
        }
        else {
            DesktopItemTapDownCallback callback =
                foundItem->getCallbackTable().getTapDownCallback();
            if (callback != NULL) {
                foundItem->getPlace(place);
                callback(data->hMainWindow, foundItem->getId(), x, y, &place);
            }

            data->prevClickTime = clickTime;
        }
    }

    if (!doubleClicked) {
        SetTimer(hWnd, TIMER_HOLD_CHECK, TAP_HOLD_TIME, NULL);
        data->isTapHold = true;
        data->tapHoldOrigin[0] = x;
        data->tapHoldOrigin[1] = y;
    }
}

static void onLButtonUp(HWND hWnd, int x, int y) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    RECT place = {0};
    DesktopItem *foundItem = NULL;

    if (data->capturedItem == NULL) {
        list<DesktopItem *>::reverse_iterator riter = data->zOrderList.rbegin();

        for ( ; riter != data->zOrderList.rend(); riter++) {
            DesktopItem *item = *riter;
            item->getPlace(place);

            if (x >= place.left && x < place.right &&
                y >= place.top && y < place.bottom) {

                foundItem = item;
                break;
            }
        }
    }
    else
        foundItem = data->capturedItem;

    if (foundItem != NULL) {
        DesktopItemTapUpCallback callback =
            foundItem->getCallbackTable().getTapUpCallback();
        if (callback != NULL) {
            foundItem->getPlace(place);
            callback(data->hMainWindow, foundItem->getId(), x, y, &place);
        }
    }

    if (data->isTapHold) {
        KillTimer(hWnd, TIMER_HOLD_CHECK);
        data->isTapHold = false;
    }
}

static void onMouseMove(HWND hWnd, int x, int y) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    RECT place = {0};
    DesktopItem *foundItem = NULL;

    if (data->capturedItem == NULL) {
        list<DesktopItem *>::reverse_iterator riter = data->zOrderList.rbegin();

        for ( ; riter != data->zOrderList.rend(); riter++) {
            DesktopItem *item = *riter;
            item->getPlace(place);

            if (x >= place.left && x < place.right &&
                y >= place.top && y < place.bottom) {

                foundItem = item;
                break;
            }
        }
    }
    else
        foundItem = data->capturedItem;

    if (foundItem != NULL) {
        DesktopItemTapMoveCallback callback =
            foundItem->getCallbackTable().getTapMoveCallback();
        if (callback != NULL) {
            foundItem->getPlace(place);
            callback(data->hMainWindow, foundItem->getId(), x, y, &place);
        }
    }

    if (data->isTapHold) {
        int dx = x - data->tapHoldOrigin[0];
        int dy = y - data->tapHoldOrigin[1];

        if (dx * dx + dy * dy > TAP_HOLD_RANGE * TAP_HOLD_RANGE) {
            KillTimer(hWnd, TIMER_HOLD_CHECK);
            data->isTapHold = false;
        }
    }
}

static void onActivate(HWND hWnd, int active) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (active == WA_INACTIVE) {
        if (data->focusedItem != NULL) {
            DesktopItem *prevFocusedItem = data->focusedItem;
            data->focusedItem = NULL;

            DesktopItemBlurCallback callback =
                prevFocusedItem->getCallbackTable().getBlurCallback();
            if (callback != NULL)
                callback(data->hMainWindow, prevFocusedItem->getId());

            RECT place = {0};
            prevFocusedItem->getPlace(place);
            InvalidateRect(hWnd, &place, true);
        }
    }
}

static void onKeyDown(HWND hWnd, int keyCode) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->focusedItem != NULL) {
        DesktopItemKeyDownCallback callback =
            data->focusedItem->getCallbackTable().getKeyDownCallback();
        if (callback != NULL)
            callback(data->hMainWindow, data->focusedItem->getId(), keyCode);
    }
}

static void onKeyUp(HWND hWnd, int keyCode) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->focusedItem != NULL) {
        DesktopItemKeyUpCallback callback =
            data->focusedItem->getCallbackTable().getKeyUpCallback();
        if (callback != NULL)
            callback(data->hMainWindow, data->focusedItem->getId(), keyCode);
    }
}

static void onPaint(HWND hWnd, HDC hDC) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    RECT clip;
    GetClipBox(hDC, &clip);

    HDC hOffscrDC = CreateCompatibleDC(hDC);
    SelectObject(hOffscrDC, data->hOffscreenBitmap);

    drawBackground(hWnd, hOffscrDC, clip);

    RECT place = {0};
    list<DesktopItem *>::iterator iter = data->zOrderList.begin();

    for ( ; iter != data->zOrderList.end(); iter++) {
        DesktopItem *item = *iter;
        item->getPlace(place);

        if (place.left < clip.right && place.right > clip.left &&
            place.top < clip.bottom && place.bottom > clip.top) {

            DesktopItemPaintCallback callback =
                item->getCallbackTable().getPaintCallback();
            if (callback != NULL) {
                callback(data->hMainWindow, item->getId(), hOffscrDC, &place,
                    item == data->focusedItem);
            }
        }
    }

    BitBlt(hDC, clip.left, clip.top, clip.right - clip.left,
        clip.bottom - clip.top, hOffscrDC, clip.left, clip.top, SRCCOPY);

    DeleteObject(hOffscrDC);
}

static void onWindowPosChanged(HWND hWnd) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (data->isChangingWindowPosition)
        return;

    HWND hCeDesktopWindow = FindWindow(_T("DesktopExplorerWindow"), NULL);
    HWND hHhTaskBar = FindWindow(_T("HHTaskBar"), NULL);
    HWND hEdStatusPower = FindWindow(_T("EdStatusPower"), NULL);

    HWND hFrontWindow = GetWindow(hCeDesktopWindow, GW_HWNDPREV);
    while (hFrontWindow != NULL) {
        if (IsWindowVisible(hFrontWindow) &&
            hFrontWindow != hHhTaskBar && hFrontWindow != hEdStatusPower &&
            (GetWindowLong(hFrontWindow, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0) {

            break;
        }

        hFrontWindow = GetWindow(hFrontWindow, GW_HWNDPREV);
    }

    if (hFrontWindow == hWnd)
        return;

    data->isChangingWindowPosition = true;

    if (hFrontWindow == NULL)
        hFrontWindow = HWND_TOP;

    SetWindowPos(hWnd, hFrontWindow, 0, 0, 0, 0,
        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    data->isChangingWindowPosition = false;
}

static void onTimer(HWND hWnd, int idEvent) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (idEvent == TIMER_HOLD_CHECK) {
        if (!data->isTapHold)
            return;

        int x = data->tapHoldOrigin[0];
        int y = data->tapHoldOrigin[1];

        RECT place = {0};
        DesktopItem *foundItem = NULL;

        if (data->capturedItem == NULL) {
            list<DesktopItem *>::reverse_iterator riter =
                data->zOrderList.rbegin();

            for ( ; riter != data->zOrderList.rend(); riter++) {
                DesktopItem *item = *riter;
                item->getPlace(place);

                if (x >= place.left && x < place.right &&
                    y >= place.top && y < place.bottom) {

                    foundItem = item;
                    break;
                }
            }
        }
        else
            foundItem = data->capturedItem;

        // turn timer off before calling handler
        KillTimer(hWnd, TIMER_HOLD_CHECK);
        data->isTapHold = false;

        if (foundItem == NULL) {
            POINT pt = {x, y};
            ClientToScreen(hWnd, &pt);

            RECT rect = {x, y, x, y};
            setMenuAttachedRect(data->hMenuWindow, &rect);

            showMenu(data->hMenuWindow);
        }
        else {
            DesktopItemTapHoldCallback callback =
                foundItem->getCallbackTable().getTapHoldCallback();
            if (callback != NULL) {
                foundItem->getPlace(place);
                callback(data->hMainWindow, foundItem->getId(), x, y, &place);
            }
        }
    }
}

static int obtainNewItemId(HWND hWnd) {
    const int MIN_ID = 101;
    const int MAX_ID = MIN_ID + 1000;
    static int currentId = MIN_ID;

    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    while (data->itemTable.find(currentId) != data->itemTable.end()) {
        if (++currentId > MAX_ID)
            currentId = MIN_ID;
    }

    int retId = currentId;
    if (++currentId > MAX_ID)
        currentId = MIN_ID;

    return retId;
}

static void drawBackground(HWND hWnd, HDC hDC, const RECT &rect) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    COLORREF desktopColor = g_colorProfile.getColor(_T("desktop"));

    if (data->hDesktopBitmap == NULL) {
        HBRUSH hBrush = CreateSolidBrush(desktopColor);
        FillRect(hDC, &rect, hBrush);
        DeleteObject(hBrush);
    }
    else {
        HDC hOffscrDC = CreateCompatibleDC(hDC);
        HANDLE hPrevBmp = SelectObject(hOffscrDC, data->hDesktopBitmap);

        BitBlt(hDC, rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top,
            hOffscrDC, rect.left, rect.top, SRCCOPY);

        SelectObject(hOffscrDC, hPrevBmp);

        DeleteObject(hOffscrDC);
    }
}

struct JpegErrorManager {
    struct jpeg_error_mgr pub;

    jmp_buf setjmp_buffer;
};

static void jpegErrorExit(j_common_ptr cinfo) {
    JpegErrorManager *myerr = (JpegErrorManager *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

static HBITMAP readJpegFile(HWND hWnd, const tstring &fileName)
{
    FILE * infile;
    if ((infile = _tfopen(fileName.c_str(), _T("rb"))) == NULL) {
        fprintf(stderr, "can't open %s\n", fileName.c_str());
        return 0;
    }

    struct jpeg_decompress_struct cinfo;
    struct JpegErrorManager jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return 0;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);

    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    int width = cinfo.output_width;
    int height = cinfo.output_height;

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    int lineBytes = ((((width * 24) + 31) & ~31) >> 3);
    bmi.bmiHeader.biSizeImage = lineBytes * height;

    PAINTSTRUCT paintStruct;
    HDC hDC = BeginPaint(hWnd, &paintStruct);

    unsigned char *buff = NULL;
    HBITMAP hBitmap = CreateDIBSection((HDC)hDC, &bmi, DIB_RGB_COLORS,
        (void **)&buff, NULL, 0);

    EndPaint(hWnd, &paintStruct);

    long offset = lineBytes * (height - 1);
    while (cinfo.output_scanline < cinfo.output_height) {
        JSAMPROW praw = (JSAMPROW)(buff + offset);
        jpeg_read_scanlines(&cinfo, (JSAMPARRAY)&praw, 1);

        int swapOffset = 0;
        for ( ; swapOffset < lineBytes; swapOffset += 3) {
            JSAMPROW swapPraw = praw + swapOffset;
            unsigned char t = swapPraw[0];
            swapPraw[0] = swapPraw[2];
            swapPraw[2] = t;
        }

        offset -= lineBytes;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(infile);

    return hBitmap;
}

static void changeWindowLayout(HWND hWnd) {
    DesktopData *data = (DesktopData *)GetWindowLong(hWnd, GWL_USERDATA);

    RECT desktopRect;
    desktopRect = g_EdRect;

    MoveWindow(hWnd, 0, 0, desktopRect.right, desktopRect.bottom,
        false);

    if (data->hOffscreenBitmap != NULL)
        DeleteObject(data->hOffscreenBitmap);

    HDC hDC = GetDC(hWnd);
    data->hOffscreenBitmap = CreateCompatibleBitmap(hDC, desktopRect.right,
        desktopRect.bottom);
    ReleaseDC(hWnd, hDC);
}
