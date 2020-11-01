#include "kncecomm.h"

#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_CHOOSE_FILE_CURRENT_DIRECTORY_LABEL = 102,
    IDC_CHOOSE_FILE_DIRECTORY_LIST = 103,
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *createStruct);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onDirectoryList(HWND hDlg, int event);
static void updateDirectoryList(HWND hDlg);

extern HINSTANCE g_hInstance;

struct KnceDirDlgData {
    HFONT hFont;
    KnceChooseDirectoryParams *chooseDirectoryParams;
    tstring currentPath;
};

bool showChooseDirectoryDialog(HWND hOwnerWindow,
    KnceChooseDirectoryParams *params) {

    int ret = DialogBoxParam(g_hInstance, _T("CHOOSE_DIRECTORY"), hOwnerWindow,
        (DLGPROC)dlgProc, (LPARAM)params);

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
        case IDC_CHOOSE_FILE_DIRECTORY_LIST:
            onDirectoryList(hDlg, event);
            break;
        default:
            return false;
        }

        return true;
    }
	}

	return false;
}

static void onInitDialog(HWND hDlg, void *params) {
    KnceDirDlgData *data = new KnceDirDlgData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->hFont = knceCreateDefaultFont(true, 100, false, false);
    knceSetDialogFont(hDlg, data->hFont);

    data->chooseDirectoryParams = (KnceChooseDirectoryParams *)params;

    data->currentPath = data->chooseDirectoryParams->initialPath;
    if (data->currentPath.empty())
        data->currentPath = _T("\\");

    updateDirectoryList(hDlg);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    KnceDirDlgData *data = (KnceDirDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hDirList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_DIRECTORY_LIST);
    if (GetFocus() == hDirList) {
        onDirectoryList(hDlg, LBN_DBLCLK);
        return;
    }

    _tcsncpy(data->chooseDirectoryParams->directory, data->currentPath.c_str(),
        MAX_PATH);

    DeleteObject(data->hFont);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    KnceDirDlgData *data = (KnceDirDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    DeleteObject(data->hFont);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onDirectoryList(HWND hDlg, int event) {
    const int dirDispLen = MAX_PATH + 2;

    KnceDirDlgData *data = (KnceDirDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    if (event != LBN_DBLCLK)
        return;

    HWND hDirList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_DIRECTORY_LIST);

    int selected = SendMessage(hDirList, LB_GETCURSEL, 0, 0);
    if (selected == -1)
        return;

    tstring dirDisp;
    TCHAR dirDispCStr[dirDispLen];

    if (SendMessage(hDirList, LB_GETTEXTLEN, selected, 0) < dirDispLen) {
        SendMessage(hDirList, LB_GETTEXT, selected, (LPARAM)dirDispCStr);
        dirDisp = dirDispCStr;
    }

    if (dirDisp == _T("(Up...)")) {
        int pos = data->currentPath.rfind(_T('\\'));
        if (pos == 0)
            data->currentPath = _T("\\");
        else
            data->currentPath = data->currentPath.substr(0, pos);
    }
    else {
        tstring dirName = dirDisp.substr(1, dirDisp.length() - 2);
        if (data->currentPath == _T("\\"))
            data->currentPath += dirName;
        else
            data->currentPath += _T("\\") + dirName;
    }

    updateDirectoryList(hDlg);
}

static void updateDirectoryList(HWND hDlg) {
    KnceDirDlgData *data = (KnceDirDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    bool isRoot = data->currentPath == _T("\\");

    tstring dirName;
    if (isRoot)
        dirName = _T("[My Device]");
    else {
        dirName = _T("[") + data->currentPath.substr(
            data->currentPath.rfind(_T('\\')) + 1) + _T("]");
    }

    HWND hDirLabel = GetDlgItem(hDlg, IDC_CHOOSE_FILE_CURRENT_DIRECTORY_LABEL);
    SetWindowText(hDirLabel, dirName.c_str());

    HWND hDirList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_DIRECTORY_LIST);

    SendMessage(hDirList, LB_RESETCONTENT, 0, 0);

    if (!isRoot)
        SendMessage(hDirList, LB_ADDSTRING, 0, (LPARAM)_T("(Up...)"));

    tstring findPath;
    if (isRoot)
        findPath = _T("\\*.*");
    else
        findPath = data->currentPath + _T("\\*.*");

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(findPath.c_str(), &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            tstring dirDisp = _T("[") + tstring(fd.cFileName) + _T("]");
            SendMessage(hDirList, LB_ADDSTRING, 0, (LPARAM)dirDisp.c_str());
        }

        while (FindNextFile(hFind, &fd)) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                tstring dirDisp = _T("[") + tstring(fd.cFileName) + _T("]");
                SendMessage(hDirList, LB_ADDSTRING, 0,
                    (LPARAM)dirDisp.c_str());
            }
        }
    }

    FindClose(hFind);

    int numDirs = SendMessage(hDirList, LB_GETCOUNT, 0, 0);
    if (isRoot)
        SendMessage(hDirList, LB_SETCURSEL, 0, 0);
    else {
        if (numDirs > 1)
            SendMessage(hDirList, LB_SETCURSEL, 1, 0);
        else
            SendMessage(hDirList, LB_SETCURSEL, 0, 0);
    }
}
