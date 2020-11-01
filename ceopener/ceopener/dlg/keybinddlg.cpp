#include "keybinddlg.h"

#include <algorithm>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <knceutil.h>
#include <kncedlg.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_BINDING_TREE = 101,
    IDC_ENABLED_CHECK = 111,
    IDC_USER_KEY_FN_CHECK = 122,
    IDC_USER_KEY_LABEL = 123,
    IDC_USER_KEY_BROWSE_BUTTON = 124,
    IDC_USE_DEFAULT_BUTTON = 125
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onEnabled(HWND hDlg);
static void onUserKeyBrowse(HWND hDlg);
static void onUseDefault(HWND hDlg);
static void onBindingTree(HWND hDlg, NMHDR *pnmh);
static void updateTreeItem(HWND hDlg, HTREEITEM hTreeItem);
static void updateBinding(HWND hDlg, HTREEITEM hTreeItem);

extern HINSTANCE g_hInstance;

struct KeyBindingDialogData {
    KeyBindingDialogParams *dialogParams;
    int userKeyCode;
};

KeyBindingDialogParams::Binding::Binding() {
    m_isEnabled = false;
    m_defaultFn = false;
    m_defaultKeyCode = 0;
    m_bindedFn = false;
    m_bindedKeyCode = 0;
}

KeyBindingDialogParams::Binding::~Binding() {
}

KeyBindingDialogParams::KeyBindingDialogParams() {
}

KeyBindingDialogParams::~KeyBindingDialogParams() {
}

bool showKeyBindingDialog(HWND hOwnerWindow, KeyBindingDialogParams &params) {
    int ret = DialogBoxParam(g_hInstance, _T("KEY_BINDING"),
        hOwnerWindow, (DLGPROC)dlgProc, (LPARAM)&params);

    return ret == IDOK;
}

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
    case WM_INITDIALOG:
        onInitDialog(hDlg, (void *)lParam);
        return true;
    case WM_COMMAND:
    {
        int id    = LOWORD(wParam); 
        int event = HIWORD(wParam);

        switch (id) {
        case IDOK:
            onOk(hDlg);
            break;
        case IDCANCEL:
            onCancel(hDlg);
            break;
        case IDC_ENABLED_CHECK:
            onEnabled(hDlg);
            break;
        case IDC_USER_KEY_BROWSE_BUTTON:
            onUserKeyBrowse(hDlg);
            break;
        case IDC_USE_DEFAULT_BUTTON:
            onUseDefault(hDlg);
            break;
        default:
            return false;
        }

        return true;
    }
    case WM_NOTIFY:
        switch (wParam) {
        case IDC_BINDING_TREE:
            onBindingTree(hDlg, (NMHDR *)lParam);
            break;
        default:
            return false;
        }
	}

	return false;
}

static void onInitDialog(HWND hDlg, void *params) {
    int i;
	TCHAR dbg[1024];

    KeyBindingDialogData *data = new KeyBindingDialogData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->dialogParams = (KeyBindingDialogParams *)params;

    KnceUtil::adjustDialogLayout(hDlg);

    HWND hBindTree = GetDlgItem(hDlg, IDC_BINDING_TREE);
    TVINSERTSTRUCT treeItem = {0};

    map<tstring, HTREEITEM> catTable;

    vector<KeyBindingDialogParams::Binding *> &binds =
        data->dialogParams->getBindings();
    int numBinds = binds.size();

    for (i = 0; i < numBinds; i++) {
        KeyBindingDialogParams::Binding *bind = binds[i];
        tstring cat = bind->getCategory();

        HTREEITEM hTreeItem = NULL;
        if (catTable.find(cat) == catTable.end()) {
            treeItem.hParent = TVI_ROOT;
            treeItem.hInsertAfter = TVI_LAST;
            treeItem.item.mask = TVIF_TEXT;
            treeItem.item.pszText = (LPTSTR)cat.c_str();

            hTreeItem = TreeView_InsertItem(hBindTree, &treeItem);

            catTable[cat] = hTreeItem;
        }
        else
            hTreeItem = catTable[cat];

		_tcscpy(dbg,(LPTSTR)bind->getName().c_str());

        treeItem.hParent = hTreeItem;
        treeItem.hInsertAfter = TVI_LAST;
        treeItem.item.mask = TVIF_TEXT | TVIF_PARAM;
        treeItem.item.pszText = dbg;
        treeItem.item.lParam = (LPARAM)bind;

        TreeView_InsertItem(hBindTree, &treeItem);
    }

    HTREEITEM hTreeRootItem = TreeView_GetRoot(hBindTree);
    if (hTreeRootItem == NULL)
        updateTreeItem(hDlg, NULL);
    else
        TreeView_SelectItem(hBindTree, hTreeRootItem);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    KeyBindingDialogData *data =
        (KeyBindingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hBindTree = GetDlgItem(hDlg, IDC_BINDING_TREE);

    HTREEITEM hTreeItem = TreeView_GetSelection(hBindTree);
    if (hTreeItem != NULL)
        updateBinding(hDlg, hTreeItem);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    KeyBindingDialogData *data =
        (KeyBindingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onEnabled(HWND hDlg) {
    HWND hEnabledCheck = GetDlgItem(hDlg, IDC_ENABLED_CHECK);
    bool enabled = SendMessage(hEnabledCheck, BM_GETCHECK, 0, 0) != 0;

    HWND hUserKeyFnCheck = GetDlgItem(hDlg, IDC_USER_KEY_FN_CHECK);
    HWND hUserKeyKeyLabel = GetDlgItem(hDlg, IDC_USER_KEY_LABEL);
    HWND hUserKeyBrowseButton = GetDlgItem(hDlg, IDC_USER_KEY_BROWSE_BUTTON);
    HWND hUseDefButton = GetDlgItem(hDlg, IDC_USE_DEFAULT_BUTTON);

    EnableWindow(hUserKeyFnCheck, enabled);
    EnableWindow(hUserKeyKeyLabel, enabled);
    EnableWindow(hUserKeyBrowseButton, enabled);
    EnableWindow(hUseDefButton, enabled);
}

static void onUserKeyBrowse(HWND hDlg) {
    KeyBindingDialogData *data =
        (KeyBindingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    int keyCode = 0;
    if (knceCaptureKey(hDlg, &keyCode)) {
        data->userKeyCode = keyCode;

        tstring keyName;
        KnceUtil::hardKeyCodeToName(keyName, keyCode);

        HWND hUserKeyKeyLabel = GetDlgItem(hDlg, IDC_USER_KEY_LABEL);
        SetWindowText(hUserKeyKeyLabel, keyName.c_str());
    }
}

static void onUseDefault(HWND hDlg) {
    KeyBindingDialogData *data =
        (KeyBindingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hBindTree = GetDlgItem(hDlg, IDC_BINDING_TREE);

    HTREEITEM hTreeItem = TreeView_GetSelection(hBindTree);

    TVITEM treeItem = {0};
    treeItem.mask = TVIF_HANDLE | TVIF_PARAM;
    treeItem.hItem = hTreeItem;
    TreeView_GetItem(hBindTree, &treeItem);

    KeyBindingDialogParams::Binding *bind =
        (KeyBindingDialogParams::Binding *)treeItem.lParam;

    bool fn = bind->getDefaultFn();
    int keyCode = bind->getDefaultKeyCode();

    HWND hUserKeyFnCheck = GetDlgItem(hDlg, IDC_USER_KEY_FN_CHECK);
    HWND hUserKeyKeyLabel = GetDlgItem(hDlg, IDC_USER_KEY_LABEL);

    SendMessage(hUserKeyFnCheck, BM_SETCHECK, fn, 0);

    data->userKeyCode = keyCode;

    tstring keyName;
    KnceUtil::hardKeyCodeToName(keyName, keyCode);
    SetWindowText(hUserKeyKeyLabel, keyName.c_str());
}

static void onBindingTree(HWND hDlg, NMHDR *pnmh) {
    KeyBindingDialogData *data =
        (KeyBindingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    if (pnmh->code != TVN_SELCHANGED)
        return;

    HTREEITEM hOldTreeItem = ((NMTREEVIEW *)pnmh)->itemOld.hItem;
    if (hOldTreeItem != NULL)
        updateBinding(hDlg, hOldTreeItem);

    updateTreeItem(hDlg, ((NMTREEVIEW *)pnmh)->itemNew.hItem);
}

static void updateTreeItem(HWND hDlg, HTREEITEM hTreeItem) {
    KeyBindingDialogData *data =
        (KeyBindingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    KeyBindingDialogParams::Binding *bind = NULL;
    if (hTreeItem != NULL) {
        HWND hBindTree = GetDlgItem(hDlg, IDC_BINDING_TREE);

        TVITEM treeItem = {0};
        treeItem.mask = TVIF_HANDLE | TVIF_PARAM;
        treeItem.hItem = hTreeItem;
        TreeView_GetItem(hBindTree, &treeItem);

        bind = (KeyBindingDialogParams::Binding *)treeItem.lParam;
    }

    HWND hEnabledCheck = GetDlgItem(hDlg, IDC_ENABLED_CHECK);
    HWND hUserKeyFnCheck = GetDlgItem(hDlg, IDC_USER_KEY_FN_CHECK);
    HWND hUserKeyKeyLabel = GetDlgItem(hDlg, IDC_USER_KEY_LABEL);
    HWND hUserKeyBrowseButton = GetDlgItem(hDlg, IDC_USER_KEY_BROWSE_BUTTON);
    HWND hUseDefButton = GetDlgItem(hDlg, IDC_USE_DEFAULT_BUTTON);

    if (bind == NULL) {
        EnableWindow(hEnabledCheck, false);
        EnableWindow(hUserKeyFnCheck, false);
        EnableWindow(hUserKeyKeyLabel, false);
        EnableWindow(hUserKeyBrowseButton, false);
        EnableWindow(hUseDefButton, false);

        return;
    }

    EnableWindow(hEnabledCheck, true);

    bool enabled = bind->isEnabled();
    SendMessage(hEnabledCheck, BM_SETCHECK, enabled, 0);

    EnableWindow(hUserKeyFnCheck, enabled);
    EnableWindow(hUserKeyKeyLabel, enabled);
    EnableWindow(hUserKeyBrowseButton, enabled);
    EnableWindow(hUseDefButton, enabled);

    bool fn = bind->getBindedKeyCode() == 0 ?
        bind->getDefaultFn() : bind->getBindedFn();
    int keyCode = bind->getBindedKeyCode() == 0 ?
        bind->getDefaultKeyCode() : bind->getBindedKeyCode();

    SendMessage(hUserKeyFnCheck, BM_SETCHECK, fn, 0);

    data->userKeyCode = keyCode;

    tstring keyName;
    KnceUtil::hardKeyCodeToName(keyName, keyCode);
    SetWindowText(hUserKeyKeyLabel, keyName.c_str());
}

static void updateBinding(HWND hDlg, HTREEITEM hTreeItem) {
    KeyBindingDialogData *data =
        (KeyBindingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hBindTree = GetDlgItem(hDlg, IDC_BINDING_TREE);

    TVITEM treeItem = {0};
    treeItem.mask = TVIF_HANDLE | TVIF_PARAM;
    treeItem.hItem = hTreeItem;
    TreeView_GetItem(hBindTree, &treeItem);

    KeyBindingDialogParams::Binding *bind =
        (KeyBindingDialogParams::Binding *)treeItem.lParam;
    if (bind == NULL)
        return;

    HWND hEnabledCheck = GetDlgItem(hDlg, IDC_ENABLED_CHECK);
    HWND hUserKeyFnCheck = GetDlgItem(hDlg, IDC_USER_KEY_FN_CHECK);
    HWND hUserKeyKeyLabel = GetDlgItem(hDlg, IDC_USER_KEY_LABEL);
    HWND hUserKeyBrowseButton = GetDlgItem(hDlg, IDC_USER_KEY_BROWSE_BUTTON);
    HWND hUseDefButton = GetDlgItem(hDlg, IDC_USE_DEFAULT_BUTTON);

    bind->setEnabled(SendMessage(hEnabledCheck, BM_GETCHECK, 0, 0) != 0);

    bool fn = SendMessage(hUserKeyFnCheck, BM_GETCHECK, 0, 0) != 0;
    int keyCode = data->userKeyCode;

    if (fn != bind->getDefaultFn() || keyCode != bind->getDefaultKeyCode()) {
        bind->setBindedFn(fn);
        bind->setBindedKeyCode(keyCode);
    }
    else {
        bind->setBindedFn(false);
        bind->setBindedKeyCode(0);
    }
}
