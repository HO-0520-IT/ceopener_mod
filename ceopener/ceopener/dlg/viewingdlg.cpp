#include "viewingdlg.h"

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
	IDC_FONT_LABEL = 102,
	IDC_FONT_BROWSE = 103,
	IDC_COLOR_COMBO = 112,
	IDC_WALLPAPER_COMBO = 114,
	IDC_SHOW_POPUPS_TASKBAR_CHECK = 121,
	IDC_CHANGE_TASKBAR_HEIGHT_CHECK = 122
};

static BOOL WINAPI viewingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
	LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onFontBrowse(HWND hDlg);
static void setupColorCombo(HWND hDlg);
static void setupWallpaperCombo(HWND hDlg);
static void updateFontLabel(HWND hDlg);

extern HINSTANCE g_hInstance;

struct ViewingDialogData {
	ViewingParams *viewingParams;
	HFONT hCurrentFont;
};

ViewingParams::ViewingParams() {
}

ViewingParams::~ViewingParams() {
}

HWND showViewingDialog(HWND hOwnerWindow, ViewingParams &params) {
	HWND hDlg = CreateDialogParam(g_hInstance, _T("VIEWING"),
		hOwnerWindow, (DLGPROC)viewingDlgProc, (LPARAM)&params);

	return hDlg;
}

static BOOL WINAPI viewingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
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
		case IDC_FONT_BROWSE:
			onFontBrowse(hDlg);
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
	ViewingDialogData *data = new ViewingDialogData();
	SetWindowLong(hDlg, GWL_USERDATA, (long)data);

	data->viewingParams = (ViewingParams *)params;

	KnceUtil::adjustDialogLayout(hDlg);

	tstring fontName;
	int pointSize = 0;
	bool isBold = false;
	bool isItalic = false;

	KnceUtil::getFontAttributes(data->viewingParams->getFont(), fontName,
		pointSize, isBold, isItalic);
	data->hCurrentFont = KnceUtil::createFont(fontName, pointSize, isBold,
		isItalic);

	setupColorCombo(hDlg);

	HWND hColorCombo = GetDlgItem(hDlg, IDC_COLOR_COMBO);

	tstring colorProfileName = data->viewingParams->getColorProfileName();
	if (colorProfileName.empty())
		SendMessage(hColorCombo, CB_SETCURSEL, 0, 0);
	else {
		int itemIndex = SendMessage(hColorCombo, CB_FINDSTRINGEXACT, -1,
			(LPARAM)colorProfileName.c_str());
		SendMessage(hColorCombo, CB_SETCURSEL, itemIndex, 0);
	}

	setupWallpaperCombo(hDlg);

	HWND hWallpaperCombo = GetDlgItem(hDlg, IDC_WALLPAPER_COMBO);

	tstring wallpaperName = data->viewingParams->getWallpaperName();
	if (wallpaperName.empty())
		SendMessage(hWallpaperCombo, CB_SETCURSEL, 0, 0);
	else {
		int itemIndex = SendMessage(hWallpaperCombo, CB_FINDSTRINGEXACT, -1,
			(LPARAM)wallpaperName.c_str());
		SendMessage(hWallpaperCombo, CB_SETCURSEL, itemIndex, 0);
	}

	HWND hShowPopupsTaskbarCheck = GetDlgItem(hDlg,
		IDC_SHOW_POPUPS_TASKBAR_CHECK);
	SendMessage(hShowPopupsTaskbarCheck, BM_SETCHECK,
		data->viewingParams->isShowPopupsTaskbar(), 0);

	HWND hChangeTaskbarHeightCheck = GetDlgItem(hDlg,
		IDC_CHANGE_TASKBAR_HEIGHT_CHECK);
	SendMessage(hChangeTaskbarHeightCheck, BM_SETCHECK,
		data->viewingParams->isChangeTaskbarHeight(), 0);

	updateFontLabel(hDlg);
}

static void onOk(HWND hDlg) {
	ViewingDialogData *data =
		(ViewingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	data->viewingParams->setFont(data->hCurrentFont);

	HWND hColorCombo = GetDlgItem(hDlg, IDC_COLOR_COMBO);

	TCHAR colorProfileNameCStr[MAX_PATH];
	GetWindowText(hColorCombo, colorProfileNameCStr, MAX_PATH);
	tstring colorProfileName = colorProfileNameCStr;

	if (colorProfileName == _T("(•W€)"))
		data->viewingParams->setColorProfileName(_T(""));
	else
		data->viewingParams->setColorProfileName(colorProfileNameCStr);

	HWND hWallpaperCombo = GetDlgItem(hDlg, IDC_WALLPAPER_COMBO);

	TCHAR wallpaperNameCStr[MAX_PATH];
	GetWindowText(hWallpaperCombo, wallpaperNameCStr, MAX_PATH);
	tstring wallpaperName = wallpaperNameCStr;

	if (wallpaperName == _T("(‚È‚µ)"))
		data->viewingParams->setWallpaperName(_T(""));
	else
		data->viewingParams->setWallpaperName(wallpaperNameCStr);

	HWND hShowPopupsTaskbarCheck = GetDlgItem(hDlg,
		IDC_SHOW_POPUPS_TASKBAR_CHECK);
	data->viewingParams->setShowPopupsTaskbar(
		SendMessage(hShowPopupsTaskbarCheck, BM_GETCHECK, 0, 0) != 0);

	HWND hChangeTaskbarHeightCheck = GetDlgItem(hDlg,
		IDC_CHANGE_TASKBAR_HEIGHT_CHECK);
	data->viewingParams->setChangeTaskbarHeight(
		SendMessage(hChangeTaskbarHeightCheck, BM_GETCHECK, 0, 0) != 0);

	EndDialog(hDlg, IDOK);

	delete data;
}

static void onCancel(HWND hDlg) {
	ViewingDialogData *data =
		(ViewingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	DeleteObject(data->hCurrentFont);

	EndDialog(hDlg, IDCANCEL);

	delete data;
}

static void onFontBrowse(HWND hDlg) {
	ViewingDialogData *data =
		(ViewingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	KnceChooseFontParams params = {0};
	params.hFont = data->hCurrentFont;
	params.isFixedOnly = false;

	if (knceChooseFont(hDlg, &params)) {
		DeleteObject(data->hCurrentFont);
		data->hCurrentFont = params.hFont;

		updateFontLabel(hDlg);
	}
}

static void setupColorCombo(HWND hDlg) {
	HWND hColorCombo = GetDlgItem(hDlg, IDC_COLOR_COMBO);

	SendMessage(hColorCombo, CB_ADDSTRING, 0, (LPARAM)_T("(•W€)"));

	tstring pat = _T("*.dat");

	tstring curDir = KnceUtil::getCurrentDirectory();

	tstring colorProfileDir = curDir + _T("\\colors");
	tstring findPath = colorProfileDir + _T("\\*.*");

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(findPath.c_str(), &fd);
	
	if (hFind != INVALID_HANDLE_VALUE) {
		if (KnceUtil::matchMultiFileExtension(fd.cFileName, pat))
			SendMessage(hColorCombo, CB_ADDSTRING, 0, (LPARAM)fd.cFileName);

		while (FindNextFile(hFind, &fd)) {
			if (KnceUtil::matchMultiFileExtension(fd.cFileName, pat)) {
				SendMessage(hColorCombo, CB_ADDSTRING, 0,
					(LPARAM)fd.cFileName);
			}
		}
	}

	FindClose(hFind);
}

static void setupWallpaperCombo(HWND hDlg) {
	HWND hWallpaperCombo = GetDlgItem(hDlg, IDC_WALLPAPER_COMBO);

	SendMessage(hWallpaperCombo, CB_ADDSTRING, 0, (LPARAM)_T("(‚È‚µ)"));

	tstring pat = _T("*.bmp;*.jpg");

	tstring curDir = KnceUtil::getCurrentDirectory();

	tstring wallpaperDir = curDir + _T("\\wallpapers");
	tstring findPath = wallpaperDir + _T("\\*.*");

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(findPath.c_str(), &fd);
	
	if (hFind != INVALID_HANDLE_VALUE) {
		if (KnceUtil::matchMultiFileExtension(fd.cFileName, pat))
			SendMessage(hWallpaperCombo, CB_ADDSTRING, 0, (LPARAM)fd.cFileName);

		while (FindNextFile(hFind, &fd)) {
			if (KnceUtil::matchMultiFileExtension(fd.cFileName, pat)) {
				SendMessage(hWallpaperCombo, CB_ADDSTRING, 0,
					(LPARAM)fd.cFileName);
			}
		}
	}

	FindClose(hFind);
}

static void updateFontLabel(HWND hDlg) {
	ViewingDialogData *data =
		(ViewingDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	tstring fontName;
	int pointSize = 0;
	bool isBold = false;
	bool isItalic = false;

	KnceUtil::getFontAttributes(data->hCurrentFont, fontName, pointSize,
		isBold, isItalic);

	TCHAR sizeCStr[16];
	_sntprintf(sizeCStr, 16, _T("%dpt"), pointSize / 10);

	HWND hFontLabel = GetDlgItem(hDlg, IDC_FONT_LABEL);
	SetWindowText(hFontLabel, (fontName + _T(", ") + sizeCStr).c_str());
}
