#include "transsetdlg.h"

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
	IDC_OK_BUTTON = 101,
	IDC_CANCEL_BUTTON = 102,
	IDC_PARAMS_LIST = 111,
	IDC_UNTRANS_RADIO = 121,
	IDC_TRANS25_RADIO = 122,
	IDC_TRANS50_RADIO = 123,
	IDC_TRANS75_RADIO = 124,
	RADIO_COUNT = 4
};

static LPTSTR suffixes[] = {
	_T(" - 100%"),
	_T(" - 75%"),
	_T(" - 50%"),
	_T(" - 25%")
};

static BOOL WINAPI settingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
	LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onChangeParams(HWND hDlg);
static void onChangeRadio(HWND hDlg, UINT id);

extern HINSTANCE g_hInstance;

struct TransSetDialogData {
	TransSettingParams *settingParams;
	vector<UINT> transparencyCache;
	vector<tstring> listBoxStr;
	int curSel;
};

TransSettingParams::TransSettingParams() {
}

TransSettingParams::~TransSettingParams() {
}

HWND showTransSettingDialog(HWND hOwnerWindow, TransSettingParams &params) {
	HWND hDlg = CreateDialogParam(g_hInstance, _T("TRANS_SETTING"),
		hOwnerWindow, (DLGPROC)settingDlgProc, (LPARAM)&params);

	return hDlg;
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
		case IDC_PARAMS_LIST:
			if(HIWORD(wParam) == LBN_SELCHANGE)
				onChangeParams(hDlg);
			break;
		default:
	if (id >= IDC_UNTRANS_RADIO && id <= IDC_TRANS75_RADIO) {
		onChangeRadio(hDlg, id);
	}
			return false;
		}

		return true;
	}
	}

	return false;
}

static void onInitDialog(HWND hDlg, void *params) {
	TransSetDialogData *data = new TransSetDialogData();
	data->curSel = 0;
	SetWindowLong(hDlg, GWL_USERDATA, (long)data);

	data->settingParams = (TransSettingParams *)params;

	KnceUtil::adjustDialogLayout(hDlg);

	HWND hParamsList = GetDlgItem(hDlg, IDC_PARAMS_LIST);

	SendMessage(hParamsList, LB_INSERTSTRING, 0, (LPARAM)_T("ˆêŠ‡Ý’è"));
	data->listBoxStr.push_back(tstring(_T("ˆêŠ‡Ý’è")));
	data->transparencyCache.push_back(NULL);

	int index = 1;
	TaskItemTransparency *pItemTransparency = data->settingParams->getTaskItemTransparency();
	TaskItemTransparencyVector::iterator iter = pItemTransparency->begin();
	for (; iter != pItemTransparency->end(); iter++) {
		if (iter->second.size() == 1) {
			TCHAR instStr[MAX_PATH + 8];
			LPTSTR modName = (LPTSTR)iter->first.c_str();
			int transparency = *iter->second.begin()->second;

			_tcscpy(instStr, modName);
			_tcscat(instStr, suffixes[transparency]);
			SendMessage(hParamsList, LB_INSERTSTRING, index, (LPARAM)instStr);
			index++;

			data->transparencyCache.push_back(transparency);
			data->listBoxStr.push_back(modName);
		}
		else {
			int subIndex = 1;
			map<UINT, UINT*>::iterator subIter = iter->second.begin();
			TCHAR modName[MAX_PATH + 8];
			TCHAR numStr[8];
			TCHAR *endPtr = (modName + lstrlen(modName));

			_tcscpy(modName, iter->first.c_str());
			
			for (; subIter != iter->second.end(); subIter++) {
				_ltow(subIndex, numStr, 10);
				_tcscat(modName, numStr);
				subIndex++;
				data->listBoxStr.push_back(tstring(modName));

				int transparency = *iter->second.begin()->second;
				if (transparency > 3)
					transparency = 0;
				
				_tcscat(modName, suffixes[transparency]);

				SendMessage(hParamsList, LB_INSERTSTRING, index, (LPARAM)modName);
				index++;
				
				data->transparencyCache.push_back(*iter->second.begin()->second);
				
				*endPtr = _T('\0');
			}
		}
	}
	SendMessage(hParamsList, LB_SETCURSEL, 0, 0);
}

static void onOk(HWND hDlg) {
	TransSetDialogData *data =
		(TransSetDialogData *)GetWindowLong(hDlg, GWL_USERDATA);
	
	TaskItemTransparency *pItemTransparency = data->settingParams->getTaskItemTransparency();
	TaskItemTransparencyVector::iterator iter = pItemTransparency->begin();
	for (int index = 1; iter != pItemTransparency->end(); iter++, index++) {
		if (iter->second.size() == 1) {
			int transparency = data->transparencyCache[index];

			*iter->second.begin()->second =  transparency;
		}
		else {
			int subIndex = 1;
			map<UINT, UINT*>::iterator subIter = iter->second.begin();
			TCHAR modName[MAX_PATH + 8];

			_tcscpy(modName, iter->first.c_str());
			
			for (; subIter != iter->second.end(); subIter++, index++) {
				int transparency = data->transparencyCache[index];
				
				*subIter->second = transparency;
			}
		}
	}


	EndDialog(hDlg, IDOK);

	delete data;
}

static void onCancel(HWND hDlg) {
	TransSetDialogData *data =
		(TransSetDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	EndDialog(hDlg, IDCANCEL);

	delete data;
}

static void onChangeParams(HWND hDlg) {
	TransSetDialogData *data =
		(TransSetDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	HWND hListParams = GetDlgItem(hDlg, IDC_PARAMS_LIST);

	int index = SendMessage(hListParams, LB_GETCURSEL, 0, 0);
	data->curSel = index;

	for (int i = IDC_UNTRANS_RADIO; i <= IDC_TRANS75_RADIO; i++) {
		HWND hDestRadio = GetDlgItem(hDlg, i);
		SendMessage(hDestRadio, BM_SETCHECK, FALSE, 0);
	}

	if (index == -1)
		return;
	else if (index == 0) {
	}
	else {
		HWND hDestRadio = GetDlgItem(hDlg, data->transparencyCache[index] + IDC_UNTRANS_RADIO);
		SendMessage(hDestRadio, BM_SETCHECK, TRUE, 0);
	}
}

static void onChangeRadio(HWND hDlg, UINT id) {
	TransSetDialogData *data =
		(TransSetDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	TCHAR newStr[MAX_PATH + 10];
	
	HWND hListParams = GetDlgItem(hDlg, IDC_PARAMS_LIST);
	SendMessage(hListParams, WM_SETREDRAW, FALSE, 0);
	
	int transparency = id - IDC_UNTRANS_RADIO;
	

	if (data->curSel == 0) {
		int itemcount = data->listBoxStr.size();

		for (int i = 1; i < itemcount; i++) {
			data->transparencyCache[i] = transparency;

			_tcscpy(newStr, data->listBoxStr[i].c_str());
			_tcscat(newStr, suffixes[transparency]);
			
			SendMessage(hListParams, LB_DELETESTRING, i, 0);
			SendMessage(hListParams, LB_INSERTSTRING, i, (LPARAM)newStr);
		}
	}
	else if (data->curSel >= 1) {	
		data->transparencyCache[data->curSel] = transparency;

		_tcscpy(newStr, data->listBoxStr[data->curSel].c_str());
		_tcscat(newStr, suffixes[transparency]);

		SendMessage(hListParams, LB_DELETESTRING, data->curSel, 0);
		SendMessage(hListParams, LB_INSERTSTRING, data->curSel, (LPARAM)newStr);
		SendMessage(hListParams, LB_SETCURSEL, data->curSel, 0);
	}

	SendMessage(hListParams, WM_SETREDRAW, TRUE, 0);
}













