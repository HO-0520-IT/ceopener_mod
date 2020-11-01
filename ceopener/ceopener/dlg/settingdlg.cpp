#include "settingdlg.h"

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
	IDC_TAB_CONTROL = 111
};

static BOOL WINAPI settingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
	LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onFontBrowse(HWND hDlg);
static void onTabSelChange(HWND hDlg);
static void setupColorCombo(HWND hDlg);
static void setupWallpaperCombo(HWND hDlg);
static void updateFontLabel(HWND hDlg);

extern HINSTANCE g_hInstance;

struct SettingDialogData {
	SettingParams *settingParams;
	vector<HWND> hTabPages;
};

SettingParams::SettingParams() {
}

SettingParams::~SettingParams() {
}

bool showSettingDialog(HWND hOwnerWindow, SettingParams &params) {
	int ret = DialogBoxParam(g_hInstance, _T("SETTING_PARENT"),
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
		int id	= LOWORD(wParam); 
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
		int id = wParam;
		NMHDR *header = (NMHDR*)lParam;
		switch(id) {
			case IDC_TAB_CONTROL:
				if(header->code == TCN_SELCHANGE)
					onTabSelChange(hDlg);
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

	TCITEM tc;
	tc.mask = TCIF_TEXT;

	data->settingParams = (SettingParams *)params;

	KnceUtil::adjustDialogLayout(hDlg);

	HWND hTabCtrl = GetDlgItem(hDlg, IDC_TAB_CONTROL);

	HWND hViewingTab = showViewingDialog(hTabCtrl, data->settingParams->getViewingParams());
	data->hTabPages.push_back(hViewingTab);
	HWND hTransTab  = showTransSettingDialog(hTabCtrl, data->settingParams->getTransSettingParams());
	data->hTabPages.push_back(hTransTab);
	HWND hSETab = showSESettingDialog(hTabCtrl, data->settingParams->getSESettingParams());
	data->hTabPages.push_back(hSETab);

	tc.pszText = _T("•\Ž¦");
	TabCtrl_InsertItem(hTabCtrl, 0, &tc);
	tc.pszText = _T("”¼“§–¾");
	TabCtrl_InsertItem(hTabCtrl, 1, &tc);
	tc.pszText = _T("Œø‰Ê‰¹");
	TabCtrl_InsertItem(hTabCtrl, 2, &tc);

	for(unsigned int i = 0; i < data->hTabPages.size(); i++) {
		SetWindowPos(data->hTabPages[i], HWND_TOP, 5, 23, 0, 0, SWP_NOSIZE);
	}

	TabCtrl_SetCurSel(hTabCtrl, 0);
	ShowWindow(data->hTabPages[0], SW_SHOW);

	ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
	SettingDialogData *data =
		(SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	for(unsigned int i = 0; i < data->hTabPages.size(); i++) {
		SendMessage(data->hTabPages[i], WM_COMMAND, (WPARAM)IDOK, NULL);
	}

	EndDialog(hDlg, IDOK);

	delete data;
}

static void onCancel(HWND hDlg) {
	SettingDialogData *data =
		(SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	for(unsigned int i = 0; i < data->hTabPages.size(); i++) {
		SendMessage(data->hTabPages[i], WM_COMMAND, (WPARAM)IDCANCEL, NULL);
	}


	EndDialog(hDlg, IDCANCEL);

	delete data;
}

static void onTabSelChange(HWND hDlg) {
	SettingDialogData *data =
		(SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	HWND hTabCtrl = GetDlgItem(hDlg, IDC_TAB_CONTROL);
	int curSel = TabCtrl_GetCurSel(hTabCtrl);

	for(unsigned int i = 0; i < data->hTabPages.size(); i++) {
		ShowWindow(data->hTabPages[i], i == curSel ? SW_SHOW : SW_HIDE);
	}
}

