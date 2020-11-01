#include "specialicondlg.h"

#include <string>
#include <windows.h>
#include <commctrl.h>
#include <knceutil.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_ICON_VIEW = 101,
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onItemChanged(HWND hDlg, NMLISTVIEW *nmv);
static void updateControlStates(HWND hDlg);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;

struct SpecialIconDialogData {
    SpecialIconDialogParams *dialogParams;
    HIMAGELIST hImageList;
};

SpecialIconDialogParams::SpecialIconDialogParams() {
    m_iconType = 0;
}

SpecialIconDialogParams::~SpecialIconDialogParams() {
}

bool showSpecialIconDialog(HWND hOwnerWindow,
    SpecialIconDialogParams &params) {

    int ret = DialogBoxParam(g_hInstance, _T("CHOOSE_SPECIAL_ICON"),
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
        default:
            return false;
        }

        return true;
    }
    case WM_NOTIFY:
    {
        NMHDR *nmHdr = (NMHDR *)lParam;
        switch (nmHdr->code) {
        case LVN_ITEMCHANGED:
            onItemChanged(hDlg, (NMLISTVIEW *)lParam);
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
    SpecialIconDialogData *data = new SpecialIconDialogData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->dialogParams = (SpecialIconDialogParams *)params;

    KnceUtil::adjustDialogLayout(hDlg);

    data->hImageList = ImageList_Create(32, 32, ILC_COLOR | ILC_MASK,
        1, 1);

    ImageList_AddIcon(data->hImageList, (HICON)LoadImage(g_hInstance,
        _T("LAPTOP"), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
    ImageList_AddIcon(data->hImageList, (HICON)LoadImage(g_hInstance,
        _T("TOOLS"), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
    ImageList_AddIcon(data->hImageList, (HICON)LoadImage(g_hInstance,
        _T("DRIVE"), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));
    ImageList_AddIcon(data->hImageList, (HICON)LoadImage(g_hInstance,
        _T("SD_CARD"), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR));

    HWND hIconView = GetDlgItem(hDlg, IDC_ICON_VIEW);
    ListView_SetImageList(hIconView, data->hImageList, LVSIL_NORMAL);

    LVITEM item = {0};
    item.mask = LVIF_TEXT | LVIF_IMAGE;
    item.iImage = 0;
    item.iItem = 0;
    item.pszText = _T("マイ デバイス");

    ListView_InsertItem(hIconView, &item);

    item.iImage = 1;
    item.iItem = 1;
    item.pszText = _T("コントロール パネル");
    ListView_InsertItem(hIconView, &item);

    item.iImage = 2;
    item.iItem = 2;
    item.pszText = _T("Nand3");
    ListView_InsertItem(hIconView, &item);

    item.iImage = 3;
    item.iItem = 3;
    item.pszText = _T("SD カード");
    ListView_InsertItem(hIconView, &item);

    ListView_SetItemState(hIconView, 0, LVIS_SELECTED, LVIS_SELECTED);

    updateControlStates(hDlg);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    SpecialIconDialogData *data = (SpecialIconDialogData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    HWND hIconView = GetDlgItem(hDlg, IDC_ICON_VIEW);
    int index = ListView_GetNextItem(hIconView, -1, LVNI_ALL | LVNI_SELECTED);

    data->dialogParams->setIconType(index);

    ImageList_Destroy(data->hImageList);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    SpecialIconDialogData *data = (SpecialIconDialogData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    ImageList_Destroy(data->hImageList);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onItemChanged(HWND hDlg, NMLISTVIEW *nmv) {
    updateControlStates(hDlg);
}

static void updateControlStates(HWND hDlg) {
    HWND hOkButton = GetDlgItem(hDlg, IDOK);

    HWND hIconView = GetDlgItem(hDlg, IDC_ICON_VIEW);
    int index = ListView_GetNextItem(hIconView, -1, LVNI_ALL | LVNI_SELECTED);

    if (index == -1)
        EnableWindow(hOkButton, false);
    else
        EnableWindow(hOkButton, true);
}
