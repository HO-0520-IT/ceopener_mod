#include "settingdlg.h"

#include <string>
#include <windows.h>
#include <knceutil.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
	IDC_FONT_SIZE_COMBO = 102,
	IDC_USE_SUB_TOUCH_CHECK = 111,
	IDC_LARGE_PAD_CHECK = 112,
	IDC_DISABLE_AT_ED_CHECK = 113,
	IDC_SWAP_HIPHEN_CHECK = 114
};

static BOOL WINAPI settingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
	LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);

extern HINSTANCE g_hInstance;

struct SettingDialogData {
	SettingParams *settingParams;
};

SettingParams::SettingParams() {
	m_fontSize = 0;
	m_isSubTouchUseEnabled = false;
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
	int i;

	SettingDialogData *data = new SettingDialogData();
	SetWindowLong(hDlg, GWL_USERDATA, (long)data);

	data->settingParams = (SettingParams *)params;

	KnceUtil::adjustDialogLayout(hDlg);

	HWND hFontSizeCombo = GetDlgItem(hDlg, IDC_FONT_SIZE_COMBO);

	static int fontSizes[] = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26,
		28, 36, 48, 72};

	TCHAR sizeCStr[16];
	int numSizes = sizeof(fontSizes) / sizeof(int);

	for (i = 0; i < numSizes; i++) {
		_sntprintf(sizeCStr, 16, _T("%d"), fontSizes[i]);
		SendMessage(hFontSizeCombo, CB_ADDSTRING, 0, (LPARAM)sizeCStr);
	}

	_sntprintf(sizeCStr, 16, _T("%d"),
		data->settingParams->getFontSize() / 10);
	int itemIndex = SendMessage(hFontSizeCombo, CB_FINDSTRINGEXACT, -1,
		(LPARAM)sizeCStr);
	SendMessage(hFontSizeCombo, CB_SETCURSEL, itemIndex, 0);

	HWND hUseSubTouchCheck = GetDlgItem(hDlg, IDC_USE_SUB_TOUCH_CHECK);
	SendMessage(hUseSubTouchCheck, BM_SETCHECK,
		data->settingParams->isSubTouchUseEnabled(), 0);

	HWND hLargePadCheck = GetDlgItem(hDlg, IDC_LARGE_PAD_CHECK);
	SendMessage(hLargePadCheck, BM_SETCHECK,
		data->settingParams->isLargePad(), 0);

	HWND hDisableAtEdCheck = GetDlgItem(hDlg, IDC_DISABLE_AT_ED_CHECK);
	SendMessage(hDisableAtEdCheck, BM_SETCHECK,
		data->settingParams->isDisableAtEd(), 0);

	HWND hSwapHiphen = GetDlgItem(hDlg, IDC_SWAP_HIPHEN_CHECK);
	SendMessage(hSwapHiphen, BM_SETCHECK,
		data->settingParams->isSwapHiphen(), 0);

	ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
	SettingDialogData *data =
		(SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	HWND hFontSizeCombo = GetDlgItem(hDlg, IDC_FONT_SIZE_COMBO);

	TCHAR sizeCStr[16];
	GetWindowText(hFontSizeCombo, sizeCStr, 16);
	int pointSize = _ttoi(sizeCStr) * 10;
	data->settingParams->setFontSize(pointSize);

	HWND hUseSubTouchCheck = GetDlgItem(hDlg, IDC_USE_SUB_TOUCH_CHECK);
	data->settingParams->setSubTouchUseEnabled(
		SendMessage(hUseSubTouchCheck, BM_GETCHECK, 0, 0) != 0);
	HWND hLargePadCheck = GetDlgItem(hDlg, IDC_LARGE_PAD_CHECK);
	data->settingParams->setLargePad(
		SendMessage(hLargePadCheck, BM_GETCHECK, 0, 0) != 0);
	HWND hDisableAtEdCheck = GetDlgItem(hDlg, IDC_DISABLE_AT_ED_CHECK);
	data->settingParams->setDisableAtEd(
		SendMessage(hDisableAtEdCheck, BM_GETCHECK, 0, 0) != 0);
	HWND hSwapHiphen = GetDlgItem(hDlg, IDC_SWAP_HIPHEN_CHECK);
	data->settingParams->setSwapHiphen(
		SendMessage(hSwapHiphen, BM_GETCHECK, 0, 0) != 0);

	EndDialog(hDlg, IDOK);

	delete data;
}

static void onCancel(HWND hDlg) {
	SettingDialogData *data =
		(SettingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	EndDialog(hDlg, IDCANCEL);

	delete data;
}
