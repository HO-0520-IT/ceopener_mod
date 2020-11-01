#include "settingdlg.h"

#include <string>
#include <windows.h>
#include <knceutil.h>
#include <kncedlg.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_STORE_DIRECTORY_EDIT = 104,
    IDC_STORE_DIRECTORY_BROWSE_BUTTON = 105
};

static BOOL WINAPI settingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
    LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onStoreDirectoryBrowse(HWND hDlg);

extern HINSTANCE g_hInstance;

struct SettingDialogData {
    SettingParams *settingParams;
};

SettingParams::SettingParams() {
}

SettingParams::~SettingParams() {
}

bool showSettingDialog(HWND hOwnerWindow, SettingParams &params) {
    int ret = DialogBoxParam(g_hInstance, _T("SETTINGS"),
        hOwnerWindow, (DLGPROC)settingDlgProc, (LPARAM)&params);

    return ret == IDOK;
}

static BOOL WINAPI settingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
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
        case IDC_STORE_DIRECTORY_BROWSE_BUTTON:
            onStoreDirectoryBrowse(hDlg);
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
    SettingDialogData *data = new SettingDialogData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->settingParams = (SettingParams *)params;

    KnceUtil::adjustDialogLayout(hDlg);

    HWND hDirEdit = GetDlgItem(hDlg, IDC_STORE_DIRECTORY_EDIT);
    SetWindowText(hDirEdit, data->settingParams->getStoreDirectory().c_str());

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    SettingDialogData *data =
        (SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hDirEdit = GetDlgItem(hDlg, IDC_STORE_DIRECTORY_EDIT);

    TCHAR dirCStr[MAX_PATH];
    GetWindowText(hDirEdit, dirCStr, MAX_PATH);

    data->settingParams->setStoreDirectory(dirCStr);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    SettingDialogData *data =
        (SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onStoreDirectoryBrowse(HWND hDlg) {
    KnceChooseDirectoryParams params = {0};
    _tcscpy(params.initialPath, _T("\\"));

    if (knceChooseDirectory(hDlg, &params)) {
        HWND hDirEdit = GetDlgItem(hDlg, IDC_STORE_DIRECTORY_EDIT);
        SetWindowText(hDirEdit, params.directory);
    }
}
