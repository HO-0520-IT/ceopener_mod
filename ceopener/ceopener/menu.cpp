#include "menu.h"

#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>
#include <commctrl.h>
#include "ColorProfile.h"
#include "DrawUtil.h"

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {	//âÊñ ÉTÉCÉYí≤êÆèëÇ´ä∑Ç¶çœÇ›
    IDC_MENU_ITEM_BUTTON_BASE = 1001,
    MENU_ITEM_MARGIN = 2,
    MENU_ITEM_WIDTH = 150,
    MENU_SIDE_WIDTH = 4,
    MENU_WIDTH = MENU_ITEM_WIDTH + 4
};

class MenuItem {
public:
    MenuItem();
    virtual ~MenuItem();
    HWND getButton() const { return m_hButton; }
    void setButton(HWND hButton) { m_hButton = hButton; }
    int getButtonId() const { return m_buttonId; }
    void setButtonId(int id) { m_buttonId = id; }
    HWND getSubMenuWindow() const { return m_hSubMenuWindow; }
    void setSubMenuWindow(HWND hSubMenuWindow)
        { m_hSubMenuWindow = hSubMenuWindow; }
    tstring getCaption() const { return m_caption; }
    void setCaption(const tstring &capt) { m_caption = capt; }

private:
    HWND m_hButton;
    int m_buttonId;
    HWND m_hSubMenuWindow;
    tstring m_caption;
};

void hideMenuAscendant(HWND hWnd, bool isForced);
void setParentMenuWindow(HWND hWnd, HWND hParentMenuWindow);
void setMenuAttachedRect(HWND hWnd, const RECT *rect);
static LRESULT CALLBACK menuWndProc(HWND hWnd, UINT msg, WPARAM wp,
    LPARAM lp);
static LRESULT CALLBACK menuItemButtonProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam);
static void onCreate(HWND hWnd, CREATESTRUCT *createStruct);
static void onDestroy(HWND hWnd);
static void onMenuItem(HWND hWnd, int id);
static void onExit(HWND hWnd);
static void onCloseEd(HWND hWnd);
static void onActivate(HWND hWnd, int active, HWND hwndPre);
static void onKeyDown(HWND hWnd, int keyCode);
static void onPaint(HWND hWnd, HDC hDC);
static void onDrawItem(HWND hWnd, DRAWITEMSTRUCT *drawItem);
static void showSubMenu(HWND hWnd, HWND hItemButton, bool firstFocused);
static void hideThisMenu(HWND hWnd);
static void selectFirstOrLastItem(HWND hWnd, bool first);
static int obtainNewMenuItemButtonId(HWND hWnd);
static void changeWindowLayout(HWND hWnd);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;
extern ColorProfile g_colorProfile;
extern int TASKBAR_HEIGHT;
int MENU_ITEM_HEIGHT_MAIN = 24;
int MENU_ITEM_HEIGHT_SUB = 20;

struct MenuData {
    HWND hMainWindow;
    bool isMain;
    WNDPROC prevMenuItemButtonProc;
    std::vector<MenuItem *> menuItems;
    HWND hParentMenuWindow;
    RECT attachedRect;
    HWND hSelectedMenuItemButton;
    int offsetIndex;
    HWND hUpperScrollButton;
    HWND hLowerScrollButton;
};

MenuParams::MenuParams() {
    m_isMain = false;
}

MenuParams::~MenuParams() {
}

MenuItem::MenuItem() {
    m_hButton = NULL;
    m_buttonId = 0;
    m_hSubMenuWindow = NULL;
}

MenuItem::~MenuItem() {
}

void registerMenuClass() {
    WNDCLASS windowClass;
    memset(&windowClass, 0, sizeof(WNDCLASS));
    windowClass.lpfnWndProc = menuWndProc;
    windowClass.hInstance = g_hInstance;
    windowClass.lpszClassName = _T("ceOpenerMenu");
    RegisterClass(&windowClass);
}

void unregisterMenuClass() {
    UnregisterClass(_T("ceOpenerMenu"), g_hInstance);
}

HWND createMenu(HWND hMainWindow, MenuParams &params) {
	MENU_ITEM_HEIGHT_MAIN = TASKBAR_HEIGHT;
	MENU_ITEM_HEIGHT_SUB = TASKBAR_HEIGHT / 6 * 5;
    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST,
        _T("ceOpenerMenu"), _T("ceOpener Menu"),
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, g_hInstance,
        (void *)&pair<HWND, MenuParams *>(hMainWindow, &params));

    //ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    return hWnd;
}

void destroyMenu(HWND hWnd) {
    SendMessage(hWnd, WM_CLOSE, 0, 0);
}

void showMenu(HWND hWnd) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    data->hSelectedMenuItemButton = NULL;
    data->offsetIndex = 0;
    data->hUpperScrollButton = NULL;
    data->hLowerScrollButton = NULL;

    changeWindowLayout(hWnd);

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);

    SendMessage(data->hMainWindow, RegisterWindowMessage(
        _T("OpenerMenuShow")), (WPARAM)hWnd, 0);
}

void hideMenu(HWND hWnd) {
    int i;

    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    ShowWindow(hWnd, SW_HIDE);

    int numItems = data->menuItems.size();
    for (i = 0; i < numItems; i++) {
        HWND hSubMenuWindow = data->menuItems[i]->getSubMenuWindow();
        if (hSubMenuWindow != NULL)
            hideMenu(hSubMenuWindow);
    }
}

void clearMenu(HWND hWnd) {
    int i;

    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    int numItems = data->menuItems.size();
    for (i = 0; i < numItems; i++) {
        MenuItem *item = data->menuItems[i];
        DestroyWindow(item->getButton());
        delete item;
    }

    data->menuItems.clear();
}

int appendMenuItem(HWND hWnd, const tstring &capt, HWND hSubMenuWindow) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    int id = obtainNewMenuItemButtonId(hWnd);

    int menuItemHeight = data->isMain ? MENU_ITEM_HEIGHT_MAIN :
        MENU_ITEM_HEIGHT_SUB;

    HWND hItemButton = CreateWindow(_T("BUTTON"), capt.c_str(),
        BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        CW_USEDEFAULT, CW_USEDEFAULT, MENU_ITEM_WIDTH, menuItemHeight,
        hWnd, (HMENU)id, g_hInstance, NULL);

    data->prevMenuItemButtonProc = (WNDPROC)SetWindowLong(hItemButton,
        GWL_WNDPROC, (DWORD)menuItemButtonProc);

    SendMessage(hItemButton, WM_SETFONT, (WPARAM)g_hMainFont, 0);

    if (data->isMain) {
        SetWindowPos(hItemButton, HWND_TOP, 0, 0, 0, 0,
            SWP_NOSIZE | SWP_NOMOVE);
    }

    MenuItem *item = new MenuItem();
    item->setButton(hItemButton);
    item->setButtonId(id);
    item->setSubMenuWindow(hSubMenuWindow);
    item->setCaption(capt);

    SetWindowLong(hItemButton, GWL_USERDATA, (long)item);

    if (data->isMain)
        data->menuItems.insert(data->menuItems.begin(), item);
    else
        data->menuItems.push_back(item);

    if (hSubMenuWindow != NULL)
        setParentMenuWindow(hSubMenuWindow, hWnd);

    changeWindowLayout(hWnd);

    return id;
}

void removeMenuItem(HWND hWnd, int id) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    HWND hItemButton = GetDlgItem(hWnd, id);

    MenuItem *item = (MenuItem *)GetWindowLong(hItemButton, GWL_USERDATA);
    delete item;

    DestroyWindow(hItemButton);

    data->menuItems.erase(find(data->menuItems.begin(), data->menuItems.end(),
        item));

    changeWindowLayout(hWnd);
}

void setMenuItemCaption(HWND hWnd, int id, const tstring &capt) {
    HWND hItemButton = GetDlgItem(hWnd, id);
    SetWindowText(hItemButton, capt.c_str());

    MenuItem *item = (MenuItem *)GetWindowLong(hItemButton, GWL_USERDATA);
    item->setCaption(capt);
}

void hideMenuAscendant(HWND hWnd, bool isForced) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    ShowWindow(hWnd, SW_HIDE);

    if (data->hParentMenuWindow != NULL) {
        if (isForced || data->hParentMenuWindow != GetForegroundWindow())
            hideMenuAscendant(data->hParentMenuWindow, isForced);
    }
}

void setParentMenuWindow(HWND hWnd, HWND hParentMenuWindow) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    data->hParentMenuWindow = hParentMenuWindow;
}

void setMenuAttachedRect(HWND hWnd, const RECT *rect) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    data->attachedRect = *rect;
}

LRESULT CALLBACK menuWndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam) {

	switch (msg) {
    case WM_CREATE:
        onCreate(hWnd, (CREATESTRUCT *)lParam);
        return 0;
    case WM_DESTROY:
        onDestroy(hWnd);
		return 0;
    case WM_COMMAND:
    {
        int id    = LOWORD(wParam); 
        int event = HIWORD(wParam);

        if (id >= IDC_MENU_ITEM_BUTTON_BASE)
            onMenuItem(hWnd, id);

        return 0;
    }
    case WM_ACTIVATE:
        onActivate(hWnd, wParam & 0xFFFF, (HWND)lParam);
        return 0;
    case WM_KEYDOWN:
        onKeyDown(hWnd, wParam);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT paintStruct;
        HDC hDC = BeginPaint(hWnd, &paintStruct);
        onPaint(hWnd, hDC);
        EndPaint(hWnd, &paintStruct);

        return 0;
    }
    case WM_DRAWITEM:
        onDrawItem(hWnd, (DRAWITEMSTRUCT *)lParam);
        return 0;
    //case WM_HOTKEY:
    //    onHotKey(wParam);
    //    return 0;
	}

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK menuItemButtonProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam) {

    MenuData *data = (MenuData *)GetWindowLong(GetParent(hWnd), GWL_USERDATA);

	switch (msg) {
    case WM_KEYDOWN:
    {
        int keyCode = wParam;
        if (keyCode == VK_LEFT || keyCode == VK_ESCAPE)
            hideThisMenu(GetParent(hWnd));
        else if (keyCode == VK_RIGHT)
            showSubMenu(GetParent(hWnd), hWnd, true);
        else if (keyCode == VK_UP) {
            HWND hPrev = hWnd;
            do {
                hPrev = GetWindow(hPrev, GW_HWNDPREV);
            } while (!IsWindowVisible(hPrev) && hPrev != NULL);

            if (hPrev == NULL)
                selectFirstOrLastItem(GetParent(hWnd), false);
            else {
                if (hPrev == data->hUpperScrollButton) {
                    data->offsetIndex--;
                    changeWindowLayout(GetParent(hWnd));
                }

                SetFocus(hPrev);
            }

            return 0;
        }
        else if (keyCode == VK_DOWN) {
            HWND hNext = hWnd;
            do {
                hNext = GetWindow(hNext, GW_HWNDNEXT);
            } while (!IsWindowVisible(hNext) && hNext != NULL);

            if (hNext == NULL)
                selectFirstOrLastItem(GetParent(hWnd), true);
            else {
                if (hNext == data->hLowerScrollButton) {
                    data->offsetIndex++;
                    changeWindowLayout(GetParent(hWnd));
                }

                SetFocus(hNext);
            }

            return 0;
        }
        else if (keyCode == VK_RETURN) {
            SendMessage(GetParent(hWnd), WM_COMMAND,
                (BN_CLICKED << 16) | GetDlgCtrlID(hWnd), (LPARAM)hWnd);
            return 0;
        }
    }
    }

    return CallWindowProc(data->prevMenuItemButtonProc, hWnd, msg, wParam,
        lParam);
}

static void onCreate(HWND hWnd, CREATESTRUCT *createStruct) {
    MenuData *data = new MenuData();
    SetWindowLong(hWnd, GWL_USERDATA, (long)data);

    pair<HWND, MenuParams *> *createParams =
        (pair<HWND, MenuParams *> *)createStruct->lpCreateParams;
    data->hMainWindow = createParams->first;

    MenuParams *params = createParams->second;
    data->isMain = params->isMain();

    data->prevMenuItemButtonProc = NULL;
    data->hParentMenuWindow = NULL;
    memset(&data->attachedRect, 0, sizeof(RECT));

    data->hSelectedMenuItemButton = NULL;
    data->offsetIndex = 0;
    data->hUpperScrollButton = NULL;
    data->hLowerScrollButton = NULL;

    changeWindowLayout(hWnd);
}

static void onDestroy(HWND hWnd) {
    int i;

    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    int numItems = data->menuItems.size();
    for (i = 0; i < numItems; i++) {
        MenuItem *item = data->menuItems[i];
        DestroyWindow(item->getButton());
        delete item;
    }

    delete data;
}

static void onMenuItem(HWND hWnd, int id) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    HWND hItemButton = GetDlgItem(hWnd, id);
    if (hItemButton == data->hUpperScrollButton ||
        hItemButton == data->hLowerScrollButton) {

        if (hItemButton == data->hUpperScrollButton)
            data->offsetIndex--;
        else
            data->offsetIndex++;

        changeWindowLayout(hWnd);

        SetFocus(hWnd);
    }
    else {
        MenuItem *item = (MenuItem *)GetWindowLong(hItemButton, GWL_USERDATA);

        HWND hSubMenuWindow = item->getSubMenuWindow();
        if (hSubMenuWindow == NULL) {
            PostMessage(data->hMainWindow, RegisterWindowMessage(
                _T("OpenerMenuItemSelected")), (WPARAM)hWnd, id);

            hideMenuAscendant(hWnd, true);
        }
        else
            showSubMenu(hWnd, hItemButton, false);
    }
}

static void onActivate(HWND hWnd, int active, HWND hwndPre) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    if (active == WA_INACTIVE) {
        if (hwndPre != data->hMainWindow)
            hideThisMenu(hWnd);
    }
    else {
        if (data->hSelectedMenuItemButton != NULL) {
            SetFocus(data->hSelectedMenuItemButton);
            data->hSelectedMenuItemButton = NULL;
        }
    }
}

static void onKeyDown(HWND hWnd, int keyCode) {
    if (keyCode == VK_LEFT || keyCode == VK_ESCAPE)
        hideThisMenu(hWnd);
    else if (keyCode == VK_UP)
        selectFirstOrLastItem(hWnd, false);
    else if (keyCode == VK_DOWN)
        selectFirstOrLastItem(hWnd, true);
}

static void onPaint(HWND hWnd, HDC hDC) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    COLORREF menuFaceColor = g_colorProfile.getColor(_T("menuFace"));
    COLORREF menuShadowColor = g_colorProfile.getColor(_T("menuShadow"));

    RECT rect;
    GetClientRect(hWnd, &rect);

    DrawUtil::drawPanel(hDC, rect, menuShadowColor);

    if (data->menuItems.size() == 0) {
        int menuItemHeight = data->isMain ? MENU_ITEM_HEIGHT_MAIN :
            MENU_ITEM_HEIGHT_SUB;

        RECT itemRect = {0, 0, MENU_ITEM_WIDTH, menuItemHeight};
        OffsetRect(&itemRect, MENU_ITEM_MARGIN, MENU_ITEM_MARGIN);

        HBRUSH hBrush = CreateSolidBrush(menuFaceColor);

        FillRect(hDC, &itemRect, hBrush);

        DeleteObject(hBrush);

        HFONT prevFont = (HFONT)SelectObject(hDC, g_hMainFont);

        int prevBkMode = SetBkMode(hDC, TRANSPARENT);
        COLORREF prevTextColor = SetTextColor(hDC, RGB(128, 128, 128));

        InflateRect(&itemRect, -MENU_ITEM_MARGIN * 2, 0);
        DrawText(hDC, _T("(Empty)"), -1, &itemRect, DT_LEFT | DT_VCENTER |
            DT_SINGLELINE);

        SetTextColor(hDC, prevTextColor);
        SetBkMode(hDC, prevBkMode);

        SelectObject(hDC, prevFont);
    }
}

static void onDrawItem(HWND hWnd, DRAWITEMSTRUCT *drawItem) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    COLORREF normalTextColor = g_colorProfile.getColor(_T("text"));
    COLORREF menuFaceColor = g_colorProfile.getColor(_T("menuFace"));
    COLORREF selectedItemColor = g_colorProfile.getColor(_T("selectedItem"));
    COLORREF selectedTextColor = g_colorProfile.getColor(_T("selectedText"));

    HWND hItemButton = drawItem->hwndItem;
    bool focused = (drawItem->itemState & ODS_FOCUS) != 0 ||
        hItemButton == data->hSelectedMenuItemButton;

    HDC hItemDC = drawItem->hDC;
    RECT &itemRect = drawItem->rcItem;

    HBRUSH hBrush = CreateSolidBrush(focused ? selectedItemColor :
        menuFaceColor);
    HBRUSH hPrevBrush = (HBRUSH)SelectObject(hItemDC, hBrush);

    PatBlt(hItemDC, 0, 0, itemRect.right, itemRect.bottom, PATCOPY);

    SelectObject(hItemDC, hPrevBrush);
    DeleteObject(hBrush);

    COLORREF textColor = focused ? selectedTextColor : normalTextColor;

    if (hItemButton == data->hUpperScrollButton ||
        hItemButton == data->hLowerScrollButton) {

        POINT center = {(itemRect.left + itemRect.right) / 2,
            (itemRect.top + itemRect.bottom) / 2};

        if (hItemButton == data->hUpperScrollButton) {
            DrawUtil::drawTriangle(hItemDC, center,
				DrawUtil::DRAW_TRIANGLE_NORTH, textColor);
        }
        else {
            DrawUtil::drawTriangle(hItemDC, center,
				DrawUtil::DRAW_TRIANGLE_SOUTH, textColor);
        }
    }
    else {
        TCHAR buf[256];
        GetWindowText(hItemButton, buf, 256);

        COLORREF prevTextColor = SetTextColor(hItemDC, focused ?
            selectedTextColor : textColor);
        int prevBkMode = SetBkMode(hItemDC, TRANSPARENT);

        InflateRect(&itemRect, -MENU_ITEM_MARGIN * 2, 0);
        DrawText(hItemDC, buf, -1, &itemRect, DT_LEFT | DT_VCENTER |
            DT_SINGLELINE);

        SetTextColor(hItemDC, prevTextColor);
        SetBkMode(hItemDC, prevBkMode);
    }

    MenuItem *item = (MenuItem *)GetWindowLong(hItemButton, GWL_USERDATA);
    if (item->getSubMenuWindow() != NULL) {
        POINT center = {itemRect.right - 5,
            (itemRect.top + itemRect.bottom) / 2};

		DrawUtil::drawTriangle(hItemDC, center, DrawUtil::DRAW_TRIANGLE_EAST,
            textColor);
    }
}

static void showSubMenu(HWND hWnd, HWND hItemButton, bool firstFocused) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    MenuItem *item = (MenuItem *)GetWindowLong(hItemButton, GWL_USERDATA);

    HWND hSubMenuWindow = item->getSubMenuWindow();
    if (hSubMenuWindow != NULL) {
        data->hSelectedMenuItemButton = hItemButton;

        RECT rect;
        GetWindowRect(hItemButton, &rect);
        setMenuAttachedRect(hSubMenuWindow, &rect);

        showMenu(hSubMenuWindow);

        if (firstFocused) {
            HWND hChild = GetWindow(hSubMenuWindow, GW_CHILD);
            if (hChild != NULL)
                SetFocus(hChild);
        }
    }
}

static void hideThisMenu(HWND hWnd) {
    int i;

    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    bool found = false;
    HWND hActiveWindow = GetForegroundWindow();
    int numItems = data->menuItems.size();

    for (i = 0; i < numItems; i++) {
        HWND hSubMenuWindow = data->menuItems[i]->getSubMenuWindow();
        if (hSubMenuWindow != NULL) {
            if (hSubMenuWindow == hActiveWindow) {
                found = true;
                break;
            }
        }
    }

    if (!found)
        hideMenuAscendant(hWnd, false);
}

static void selectFirstOrLastItem(HWND hWnd, bool first) {
    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    HWND hChild = GetWindow(hWnd, GW_CHILD);
    if (hChild == NULL)
        return;

    if (first) {
        HWND hFirst = GetWindow(hChild, GW_HWNDFIRST);
        while (!IsWindowVisible(hFirst) && hFirst != NULL)
            hFirst = GetWindow(hFirst, GW_HWNDNEXT);

        if (hFirst == data->hUpperScrollButton) {
            data->offsetIndex = 0;
            changeWindowLayout(hWnd);

            hChild = GetWindow(hWnd, GW_CHILD);
            hFirst = GetWindow(hChild, GW_HWNDFIRST);
        }

        SetFocus(hFirst);
    }
    else {
        HWND hLast = GetWindow(hChild, GW_HWNDLAST);
        while (!IsWindowVisible(hLast) && hLast != NULL)
            hLast = GetWindow(hLast, GW_HWNDPREV);

        if (hLast == data->hLowerScrollButton) {
            RECT desktopRect;
            HWND hDesktop = GetDesktopWindow();
            GetWindowRect(hDesktop, &desktopRect); 

            int menuItemHeight = data->isMain ? MENU_ITEM_HEIGHT_MAIN :
                MENU_ITEM_HEIGHT_SUB;

            int numItems = data->menuItems.size();
            int maxNumItems = (desktopRect.bottom - MENU_ITEM_MARGIN * 2) /
                menuItemHeight;

            data->offsetIndex = numItems - maxNumItems;
            changeWindowLayout(hWnd);

            hChild = GetWindow(hWnd, GW_CHILD);
            hLast = GetWindow(hChild, GW_HWNDLAST);
        }

        SetFocus(hLast);
    }
}

static int obtainNewMenuItemButtonId(HWND hWnd) {
    const int MIN_ID = IDC_MENU_ITEM_BUTTON_BASE;
    const int MAX_ID = MIN_ID + 1000;
    static int currentId = MIN_ID;

    int i;

    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    int numItems = data->menuItems.size();

    while (true) {
        bool found = false;
        for (i = 0; i < numItems; i++) {
            MenuItem *item = data->menuItems[i];
            if (GetDlgCtrlID(item->getButton()) == currentId) {
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

static void changeWindowLayout(HWND hWnd) {
    int i;

    MenuData *data = (MenuData *)GetWindowLong(hWnd, GWL_USERDATA);

    RECT desktopRect;
    HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktopRect); 

    int menuSideWidth = data->isMain ? MENU_SIDE_WIDTH : 0;
    int menuItemHeight = data->isMain ? MENU_ITEM_HEIGHT_MAIN :
        MENU_ITEM_HEIGHT_SUB;

    int numItems = data->menuItems.size();
    int maxNumItems = (desktopRect.bottom - MENU_ITEM_MARGIN * 2) /
        menuItemHeight;
    int numDispItems = numItems < maxNumItems ? numItems : maxNumItems;

    int windowWidth = MENU_WIDTH + menuSideWidth;

    int windowHeight = 0;
    if (numItems == 0)
        windowHeight = menuItemHeight + MENU_ITEM_MARGIN * 2;
    else
        windowHeight = menuItemHeight * numDispItems + MENU_ITEM_MARGIN * 2;

    if (data->isMain) {
        MoveWindow(hWnd, 0, desktopRect.bottom - windowHeight - TASKBAR_HEIGHT,
            windowWidth, windowHeight, false);
    }
    else {
        int windowLeft = data->attachedRect.right;
        if (windowLeft + windowWidth >= desktopRect.right)
            windowLeft = desktopRect.right - windowWidth;

        int windowTop = data->attachedRect.top;
        if (windowTop + windowHeight >= desktopRect.bottom) {
            if (data->attachedRect.bottom - windowHeight >= 0)
                windowTop = data->attachedRect.bottom - windowHeight;
            else
                windowTop = 0;
        }

        MoveWindow(hWnd, windowLeft, windowTop, windowWidth, windowHeight,
            false);
    }

    if (data->offsetIndex > 0) {
        for (i = 0; i < data->offsetIndex; i++) {
            MenuItem *item = data->menuItems[i];
            ShowWindow(item->getButton(), SW_HIDE);
        }

        data->hUpperScrollButton =
            data->menuItems[data->offsetIndex]->getButton();
    }
    else
        data->hUpperScrollButton = NULL;

    int hideIndexFrom = data->offsetIndex + numDispItems;

    for (i = data->offsetIndex; i < hideIndexFrom; i++) {
        MenuItem *item = data->menuItems[i];
        MoveWindow(item->getButton(),
            2 + menuSideWidth, menuItemHeight * (i - data->offsetIndex) + 2,
            MENU_ITEM_WIDTH, menuItemHeight, false);
        ShowWindow(item->getButton(), SW_SHOWNA);
    }

    if (numItems - hideIndexFrom > 0) {
        for (i = hideIndexFrom; i < numItems; i++) {
            MenuItem *item = data->menuItems[i];
            ShowWindow(item->getButton(), SW_HIDE);
        }

        data->hLowerScrollButton =
            data->menuItems[hideIndexFrom - 1]->getButton();
    }
    else
        data->hLowerScrollButton = NULL;
}