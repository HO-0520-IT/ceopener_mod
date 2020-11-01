#include "kncecomm.h"

#include <string>
#include <algorithm>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_APPLICATION_LIST = 101,
    IDC_PATH_EDIT = 102,
    IDC_PATH_BROWSE = 103
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *createStruct);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onApplicationList(HWND hDlg, int event);
static void onPathEdit(HWND hDlg, int event);
static void onPathBrowse(HWND hDlg, int event);
static void updateApplicationList(HWND hDlg);
static void addToBucket(vector<pair<tstring, tstring> > &captToPath,
    const tstring &dir);

extern HINSTANCE g_hInstance;

struct KnceAppDlgData {
    HFONT hFont;
    KnceChooseApplicationParams *chooseApplicationParams;
    vector<tstring> applicationPaths;
    bool eventProcessing;
};

bool showChooseApplicationDialog(HWND hOwnerWindow,
    KnceChooseApplicationParams *params) {

    int ret = DialogBoxParam(g_hInstance, _T("CHOOSE_APPLICATION"),
        hOwnerWindow, (DLGPROC)dlgProc, (LPARAM)params);

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
        case IDC_APPLICATION_LIST:
            onApplicationList(hDlg, event);
            break;
        case IDC_PATH_EDIT:
            onPathEdit(hDlg, event);
            break;
        case IDC_PATH_BROWSE:
            onPathBrowse(hDlg, event);
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
    KnceAppDlgData *data = new KnceAppDlgData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->hFont = knceCreateDefaultFont(true, 100, false, false);
    knceSetDialogFont(hDlg, data->hFont);

    data->chooseApplicationParams = (KnceChooseApplicationParams *)params;
    data->eventProcessing = false;

    updateApplicationList(hDlg);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    KnceAppDlgData *data = (KnceAppDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hPathEdit = GetDlgItem(hDlg, IDC_PATH_EDIT);
    GetWindowText(hPathEdit, data->chooseApplicationParams->fileName,
        MAX_PATH);

    HWND hAppList = GetDlgItem(hDlg, IDC_APPLICATION_LIST);
    int index = SendMessage(hAppList, LB_GETCURSEL, 0, 0);

    if (index == -1) {
        tstring title = data->chooseApplicationParams->fileName;

        int sepPos = title.rfind(_T('\\'));
        if (sepPos != tstring::npos)
            title = title.substr(sepPos + 1);

        int extPos = title.rfind(_T('.'));
        if (extPos != tstring::npos)
            title = title.substr(0, extPos);

        _tcsncpy(data->chooseApplicationParams->title, title.c_str(), 256);
    }
    else {
        TCHAR *appNameBuf = new TCHAR[
            SendMessage(hAppList, LB_GETTEXTLEN, index, 0) + 1];
        SendMessage(hAppList, LB_GETTEXT, index, (LPARAM)appNameBuf);

        _tcsncpy(data->chooseApplicationParams->title, appNameBuf, 256);

        delete [] appNameBuf;
    }

    DeleteObject(data->hFont);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    KnceAppDlgData *data = (KnceAppDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    DeleteObject(data->hFont);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onApplicationList(HWND hDlg, int event) {
    KnceAppDlgData *data = (KnceAppDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    if (event != LBN_SELCHANGE || data->eventProcessing)
        return;

    HWND hAppList = GetDlgItem(hDlg, IDC_APPLICATION_LIST);
    int index = SendMessage(hAppList, LB_GETCURSEL, 0, 0);

    if (index == -1)
        return;

    HWND hPathEdit = GetDlgItem(hDlg, IDC_PATH_EDIT);

    data->eventProcessing = true;
    SetWindowText(hPathEdit, data->applicationPaths[index].c_str());
    data->eventProcessing = false;

    HWND hOkButton = GetDlgItem(hDlg, IDOK);
    EnableWindow(hOkButton, true);
}

static void onPathEdit(HWND hDlg, int event) {
    KnceAppDlgData *data = (KnceAppDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    if (event != EN_CHANGE || data->eventProcessing)
        return;

    HWND hAppList = GetDlgItem(hDlg, IDC_APPLICATION_LIST);

    data->eventProcessing = true;
    SendMessage(hAppList, LB_SETCURSEL, -1, 0);
    data->eventProcessing = false;

    HWND hOkButton = GetDlgItem(hDlg, IDOK);
    EnableWindow(hOkButton, true);
}

static void onPathBrowse(HWND hDlg, int event) {
    KnceAppDlgData *data = (KnceAppDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    KnceChooseFileParams params = {0};
    params.isSaveFile = false;
    params.filters = _T("Programs (*.exe)|*.exe|All files (*.*)|*.*");

    if (knceChooseFile(hDlg, &params)) {
        HWND hPathEdit = GetDlgItem(hDlg, IDC_PATH_EDIT);
        SetWindowText(hPathEdit, params.fileName);
    }
}

static void updateApplicationList(HWND hDlg) {
    int i;

    KnceAppDlgData *data = (KnceAppDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

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

    HWND hAppList = GetDlgItem(hDlg, IDC_APPLICATION_LIST);
    SendMessage(hAppList, LB_RESETCONTENT, 0, 0);

    data->applicationPaths.clear();

    int numApps = captToPath.size();
    for (i = 0; i < numApps; i++) {
        SendMessage(hAppList, LB_ADDSTRING, 0,
            (LPARAM)captToPath[i].first.c_str());

        data->applicationPaths.push_back(captToPath[i].second);
    }

    HWND hOkButton = GetDlgItem(hDlg, IDOK);
    EnableWindow(hOkButton, false);
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
    tstring captLower = capt;
    transform(captLower.begin(), captLower.end(), captLower.begin(),
        _totlower);

    vector<pair<tstring, tstring> >::iterator iter = captToPath.begin();
    for ( ; iter != captToPath.end(); iter++) {
        tstring captTemp = iter->first;
        transform(captTemp.begin(), captTemp.end(), captTemp.begin(),
            _totlower);

        if (captTemp.compare(captLower) > 0)
            break;
    }

    captToPath.insert(iter, make_pair(capt, appFileName));
}
