#include "settingdlg.h"

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
	IDC_DISABLE_AT_ED_CHECK = 111,
	IDC_STORE_DATA_CHECK = 112,
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;

struct SettingDialogData {
	SettingDialogParams *dialogParams;
};

SettingDialogParams::SettingDialogParams() {
}

SettingDialogParams::~SettingDialogParams() {
}

bool showSettingDialog(HWND hOwnerWindow, SettingDialogParams &params) {
	int ret = DialogBoxParam(g_hInstance, _T("SETTINGS"), hOwnerWindow,
		(DLGPROC)dlgProc, (LPARAM)&params);

	return ret == IDOK;
}

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
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
	}

	return false;
}

static void onInitDialog(HWND hDlg, void *params) {

	SettingDialogData *data = new SettingDialogData();
	SetWindowLong(hDlg, GWL_USERDATA, (long)data);

	data->dialogParams = (SettingDialogParams *)params;

	KnceUtil::adjustDialogLayout(hDlg);

	HWND hDisableAtEdCheck = GetDlgItem(hDlg, IDC_DISABLE_AT_ED_CHECK);
	SendMessage(hDisableAtEdCheck, BM_SETCHECK, data->dialogParams->isDisableAtEd(), NULL);

	HWND hStoreDataCheck = GetDlgItem(hDlg, IDC_STORE_DATA_CHECK);
	SendMessage(hStoreDataCheck, BM_SETCHECK, data->dialogParams->isStoreData(), NULL);

	ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {

	SettingDialogData *data = (SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	HWND hDisableAtEdCheck = GetDlgItem(hDlg, IDC_DISABLE_AT_ED_CHECK);
	LRESULT isDisableAtEd = SendMessage(hDisableAtEdCheck, BM_GETCHECK, NULL, NULL);
	data->dialogParams->setDisableAtEd(isDisableAtEd == BST_CHECKED ? true : false);

	HWND hStoreDataCheck = GetDlgItem(hDlg, IDC_STORE_DATA_CHECK);
	LRESULT isStoreData = SendMessage(hStoreDataCheck, BM_GETCHECK, NULL, NULL);
	data->dialogParams->setStoreData(isStoreData == BST_CHECKED ? true : false);
	

	EndDialog(hDlg, IDOK);

	delete data;
}

static void onCancel(HWND hDlg) {

	SettingDialogData *data = (SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	EndDialog(hDlg, IDCANCEL);

	delete data;
}
