#include <map>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <knceutil.h>
#include <ceopener_plugin.h>
#include "edititemdlg.h"
#include "specialicondlg.h"


#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

#ifdef DESKTOPITEMMGR_EXPORTS
#define DESKTOPITEMMGR_API __declspec(dllexport)
#else
#define DESKTOPITEMMGR_API __declspec(dllimport)
#endif

enum {
    MENU_INDEX_ARRANGE_ICONS = 0,
    MENU_INDEX_ADD_SHORTCUT = 1,
    MENU_INDEX_ADD_SPECIAL_ICON = 2,
    MENU_INDEX_OPEN_ITEM = 3,
    MENU_INDEX_EDIT_ITEM = 4,
    MENU_INDEX_DELETE_ITEM = 5,
    ICON_SIZE = 32,
    ICON_CAPTION_GAP = 4,
    DESKTOP_ITEM_WIDTH = 80,
    DESKTOP_ITEM_HEIGHT = 65,
    DESKTOP_ITEM_GAP = 5,
    DRAG_NONE = 0,
    DRAG_READY = 1,
    DRAG_MOVE = 2,
    DRAG_START_RANGE = 5
};

class DesktopItem {
public:
    DesktopItem();
    virtual ~DesktopItem();
    tstring getIconName() const { return m_iconName; }
    void setIconName(const tstring &name) { m_iconName = name; }
    tstring getExternalIconFileName() const { return m_externalIconFileName; }
    void setExternalIconFileName(const tstring &fileName)
        { m_externalIconFileName = fileName; }
    int getExternalIconIndex() const { return m_externalIconIndex; }
    void setExternalIconIndex(int index) { m_externalIconIndex = index; }
    tstring getCaption() const { return m_caption; }
    void setCaption(const tstring &capt) { m_caption = capt; }
    tstring getApplicationPath() const { return m_applicationPath; }
    void setApplicationPath(const tstring &path) { m_applicationPath = path; }
    HICON getIcon() const { return m_hIcon; }
    void setIcon(HICON hIcon) { m_hIcon = hIcon; }
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }

private:
    tstring m_iconName;
    tstring m_externalIconFileName;
    int m_externalIconIndex;
    tstring m_caption;
    tstring m_applicationPath;
    HICON m_hIcon;
    int m_id;
};

static void onMenuItemSelected(HWND hCont, HWND hMenu, int id);
static void onDesktopItemFocus(HWND hCont, int id);
static void onDesktopItemBlur(HWND hCont, int id);
static void onDesktopItemTapDown(HWND hCont, int id, int x, int y,
    RECT *place);
static void onDesktopItemTapUp(HWND hCont, int id, int x, int y,
    RECT *place);
static void onDesktopItemTapMove(HWND hCont, int id, int x, int y,
    RECT *place);
static void onDesktopItemTapDouble(HWND hCont, int id, int x, int y,
    RECT *place);
static void onDesktopItemTapHold(HWND hCont, int id, int x, int y,
    RECT *place);
static void onDesktopItemKeyDown(HWND hCont, int id, int keyCode);
static void onDesktopItemPaint(HWND hCont, int id, HDC hDC, RECT *place,
    int focused);
static void onSettingChanged(HWND hCont, unsigned long params);
static void createDesktopItem(HWND hCont, const tstring &iconName,
    const tstring &extIconFileName, int extIconIndex, const tstring &caption,
    const tstring &appPath, bool autoLocate, int left = 0, int top = 0);
static void calcNextItemPosition(HWND hCont, int &left, int &top);
static void arrangeIcons(HWND hCont);
static void calcItemSize(HWND hCont, int &width, int &height,
    const tstring &caption);
static void launchApplication(HWND hCont, DesktopItem *item);
static void removeItem(HWND hCont, DesktopItem *item);
static void update(HWND hCont);

HINSTANCE g_hInstance = NULL;
static HFONT g_hFont = NULL;
static COLORREF g_selectedItemColor = 0;
static COLORREF g_selectedTextColor = 0;
static HWND g_hDesktopItemMenu = NULL;
static map<int, int> g_menuIdTable;
static map<int, DesktopItem *> g_desktopItemTable;
static int g_dragState = DRAG_NONE;
static int g_dragOrigin[2];
static int g_dragOffset[2];
static DesktopItem *g_selectedItem = NULL;

DesktopItem::DesktopItem() {
    m_externalIconIndex = 0;
    m_hIcon = NULL;
    m_id = 0;
}

DesktopItem::~DesktopItem() {
}

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hInstance = (HINSTANCE)hInst;

    return true;
}

extern "C" DESKTOPITEMMGR_API int pluginInit(HWND hCont) {
    int i;

    PluginFontInfo fontInfo;
    pluginGetFontInfo(hCont, &fontInfo);
    g_hFont = KnceUtil::createFont(fontInfo.faceName, fontInfo.pointSize,
        fontInfo.isBold != 0, fontInfo.isItalic != 0);

    g_selectedItemColor = pluginGetColor(hCont, _T("selectedItem"));
    g_selectedTextColor = pluginGetColor(hCont, _T("selectedText"));

    HWND hDesktopMenu = pluginGetDesktopMenu(hCont);

    PluginMenuItem menuItem = {0};
    _tcscpy(menuItem.caption, _T("アイコンの整列"));
    menuItem.onItemSelected = onMenuItemSelected;

    g_menuIdTable[MENU_INDEX_ARRANGE_ICONS] =
        pluginAppendMenuItem(hCont, hDesktopMenu, &menuItem);

    _tcscpy(menuItem.caption, _T("ショートカットの作成"));
    g_menuIdTable[MENU_INDEX_ADD_SHORTCUT] =
        pluginAppendMenuItem(hCont, hDesktopMenu, &menuItem);

    _tcscpy(menuItem.caption, _T("デスクトップのカスタマイズ"));
    g_menuIdTable[MENU_INDEX_ADD_SPECIAL_ICON] =
        pluginAppendMenuItem(hCont, hDesktopMenu, &menuItem);

    PluginMenu desktopItemMenu = {0};
    g_hDesktopItemMenu = pluginCreateSubMenu(hCont, &desktopItemMenu);

    _tcscpy(menuItem.caption, _T("開く"));
    g_menuIdTable[MENU_INDEX_OPEN_ITEM] =
        pluginAppendMenuItem(hCont, g_hDesktopItemMenu, &menuItem);

    _tcscpy(menuItem.caption, _T("編集"));
    g_menuIdTable[MENU_INDEX_EDIT_ITEM] =
        pluginAppendMenuItem(hCont, g_hDesktopItemMenu, &menuItem);

    _tcscpy(menuItem.caption, _T("削除"));
    g_menuIdTable[MENU_INDEX_DELETE_ITEM] =
        pluginAppendMenuItem(hCont, g_hDesktopItemMenu, &menuItem);

    pluginRegisterCallback(hCont, PLUGIN_CALLBACK_SETTING_CHANGED,
        onSettingChanged);

    tstring persistDir = _T("\\Nand2\\.ceopener");
    tstring desktopFileName = persistDir + _T("\\desktopitems.dat");

    if (GetFileAttributes(desktopFileName.c_str()) == -1) {
        createDesktopItem(hCont, _T("LAPTOP"), _T(""), 0, _T("マイ デバイス"),
            _T("\\Windows\\explorer.exe"), true);

        createDesktopItem(hCont, _T("TOOLS"), _T(""), 0, _T("コントロール パネル"),
            _T("\\Windows\\control.exe"), true);
    }
    else {
        map<tstring, tstring> props;
        KnceUtil::readPropertyFile(props, desktopFileName);

        TCHAR keyCStr[256];
        int numItems = _ttoi(props[_T("numItems")].c_str());

        for (i = 0; i < numItems; i++) {
            _sntprintf(keyCStr, 256, _T("item.%d.iconName"), i);
            tstring iconName = props[keyCStr];

            _sntprintf(keyCStr, 256, _T("item.%d.externalIconFileName"), i);

            tstring extIconFileName;
            if (props.find(keyCStr) != props.end())
                extIconFileName = props[keyCStr];

            _sntprintf(keyCStr, 256, _T("item.%d.externalIconIndex"), i);

            int extIconIndex = 0;
            if (props.find(keyCStr) != props.end())
                extIconIndex = _ttoi(props[keyCStr].c_str());

            _sntprintf(keyCStr, 256, _T("item.%d.caption"), i);
            tstring capt = props[keyCStr];

            _sntprintf(keyCStr, 256, _T("item.%d.applicationPath"), i);
            tstring appPath = props[keyCStr];

            _sntprintf(keyCStr, 256, _T("item.%d.left"), i);
            int left = _ttoi(props[keyCStr].c_str());

            _sntprintf(keyCStr, 256, _T("item.%d.top"), i);
            int top = _ttoi(props[keyCStr].c_str());

            createDesktopItem(hCont, iconName, extIconFileName, extIconIndex,
                capt, appPath, false, left, top);
        }
    }

    return true;
}

extern "C" DESKTOPITEMMGR_API void pluginTerminate(HWND hCont) {
    int i;

    map<tstring, tstring> props;
    TCHAR keyCStr[256];
    TCHAR valCStr[256];
    RECT place = {0};

    _sntprintf(valCStr, 256, _T("%d"), g_desktopItemTable.size());
    props[_T("numItems")] = valCStr;

    map<int, DesktopItem *>::const_iterator iter = g_desktopItemTable.begin();
    for (i = 0; iter != g_desktopItemTable.end(); iter++, i++) {
        DesktopItem *item = iter->second;

        _sntprintf(keyCStr, 256, _T("item.%d.iconName"), i);
        props[keyCStr] = item->getIconName();

        _sntprintf(keyCStr, 256, _T("item.%d.externalIconFileName"), i);
        props[keyCStr] = item->getExternalIconFileName();

        _sntprintf(keyCStr, 256, _T("item.%d.externalIconIndex"), i);
        _sntprintf(valCStr, 256, _T("%d"), item->getExternalIconIndex());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("item.%d.caption"), i);
        props[keyCStr] = item->getCaption();

        _sntprintf(keyCStr, 256, _T("item.%d.applicationPath"), i);
        props[keyCStr] = item->getApplicationPath();

        pluginGetDesktopItemPlacement(hCont, iter->first, &place);

        _sntprintf(keyCStr, 256, _T("item.%d.left"), i);
        _sntprintf(valCStr, 256, _T("%d"), place.left);
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("item.%d.top"), i);
        _sntprintf(valCStr, 256, _T("%d"), place.top);
        props[keyCStr] = valCStr;
    }

    tstring persistDir = _T("\\Nand2\\.ceopener");
    if (GetFileAttributes(persistDir.c_str()) == -1)
        CreateDirectory(persistDir.c_str(), NULL);

    tstring desktopFileName = persistDir + _T("\\desktopitems.dat");

    KnceUtil::writePropertyFile(desktopFileName, props);

    iter = g_desktopItemTable.begin();
    for ( ; iter != g_desktopItemTable.end(); iter++)
        delete iter->second;

    DeleteObject(g_hFont);
}

static void onMenuItemSelected(HWND hCont, HWND hMenu, int id) {
    if (hMenu == g_hDesktopItemMenu) {
        if (id == g_menuIdTable[MENU_INDEX_OPEN_ITEM])
            launchApplication(hCont, g_selectedItem);
        else if (id == g_menuIdTable[MENU_INDEX_EDIT_ITEM]) {
            EditItemDialogParams params;
            params.setCaption(g_selectedItem->getCaption());
            params.setApplicationPath(g_selectedItem->getApplicationPath());

            if (!showEditItemDialog(hCont, params))
                return;

            tstring appPath = params.getApplicationPath();

            g_selectedItem->setIconName(_T("APPLICATION"));
            g_selectedItem->setExternalIconFileName(appPath);
            g_selectedItem->setExternalIconIndex(0);
            g_selectedItem->setCaption(params.getCaption());
            g_selectedItem->setApplicationPath(appPath);

            SHFILEINFO fileInfo = {0};
            fileInfo.iIcon = 0;
            SHGetFileInfo(appPath.c_str(), 0, &fileInfo,
                sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_LARGEICON);
            g_selectedItem->setIcon(fileInfo.hIcon);

            pluginRedrawDesktopItem(hCont, g_selectedItem->getId());
        }
        else if (id == g_menuIdTable[MENU_INDEX_DELETE_ITEM])
            removeItem(hCont, g_selectedItem);
    }
    else {
        if (id == g_menuIdTable[MENU_INDEX_ARRANGE_ICONS])
            arrangeIcons(hCont);
        else if (id == g_menuIdTable[MENU_INDEX_ADD_SHORTCUT]) {
            EditItemDialogParams params;
            params.setBrowsingOnStartup(true);

            if (!showEditItemDialog(hCont, params))
                return;

            createDesktopItem(hCont, _T("APPLICATION"),
                params.getApplicationPath(), 0, params.getCaption(),
                params.getApplicationPath(), true);
        }
        else if (id == g_menuIdTable[MENU_INDEX_ADD_SPECIAL_ICON]) {
            SpecialIconDialogParams params;
            if (!showSpecialIconDialog(hCont, params))
                return;

            switch (params.getIconType()) {
            case SpecialIconDialogParams::ICON_MY_DEVICE:
                createDesktopItem(hCont, _T("LAPTOP"), _T(""), 0,
                    _T("マイ デバイス"), _T("\\Windows\\explorer.exe"), true);
                break;
            case SpecialIconDialogParams::ICON_CONTROL_PANEL:
                createDesktopItem(hCont, _T("TOOLS"), _T(""), 0,
                    _T("コントロール パネル"), _T("\\Windows\\control.exe"), true);
                break;
            case SpecialIconDialogParams::ICON_NAND3:
                createDesktopItem(hCont, _T("DRIVE"), _T(""), 0,
                    _T("Nand3"), _T("\\Nand3"), true);
                break;
            case SpecialIconDialogParams::ICON_STORAGE_CARD:
                createDesktopItem(hCont, _T("SD_CARD"), _T(""), 0,
                    _T("SD カード"), _T("\\Storage Card"), true);
                break;
            }
        }
    }
    update(hCont);
}

static void onDesktopItemFocus(HWND hCont, int id) {
    g_selectedItem = g_desktopItemTable[id];
}

static void onDesktopItemBlur(HWND hCont, int id) {
}

static void onDesktopItemTapDown(HWND hCont, int id, int x, int y,
    RECT *place) {

    g_dragState = DRAG_READY;

    g_dragOrigin[0] = x;
    g_dragOrigin[1] = y;

    g_dragOffset[0] = x - place->left;
    g_dragOffset[1] = y - place->top;

    pluginSetDesktopMouseCapture(hCont, id);
}

static void onDesktopItemMouseButtonUp(HWND hCont, int id, int x, int y,
    RECT *place) {

    if (g_dragState != DRAG_NONE) {
        g_dragState = DRAG_NONE;

        pluginReleaseDesktopMouseCapture(hCont);
    }

	update(hCont);
}

static void onDesktopItemTapMove(HWND hCont, int id, int x, int y,
    RECT *place) {

    if (g_dragState == DRAG_READY) {
        int dx = x - g_dragOrigin[0];
        int dy = y - g_dragOrigin[1];

        if (dx * dx + dy * dy > DRAG_START_RANGE * DRAG_START_RANGE)
            g_dragState = DRAG_MOVE;
    }
    else if (g_dragState == DRAG_MOVE) {
        pluginSetDesktopItemPosition(hCont, id, PLUGIN_DESKTOP_ITEM_NO_RESIZE,
            x - g_dragOffset[0], y - g_dragOffset[1], 0, 0);
    }
}

static void onDesktopItemTapDouble(HWND hCont, int id, int x, int y,
    RECT *place) {

    DesktopItem *item = g_desktopItemTable[id];

    launchApplication(hCont, item);
}

static void onDesktopItemTapHold(HWND hCont, int id, int x, int y,
    RECT *place) {

    if (g_dragState != DRAG_NONE) {
        g_dragState = DRAG_NONE;

        pluginReleaseDesktopMouseCapture(hCont);
    }

    POINT pt = {x, y};
    pluginDesktopToScreen(hCont, &pt);

    pluginShowPopupMenu(hCont, g_hDesktopItemMenu, pt.x, pt.y);
}

static void onDesktopItemKeyDown(HWND hCont, int id, int keyCode) {
    DesktopItem *item = g_desktopItemTable[id];

    if (keyCode == VK_RETURN)
        launchApplication(hCont, item);
    else if (keyCode == VK_BACK)
        removeItem(hCont, item);
}

static void onDesktopItemPaint(HWND hCont, int id, HDC hDC, RECT *place,
    int focused) {

    DesktopItem *item = g_desktopItemTable[id];

    int itemWidth = place->right - place->left;
    DrawIcon(hDC, (itemWidth - ICON_SIZE) / 2 + place->left, 0 + place->top,
        item->getIcon());

    HFONT hPrevFont = (HFONT)SelectObject(hDC, g_hFont);

    COLORREF prevBkColor = 0;
    int prevBkMode = 0;

    if (focused)
        prevBkColor = SetBkColor(hDC, g_selectedItemColor);
    else
        prevBkMode = SetBkMode(hDC, TRANSPARENT);

    RECT captionRect;
    captionRect.left = place->left;
    captionRect.top = ICON_SIZE + ICON_CAPTION_GAP + place->top;
    captionRect.right = place->right;
    captionRect.bottom = place->bottom;

    COLORREF prevTextColor = GetTextColor(hDC);

    if (!focused) {
        OffsetRect(&captionRect, 0, 1);

        SetTextColor(hDC, RGB(128, 128, 128));

        DrawText(hDC, item->getCaption().c_str(), -1, &captionRect,
            DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_NOCLIP);

        OffsetRect(&captionRect, 0, -1);

        OffsetRect(&captionRect, 1, 1);

        SetTextColor(hDC, RGB(0, 0, 0));

        DrawText(hDC, item->getCaption().c_str(), -1, &captionRect,
            DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_NOCLIP);

        OffsetRect(&captionRect, -1, -1);
    }

    SetTextColor(hDC, focused ? g_selectedTextColor : RGB(255, 255, 255));

    DrawText(hDC, item->getCaption().c_str(), -1, &captionRect,
        DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_NOCLIP);

    SetTextColor(hDC, prevTextColor);

    if (focused)
        SetBkColor(hDC, prevBkColor);
    else
        SetBkMode(hDC, prevBkMode);

    SelectObject(hDC, hPrevFont);
}

static void onSettingChanged(HWND hCont, unsigned long params) {
    g_selectedItemColor = pluginGetColor(hCont, _T("selectedItem"));
    g_selectedTextColor = pluginGetColor(hCont, _T("selectedText"));
}

static void createDesktopItem(HWND hCont, const tstring &iconName,
    const tstring &extIconFileName, int extIconIndex, const tstring &caption,
    const tstring &appPath, bool autoLocate, int left, int top) {

    PluginDesktopItem itemObj = {0};
    itemObj.isFocusable = true;

    if (autoLocate)
        calcNextItemPosition(hCont, left, top);

    itemObj.left = left;
    itemObj.top = top;
    calcItemSize(hCont, itemObj.width, itemObj.height, caption);

    itemObj.onFocus = onDesktopItemFocus;
    itemObj.onBlur = onDesktopItemBlur;
    itemObj.onTapDown = onDesktopItemTapDown;
    itemObj.onTapUp = onDesktopItemMouseButtonUp;
    itemObj.onTapMove = onDesktopItemTapMove;
    itemObj.onTapDouble = onDesktopItemTapDouble;
    itemObj.onTapHold = onDesktopItemTapHold;
    itemObj.onKeyDown = onDesktopItemKeyDown;
    itemObj.onPaint = onDesktopItemPaint;

    int id = pluginRegisterDesktopItem(hCont, &itemObj);

    DesktopItem *item = new DesktopItem();

    item->setIconName(iconName);
    item->setExternalIconFileName(extIconFileName);
    item->setExternalIconIndex(extIconIndex);
    item->setCaption(caption);
    item->setApplicationPath(appPath);

    if (extIconFileName.empty()) {
        item->setIcon((HICON)LoadImage(g_hInstance, iconName.c_str(),
            IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
    }
    else {
        SHFILEINFO fileInfo = {0};
        fileInfo.iIcon = extIconIndex;
        SHGetFileInfo(extIconFileName.c_str(), 0, &fileInfo,
            sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_LARGEICON);
        item->setIcon(fileInfo.hIcon);
    }

    item->setId(id);

    g_desktopItemTable[id] = item;
}

static void calcNextItemPosition(HWND hCont, int &left, int &top) {
    int i, j;

    int numItems = g_desktopItemTable.size();
    int *rectBuf = new int[numItems * 4];

    RECT place = {0};
    map<int, DesktopItem *>::const_iterator iter = g_desktopItemTable.begin();

    for (i = 0; iter != g_desktopItemTable.end(); iter++, i++) {
        pluginGetDesktopItemPlacement(hCont, iter->first, &place);
        rectBuf[i * 4] = place.left;
        rectBuf[i * 4 + 1] = place.top;
        rectBuf[i * 4 + 2] = place.right;
        rectBuf[i * 4 + 3] = place.bottom;
    }

    RECT workAreaRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);

    left = DESKTOP_ITEM_GAP;
    top = DESKTOP_ITEM_GAP;

    for (i = 0; i < 100; i++) {
        int centerX = (left + left + DESKTOP_ITEM_WIDTH) / 2;
        int centerY = (top + top + DESKTOP_ITEM_HEIGHT) / 2;

        bool hit = false;
        for (j = 0; j < numItems; j++) {
            if (centerX >= rectBuf[j * 4] &&
                centerX <= rectBuf[j * 4 + 2] &&
                centerY >= rectBuf[j * 4 + 1] &&
                centerY <= rectBuf[j * 4 + 3]) {

                hit = true;
                break;
            }
        }

        if (!hit)
            break;

        top += DESKTOP_ITEM_HEIGHT + DESKTOP_ITEM_GAP;
        if (top + DESKTOP_ITEM_HEIGHT > workAreaRect.bottom) {
            left += DESKTOP_ITEM_WIDTH + DESKTOP_ITEM_GAP;
            top = DESKTOP_ITEM_GAP;
        }
    }

    delete [] rectBuf;
}

static void arrangeIcons(HWND hCont) {
    const int HALF_GAP = DESKTOP_ITEM_GAP / 2;
    const int WIDTH_PLUS_GAP = DESKTOP_ITEM_WIDTH + DESKTOP_ITEM_GAP;
    const int HEIGHT_PLUS_GAP = DESKTOP_ITEM_HEIGHT + DESKTOP_ITEM_GAP;

    RECT place = {0};
    map<int, DesktopItem *>::const_iterator iter = g_desktopItemTable.begin();

    for ( ; iter != g_desktopItemTable.end(); iter++) {
        int id = iter->first;
        pluginGetDesktopItemPlacement(hCont, id, &place);

        int left = (int)(((place.left - HALF_GAP) /
            (double)WIDTH_PLUS_GAP) + 0.5) * WIDTH_PLUS_GAP +
            DESKTOP_ITEM_GAP;
        int top = (int)(((place.top - HALF_GAP) /
            (double)HEIGHT_PLUS_GAP) + 0.5) * HEIGHT_PLUS_GAP +
            DESKTOP_ITEM_GAP;

        pluginSetDesktopItemPosition(hCont, id, PLUGIN_DESKTOP_ITEM_NO_RESIZE,
            left, top, 0, 0);
    }
}

static void calcItemSize(HWND hCont, int &width, int &height,
    const tstring &caption) {

    HDC hContDC = GetDC(hCont);

    HFONT hPrevFont = (HFONT)SelectObject(hContDC, g_hFont);

    RECT captionRect = {0, 0, DESKTOP_ITEM_WIDTH, 1};

    int captionHeight = DrawText(hContDC, caption.c_str(), -1, &captionRect,
        DT_CALCRECT | DT_CENTER | DT_NOPREFIX | DT_WORDBREAK | DT_NOCLIP);

    SelectObject(hContDC, hPrevFont);

    ReleaseDC(hCont, hContDC);

    width = DESKTOP_ITEM_WIDTH;
    height = ICON_SIZE + ICON_CAPTION_GAP + captionHeight + 1;
}

static void launchApplication(HWND hCont, DesktopItem *item) {
    tstring fileName = item->getApplicationPath();
    tstring argStr;

    if ((GetFileAttributes(fileName.c_str()) &
        FILE_ATTRIBUTE_DIRECTORY) != 0) {

        argStr = fileName;
        fileName = _T("\\Windows\\explorer.exe");
    }

    SHELLEXECUTEINFO execInfo = {0};
    execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    execInfo.fMask = 0;
    execInfo.hwnd = hCont;
    execInfo.lpVerb = _T("open");
    execInfo.lpFile = fileName.c_str();
    execInfo.lpParameters = argStr.c_str();
    execInfo.nShow = SW_SHOWNORMAL;
    execInfo.hInstApp = g_hInstance;

    ShellExecuteEx(&execInfo);
}

static void removeItem(HWND hCont, DesktopItem *item) {
    tstring msg = _T("'") + item->getCaption() + _T("' を削除しますか?");

    int ret = MessageBox(hCont, msg.c_str(), _T("確認"),
        MB_YESNO | MB_ICONQUESTION);

    if (ret == IDYES) {
        int id = item->getId();
        pluginUnregisterDesktopItem(hCont, id);

        delete g_desktopItemTable[id];
        g_desktopItemTable.erase(id);
    }
}


void update(HWND hCont){
    int i;

    map<tstring, tstring> props;
    TCHAR keyCStr[256];
    TCHAR valCStr[256];
    RECT place = {0};

    _sntprintf(valCStr, 256, _T("%d"), g_desktopItemTable.size());
    props[_T("numItems")] = valCStr;

    map<int, DesktopItem *>::const_iterator iter = g_desktopItemTable.begin();
    for (i = 0; iter != g_desktopItemTable.end(); iter++, i++) {
        DesktopItem *item = iter->second;

        _sntprintf(keyCStr, 256, _T("item.%d.iconName"), i);
        props[keyCStr] = item->getIconName();

        _sntprintf(keyCStr, 256, _T("item.%d.externalIconFileName"), i);
        props[keyCStr] = item->getExternalIconFileName();

        _sntprintf(keyCStr, 256, _T("item.%d.externalIconIndex"), i);
        _sntprintf(valCStr, 256, _T("%d"), item->getExternalIconIndex());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("item.%d.caption"), i);
        props[keyCStr] = item->getCaption();

        _sntprintf(keyCStr, 256, _T("item.%d.applicationPath"), i);
        props[keyCStr] = item->getApplicationPath();

        pluginGetDesktopItemPlacement(hCont, iter->first, &place);

        _sntprintf(keyCStr, 256, _T("item.%d.left"), i);
        _sntprintf(valCStr, 256, _T("%d"), place.left);
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("item.%d.top"), i);
        _sntprintf(valCStr, 256, _T("%d"), place.top);
        props[keyCStr] = valCStr;
    }

    tstring persistDir = _T("\\Nand2\\.ceopener");
    if (GetFileAttributes(persistDir.c_str()) == -1)
        CreateDirectory(persistDir.c_str(), NULL);

    tstring desktopFileName = persistDir + _T("\\desktopitems.dat");

    KnceUtil::writePropertyFile(desktopFileName, props);
}