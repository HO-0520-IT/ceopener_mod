#include "edititemdlg.h"

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
    IDC_CAPTION_EDIT = 102,
    IDC_APPLICATION_EDIT = 112,
    IDC_APPLICATION_BROWSE_BUTTON = 113
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onCaptionEdit(HWND hDlg, int event);
static void onApplicationEdit(HWND hDlg, int event);
static void onApplicationBrowse(HWND hDlg);
static void updateOkButton(HWND hDlg);
static void browseApplication(HWND hDlg);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;

struct EditItemDialogData {
    EditItemDialogParams *dialogParams;
};

EditItemDialogParams::EditItemDialogParams() {
    m_browsingOnStartup = false;
}

EditItemDialogParams::~EditItemDialogParams() {
}

bool showEditItemDialog(HWND hOwnerWindow, EditItemDialogParams &params) {
    int ret = DialogBoxParam(g_hInstance, _T("EDIT_ITEM"),
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
        case IDC_CAPTION_EDIT:
            onCaptionEdit(hDlg, event);
            break;
        case IDC_APPLICATION_EDIT:
            onApplicationEdit(hDlg, event);
            break;
        case IDC_APPLICATION_BROWSE_BUTTON:
            onApplicationBrowse(hDlg);
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
    EditItemDialogData *data = new EditItemDialogData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->dialogParams = (EditItemDialogParams *)params;

    KnceUtil::adjustDialogLayout(hDlg);

    HWND hCaptionEdit = GetDlgItem(hDlg, IDC_CAPTION_EDIT);
    SetWindowText(hCaptionEdit, data->dialogParams->getCaption().c_str());

    HWND hAppEdit = GetDlgItem(hDlg, IDC_APPLICATION_EDIT);
    SetWindowText(hAppEdit, data->dialogParams->getApplicationPath().c_str());

    updateOkButton(hDlg);

    ShowWindow(hDlg, SW_SHOW);

    if (data->dialogParams->isBrowsingOnStartup())
        browseApplication(hDlg);
}

static void onOk(HWND hDlg) {
    EditItemDialogData *data = (EditItemDialogData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    TCHAR captionCStr[256];
    HWND hCaptionEdit = GetDlgItem(hDlg, IDC_CAPTION_EDIT);
    GetWindowText(hCaptionEdit, captionCStr, 256);

    data->dialogParams->setCaption(captionCStr);

    TCHAR fileNameCStr[MAX_PATH];
    HWND hAppEdit = GetDlgItem(hDlg, IDC_APPLICATION_EDIT);
    GetWindowText(hAppEdit, fileNameCStr, 256);

    data->dialogParams->setCaption(captionCStr);
    data->dialogParams->setApplicationPath(fileNameCStr);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    EditItemDialogData *data = (EditItemDialogData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onCaptionEdit(HWND hDlg, int event) {
    EditItemDialogData *data = (EditItemDialogData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    if (event == EN_CHANGE)
        updateOkButton(hDlg);
}

static void onApplicationEdit(HWND hDlg, int event) {
    EditItemDialogData *data = (EditItemDialogData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    if (event == EN_CHANGE)
        updateOkButton(hDlg);
}

static void onApplicationBrowse(HWND hDlg) {
    browseApplication(hDlg);
}

static void updateOkButton(HWND hDlg) {
    HWND hCaptionEdit = GetDlgItem(hDlg, IDC_CAPTION_EDIT);
    bool enabled = GetWindowTextLength(hCaptionEdit) > 0;

    HWND hAppEdit = GetDlgItem(hDlg, IDC_APPLICATION_EDIT);
    enabled = enabled || GetWindowTextLength(hAppEdit) > 0;

    HWND hOkButton = GetDlgItem(hDlg, IDOK);
    EnableWindow(hOkButton, enabled);
}

static void browseApplication(HWND hDlg) {
    KnceChooseApplicationParams params = {0};

    HWND hAppEdit = GetDlgItem(hDlg, IDC_APPLICATION_EDIT);
    GetWindowText(hAppEdit, params.fileName, 256);

    if (!knceChooseApplication(hDlg, &params))
        return;

    HWND hCaptionEdit = GetDlgItem(hDlg, IDC_CAPTION_EDIT);
    SetWindowText(hCaptionEdit, params.title);

    SetWindowText(hAppEdit, params.fileName);
}
