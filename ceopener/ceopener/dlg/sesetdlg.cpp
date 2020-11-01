#include "sesetdlg.h"

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
	IDC_SE_LISTVIEW = 111,
	IDC_PLAY_BUTTON = 121,
	IDC_STOP_BUTTON = 122,
	IDC_BROWSE_BUTTON = 123,
	IDC_FILENAME_EDIT = 131
};

static BOOL WINAPI sesetDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
	LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onListViewSelChange(HWND hDlg, int iItem);
static void onPlay(HWND hDlg);
static void onStop(HWND hDlg);
static void onBrowse(HWND hDlg);
static int ListView_GetNextChecked(HWND hLV, int iStart);

extern HINSTANCE g_hInstance;

struct SESettingDialogData {
	SESettingParams *sesetParams;
	int curSel;
};

SESettingParams::SESettingParams() {
}

SESettingParams::~SESettingParams() {
}

HWND showSESettingDialog(HWND hOwnerWindow, SESettingParams &params) {
	HWND hDlg = CreateDialogParam(g_hInstance, _T("SE_SETTING"),
		hOwnerWindow, (DLGPROC)sesetDlgProc, (LPARAM)&params);

	return hDlg;
}

static BOOL WINAPI sesetDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		onInitDialog(hDlg, (void *)lParam);
		return true;
	case WM_COMMAND:
	{
		int id	= LOWORD(wParam); 
		int event = HIWORD(wParam);

		switch (id) {
		case IDOK:
			onOk(hDlg);
			break;
		case IDCANCEL:
			onCancel(hDlg);
			break;
		case IDC_PLAY_BUTTON:
			onPlay(hDlg);
			break;
		case IDC_STOP_BUTTON:
			onStop(hDlg);
			break;
		case IDC_BROWSE_BUTTON:
			onBrowse(hDlg);
			break;
		default:
			return false;
		}

		return true;
	}
	case WM_NOTIFY:
	{
		int id = wParam;
		NMHDR *header = (NMHDR*)lParam;

		switch(id) {
			case IDC_SE_LISTVIEW:
				if(header->code == LVN_ITEMCHANGED) {
					NMLISTVIEW *nmLV = (NMLISTVIEW*)lParam;
					if((nmLV->uChanged & LVIF_STATE)
						&& (nmLV->uNewState & LVIS_SELECTED)) {
							onListViewSelChange(hDlg, nmLV->iItem);
					}
				}
				break;
			default:
				return false;
		}

		return true;
	}
		break;
	}

	return false;
}

static void onInitDialog(HWND hDlg, void *params) {
	SESettingDialogData *data = new SESettingDialogData();
	SetWindowLong(hDlg, GWL_USERDATA, (long)data);

	data->sesetParams = (SESettingParams *)params;
	data->curSel = -1;

	KnceUtil::adjustDialogLayout(hDlg);

	LVCOLUMN lc;
	lc.mask = LVCF_TEXT | LVCF_WIDTH;
	LVITEM li;
	li.mask = LVIF_TEXT;
	DWORD dwStyle;

	HWND hSEListView = GetDlgItem(hDlg, IDC_SE_LISTVIEW);
	dwStyle = ListView_GetExtendedListViewStyle(hSEListView);
	dwStyle |= LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
	ListView_SetExtendedListViewStyle(hSEListView, dwStyle);
	lc.pszText = _T("アクション");
	lc.cx = 9 * 10;
	ListView_InsertColumn(hSEListView, 0, &lc);
	lc.pszText = _T("ファイル");
	lc.cx = 9 * 60;
	ListView_InsertColumn(hSEListView, 1, &lc);

	vector<SEController::SoundEffect*> soundEffects = data->sesetParams->getSEController().getSoundEffects();

	for(unsigned int i = 0; i < soundEffects.size(); i++) {
		LPTSTR buf;
		buf = (LPTSTR)HeapAlloc(GetProcessHeap(), NULL, soundEffects[i]->verb().length() * sizeof(TCHAR) + sizeof(TCHAR));
		lstrcpy(buf, soundEffects[i]->verb().c_str());
		li.pszText = buf;
		li.iItem = i;
		li.iSubItem = 0;
		ListView_InsertItem(hSEListView, &li);
		HeapFree(GetProcessHeap(), NULL, buf);
		buf = (LPTSTR)HeapAlloc(GetProcessHeap(), NULL, soundEffects[i]->fileName().length() * sizeof(TCHAR) + sizeof(TCHAR));
		lstrcpy(buf, soundEffects[i]->fileName().c_str());
		li.pszText = buf;
		li.iSubItem = 1;
		ListView_SetItem(hSEListView, &li);
		HeapFree(GetProcessHeap(), NULL, buf);
		ListView_SetCheckState(hSEListView, i, soundEffects[i]->isEnabled());
	}

}

static void onOk(HWND hDlg) {
	SESettingDialogData *data =
		(SESettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	LPTSTR buf = new TCHAR[MAX_PATH];
	LPTSTR verbCStr = new TCHAR[MAX_PATH],
		fileCStr = new TCHAR[MAX_PATH];
	LVITEM li;
	SEController &seController = data->sesetParams->getSEController();

	HWND hSEListView = GetDlgItem(hDlg, IDC_SE_LISTVIEW);
	HWND hFileNameEdit = GetDlgItem(hDlg, IDC_FILENAME_EDIT);

	GetWindowText(hFileNameEdit, buf, MAX_PATH);
	li.mask = LVIF_TEXT;
	li.iItem = data->curSel;
	li.iSubItem = 1;
	li.pszText = buf;
	ListView_SetItem(hSEListView, &li);

	int nCount = ListView_GetItemCount(hSEListView);
	li.cchTextMax = MAX_PATH;
	li.mask = LVIF_TEXT;

	for(int i = 0; i < nCount; i++) {
		li.iItem = i;
		li.iSubItem = 0;
		li.pszText = verbCStr;
		
		ListView_GetItem(hSEListView, &li);

		li.iSubItem = 1;
		li.pszText = fileCStr;
		ListView_GetItem(hSEListView, &li);

		seController[verbCStr] = fileCStr;
		seController[verbCStr].setEnabled(ListView_GetCheckState(hSEListView, i) ? true : false);
	}

	EndDialog(hDlg, IDOK);

	delete fileCStr;
	delete verbCStr;
	delete buf;
	delete data;
}

static void onCancel(HWND hDlg) {
	SESettingDialogData *data =
		(SESettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	EndDialog(hDlg, IDCANCEL);

	delete data;
}

static void onListViewSelChange(HWND hDlg, int iItem) {
	SESettingDialogData *data =
		(SESettingDialogData*)GetWindowLong(hDlg, GWL_USERDATA);

	LPTSTR buf = new TCHAR[MAX_PATH * sizeof(TCHAR)];
	LVITEM li;

	HWND hSEListView = GetDlgItem(hDlg, IDC_SE_LISTVIEW);

	HWND hFileNameEdit = GetDlgItem(hDlg, IDC_FILENAME_EDIT);
	GetWindowText(hFileNameEdit, buf, MAX_PATH);
	li.mask = LVIF_TEXT;
	li.pszText = buf;
	li.iItem = data->curSel;
	li.iSubItem = 1;
	ListView_SetItem(hSEListView, &li);

	li.mask = LVIF_TEXT;
	li.pszText = buf;
	li.iItem = iItem;
	li.iSubItem = 1;
	li.cchTextMax = MAX_PATH;
	ListView_GetItem(hSEListView, &li);

	SetWindowText(hFileNameEdit, buf);

	data->curSel = iItem;

	delete buf;
}

static void onPlay(HWND hDlg) {
	LPTSTR buf = new TCHAR[MAX_PATH * sizeof(TCHAR)];

	HWND hFileNameEdit = GetDlgItem(hDlg, IDC_FILENAME_EDIT);
	GetWindowText(hFileNameEdit, buf, MAX_PATH);
	BOOL res = PlaySound(buf, g_hInstance, SND_FILENAME | SND_ASYNC);

	delete buf;
}

static void onStop(HWND hDlg) {
	PlaySound(NULL, g_hInstance, NULL);
}

static int ListView_GetNextChecked(HWND hLV, int iStart) {
	int nCount = ListView_GetItemCount(hLV);

	for(int i = iStart; i < nCount; i++) {
		if(ListView_GetCheckState(hLV, i) != 0)
			return i;
	}

	return -1;
}

static void onBrowse(HWND hDlg) {
	SESettingDialogData *data =
		(SESettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	LPTSTR buf = new TCHAR[MAX_PATH];

	HWND hFileNameEdit = GetDlgItem(hDlg, IDC_FILENAME_EDIT);
	GetWindowText(hFileNameEdit, buf, MAX_PATH);

	KnceChooseFileParams params = {0};
	_tcsncpy(params.fileName, buf, MAX_PATH);
    params.isSaveFile = false;
    params.filters = _T("WAVEサウンド (*.wav)|*.wav");

    if (!knceChooseFile(hDlg, &params))
        return;
	
	SetWindowText(hFileNameEdit, params.fileName);

	delete buf;
}