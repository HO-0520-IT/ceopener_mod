#include "presetdlg.h"

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
	IDC_NAME_EDIT = 101,
	IDC_TARGET_KEY_FN_CHECK = 112,
	IDC_TARGET_KEY_LABEL = 113,
	IDC_TARGET_KEY_BROWSE_BUTTON = 114,
	IDC_ACTION_VIRTUAL_KEY_RADIO = 131,
	IDC_ACTION_PROGRAM_RADIO = 133,
	IDC_ACTION_SPECIAL_RADIO = 134,
	IDC_ACTION_VIRTUAL_KEY_MODIFIERS_COMBO = 141,
	IDC_ACTION_VIRTUAL_KEY_COMBO = 143,
	IDC_ACTION_PROGRAM_PATH_EDIT = 151,
	IDC_ACTION_PROGRAM_PATH_BROWSE_BUTTON = 152,
	IDC_ACTION_SPECIAL_ACTION_COMBO = 161
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onActionVirtualKeyRadio(HWND hDlg);
static void onActionProgramRadio(HWND hDlg);
static void onActionSpecialRadio(HWND hDlg);
static void onTargetKeyBrowse(HWND hDlg);
static void onActionProgramPathBrowse(HWND hDlg);
static void updateControlStates(HWND hDlg);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;

struct PresetDialogData {
	PresetDialogParams *dialogParams;
	int targetKeyCode;
};

PresetDialogParams::PresetDialogParams() {
}

PresetDialogParams::~PresetDialogParams() {
}

bool showPresetDialog(HWND hOwnerWindow, PresetDialogParams &params) {
	int ret = DialogBoxParam(g_hInstance, _T("PRESET"), hOwnerWindow,
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
		case IDC_ACTION_VIRTUAL_KEY_RADIO:
			onActionVirtualKeyRadio(hDlg);
			break;
		case IDC_ACTION_PROGRAM_RADIO:
			onActionProgramRadio(hDlg);
			break;
		case IDC_ACTION_SPECIAL_RADIO:
			onActionSpecialRadio(hDlg);
			break;
		case IDC_TARGET_KEY_BROWSE_BUTTON:
			onTargetKeyBrowse(hDlg);
			break;
		case IDC_ACTION_PROGRAM_PATH_BROWSE_BUTTON:
			onActionProgramPathBrowse(hDlg);
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

	PresetDialogData *data = new PresetDialogData();
	SetWindowLong(hDlg, GWL_USERDATA, (long)data);

	data->dialogParams = (PresetDialogParams *)params;

	KnceUtil::adjustDialogLayout(hDlg);

	static TCHAR *modCStrs[] = {
		_T("(なし)"),
		_T("Alt"),
		_T("Ctrl"),
		_T("Ctrl + Alt"),
		_T("Shift"),
		_T("Shift + Alt"),
		_T("Ctrl + Shift"),
		_T("Ctrl + Shift + Alt")
	};

	HWND hVirtKeyModsCombo = GetDlgItem(hDlg,
        IDC_ACTION_VIRTUAL_KEY_MODIFIERS_COMBO);

    int numMods = sizeof(modCStrs) / sizeof(TCHAR *);
    for (i = 0; i < numMods; i++)
        SendMessage(hVirtKeyModsCombo, CB_ADDSTRING, 0, (LPARAM)modCStrs[i]);

	static TCHAR *virtKeyCStrs[] = {
		_T("01: LBUTTON"), _T("02: RBUTTON"),
		_T("03: CANCEL"),
		_T("04: MBUTTON"),
		_T("05: XBUTTON1"), _T("06: XBUTTON2"),
		_T("08: BACK"),
		_T("09: TAB"),
		_T("0C: CLEAR"),
		_T("0D: RETURN"),
		_T("10: SHIFT"), _T("11: CONTROL"),
		_T("12: MENU"),
		_T("13: PAUSE"),
		_T("14: CAPITAL"), _T("15: KANA"),
		_T("17: JUNJA"), _T("18: FINAL"), _T("19: KANJI"),
		_T("1B: ESCAPE"),
		_T("1C: CONVERT"), _T("1D: NONCONVERT"), _T("1E: ACCEPT"),
		_T("1F: MODECHANGE"),
		_T("20: SPACE"),
		_T("21: PRIOR"), _T("22: NEXT"), _T("23: END"), _T("24: HOME"),
		_T("25: LEFT"), _T("26: UP"), _T("27: RIGHT"), _T("28: DOWN"),
		_T("29: SELECT"), _T("2A: PRINT"), _T("2B: EXECUTE"),
		_T("2C: SNAPSHOT"),
		_T("2D: INSERT"), _T("2E: DELETE"),
		_T("2F: HELP"),
		_T("30: 0"), _T("31: 1"), _T("32: 2"), _T("33: 3"), _T("34: 4"),
		_T("35: 5"), _T("36: 6"), _T("37: 7"), _T("38: 8"), _T("39: 9"),
		_T("41: A"), _T("42: B"), _T("43: C"), _T("44: D"), _T("45: E"),
		_T("46: F"), _T("47: G"), _T("48: H"), _T("49: I"), _T("4A: J"),
		_T("4B: K"), _T("4C: L"), _T("4D: M"), _T("4E: N"), _T("4F: O"),
		_T("50: P"), _T("51: Q"), _T("52: R"), _T("53: S"), _T("54: T"),
		_T("55: U"), _T("56: V"), _T("57: W"), _T("58: X"), _T("59: Y"),
		_T("5A: Z"),
		_T("5B: LWIN"), _T("5C: RWIN"), _T("5D: APPS"),
		_T("5F: SLEEP"),
		_T("60: NUMPAD0"), _T("61: NUMPAD1"), _T("62: NUMPAD2"),
		_T("63: NUMPAD3"), _T("64: NUMPAD4"), _T("65: NUMPAD5"),
		_T("66: NUMPAD6"), _T("67: NUMPAD7"), _T("68: NUMPAD8"),
		_T("69: NUMPAD9"),
		_T("6A: MULTIPLY"), _T("6B: ADD"),
		_T("6C: SEPARATOR"),
		_T("6D: SUBTRACT"), _T("6E: DECIMAL"), _T("6F: DIVIDE"),
		_T("70: F1"), _T("71: F2"), _T("72: F3"), _T("73: F4"), _T("74: F5"),
		_T("75: F6"), _T("76: F7"), _T("77: F8"), _T("78: F9"), _T("79: F10"),
		_T("7A: F11"), _T("7B: F12"), _T("7C: F13"), _T("7D: F14"),
		_T("7E: F15"), _T("7F: F16"), _T("80: F17"), _T("81: F18"),
		_T("82: F19"), _T("83: F20"), _T("84: F21"), _T("85: F22"),
		_T("86: F23"), _T("87: F24"),
		_T("90: NUMLOCK"), _T("91: SCROLL"),
		_T("A0: LSHIFT"), _T("A1: RSHIFT"),
		_T("A2: LCONTROL"), _T("A3: RCONTROL"),
		_T("A4: LMENU"), _T("A5: RMENU"),
		_T("A6: BROWSER_BACK"), _T("A7: BROWSER_FORWARD"),
		_T("A8: BROWSER_REFRESH"), _T("A9: BROWSER_STOP"),
		_T("AA: BROWSER_SEARCH"), _T("AB: BROWSER_FAVORITES"),
		_T("AC: BROWSER_HOME"),
		_T("AD: VOLUME_MUTE"), _T("AE: VOLUME_DOWN"), _T("AF: VOLUME_UP"),
		_T("B0: MEDIA_NEXT_TRACK"), _T("B1: MEDIA_PREV_TRACK"),
		_T("B2: MEDIA_STOP"), _T("B3: MEDIA_PLAY_PAUSE"),
		_T("B4: LAUNCH_MAIL"), _T("B5: LAUNCH_MEDIA_SELECT"),
		_T("B6: LAUNCH_APP1"), _T("B7: LAUNCH_APP2"),
		_T("BA: OEM_1"),
		_T("BB: OEM_PLUS"), _T("BC: OEM_COMMA"), _T("BD: OEM_MINUS"),
		_T("BE: OEM_PERIOD"),
		_T("BF: OEM_2"), _T("C0: OEM_3"),
		_T("DB: OEM_4"), _T("DC: OEM_5"), _T("DD: OEM_6"), _T("DE: OEM_7"),
		_T("DF: OEM_8"),
		_T("E1: OEM_AX"), _T("E2: OEM_102"),
		_T("E3: ICO_HELP"), _T("E4: ICO_00"),
		_T("E5: PROCESSKEY"),
		_T("E6: ICO_CLEAR"),
		_T("E7: PACKET"),
		_T("E9: OEM_RESET"), _T("EA: OEM_JUMP"),
		_T("EB: OEM_PA1"), _T("EC: OEM_PA2"), _T("ED: OEM_PA3"),
		_T("EE: OEM_WSCTRL"),
		_T("EF: OEM_CUSEL"),
		_T("F0: OEM_ATTN"),
		_T("F1: OEM_FINISH"),
		_T("F2: OEM_COPY"), _T("F3: OEM_AUTO"), _T("F4: OEM_ENLW"),
		_T("F5: OEM_BACKTAB"),
		_T("F6: ATTN"),
		_T("F7: CRSEL"), _T("F8: EXSEL"),
		_T("F9: EREOF"),
		_T("FA: PLAY"),
		_T("FB: ZOOM"),
		_T("FC: NONAME"),
		_T("FD: PA1"),
		_T("FE: OEM_CLEAR")
	};

	HWND hVirtKeyCombo = GetDlgItem(hDlg, IDC_ACTION_VIRTUAL_KEY_COMBO);

	int numKeys = sizeof(virtKeyCStrs) / sizeof(TCHAR *);
	for (i = 0; i < numKeys; i++)
		SendMessage(hVirtKeyCombo, CB_ADDSTRING, 0, (LPARAM)virtKeyCStrs[i]);

	static TCHAR *specialActCStrs[] = {
		_T("01: 辞書アプリを前面に表示"),
		_T("11: 音量を下げる"),
		_T("12: 音量を上げる")
	};

	HWND hSpecialActCombo = GetDlgItem(hDlg, IDC_ACTION_SPECIAL_ACTION_COMBO);

	int numSpecialActs = sizeof(specialActCStrs) / sizeof(TCHAR *);
	for (i = 0; i < numSpecialActs; i++) {
		SendMessage(hSpecialActCombo, CB_ADDSTRING, 0,
			(LPARAM)specialActCStrs[i]);
	}

	BindItem *bindItem = data->dialogParams->getBindItem();

	HWND hNameEdit = GetDlgItem(hDlg, IDC_NAME_EDIT);
	SetWindowText(hNameEdit, bindItem->getName().c_str());

	HWND hKeyFnCheck = GetDlgItem(hDlg, IDC_TARGET_KEY_FN_CHECK);
	SendMessage(hKeyFnCheck, BM_SETCHECK, bindItem->getTargetKeyFn(), 0);

	data->targetKeyCode = bindItem->getTargetKeyCode();

	tstring keyName;
	KnceUtil::hardKeyCodeToName(keyName, data->targetKeyCode);

	HWND hKeyLabel = GetDlgItem(hDlg, IDC_TARGET_KEY_LABEL);
	SetWindowText(hKeyLabel, keyName.c_str());

	int actType = bindItem->getActionType();
	if (actType == BindItem::ACTION_VIRTUAL_KEY) {
		HWND hActionRadio = GetDlgItem(hDlg, IDC_ACTION_VIRTUAL_KEY_RADIO);
		SendMessage(hActionRadio, BM_SETCHECK, BST_CHECKED, 0);
	}
	else if (actType == BindItem::ACTION_PROGRAM) {
		HWND hActionRadio = GetDlgItem(hDlg, IDC_ACTION_PROGRAM_RADIO);
		SendMessage(hActionRadio, BM_SETCHECK, BST_CHECKED, 0);
	}
	else {
		HWND hActionRadio = GetDlgItem(hDlg, IDC_ACTION_SPECIAL_RADIO);
		SendMessage(hActionRadio, BM_SETCHECK, BST_CHECKED, 0);
	}

	SendMessage(hVirtKeyModsCombo, CB_SETCURSEL,
		bindItem->getVirtualKeyModifiers(), 0);

	TCHAR prefixCStr[16];
	_sntprintf(prefixCStr, 16, _T("%02x:"), bindItem->getVirtualKeyCode());
	int index = SendMessage(hVirtKeyCombo, CB_FINDSTRING, 0,
		(LPARAM)prefixCStr);

	SendMessage(hVirtKeyCombo, CB_SETCURSEL, index, 0);

	HWND hPathEdit = GetDlgItem(hDlg, IDC_ACTION_PROGRAM_PATH_EDIT);
	SetWindowText(hPathEdit, bindItem->getProgramFileName().c_str());

	_sntprintf(prefixCStr, 16, _T("%02x:"), bindItem->getSpecialAction());
	index = SendMessage(hSpecialActCombo, CB_FINDSTRING, 0,
		(LPARAM)prefixCStr);

	SendMessage(hSpecialActCombo, CB_SETCURSEL, index, 0);

	updateControlStates(hDlg);

	ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
	PresetDialogData *data = (PresetDialogData *)GetWindowLong(hDlg,
		GWL_USERDATA);

	BindItem *bindItem = data->dialogParams->getBindItem();

	HWND hNameEdit = GetDlgItem(hDlg, IDC_NAME_EDIT);

	TCHAR nameCStr[256];
	GetWindowText(hNameEdit, nameCStr, 256);

	bindItem->setName(nameCStr);

	HWND hKeyFnCheck = GetDlgItem(hDlg, IDC_TARGET_KEY_FN_CHECK);
	bindItem->setTargetKeyFn(SendMessage(hKeyFnCheck, BM_GETCHECK, 0, 0) != 0);

	bindItem->setTargetKeyCode(data->targetKeyCode);

	HWND hActionVirtRadio = GetDlgItem(hDlg, IDC_ACTION_VIRTUAL_KEY_RADIO);
	HWND hActionProgRadio = GetDlgItem(hDlg, IDC_ACTION_PROGRAM_RADIO);

	if (SendMessage(hActionVirtRadio, BM_GETCHECK, BST_CHECKED, 0) != 0)
		bindItem->setActionType(BindItem::ACTION_VIRTUAL_KEY);
	else if (SendMessage(hActionProgRadio, BM_GETCHECK, BST_CHECKED, 0) != 0)
		bindItem->setActionType(BindItem::ACTION_PROGRAM);
	else
		bindItem->setActionType(BindItem::ACTION_SPECIAL);

	HWND hVirtKeyModsCombo = GetDlgItem(hDlg,
		IDC_ACTION_VIRTUAL_KEY_MODIFIERS_COMBO);
	bindItem->setVirtualKeyModifiers(SendMessage(hVirtKeyModsCombo,
		CB_GETCURSEL, 0, 0));

	HWND hVirtKeyCombo = GetDlgItem(hDlg, IDC_ACTION_VIRTUAL_KEY_COMBO);

	TCHAR virtKeyCStr[256];
	GetWindowText(hVirtKeyCombo, virtKeyCStr, 256);
	virtKeyCStr[3] = _T('\0');

	int virtKeyCode = 0;
	_stscanf(virtKeyCStr, _T("%02x:"), &virtKeyCode);

	bindItem->setVirtualKeyCode(virtKeyCode);

	HWND hPathEdit = GetDlgItem(hDlg, IDC_ACTION_PROGRAM_PATH_EDIT);

	TCHAR pathCStr[MAX_PATH];
	GetWindowText(hPathEdit, pathCStr, MAX_PATH);

	bindItem->setProgramFileName(pathCStr);

	HWND hSpecialActCombo = GetDlgItem(hDlg, IDC_ACTION_SPECIAL_ACTION_COMBO);

	TCHAR specialActCStr[256];
	GetWindowText(hSpecialActCombo, specialActCStr, 256);
	specialActCStr[3] = _T('\0');

	int specialAct = 0;
	_stscanf(specialActCStr, _T("%02x:"), &specialAct);

	bindItem->setSpecialAction(specialAct);

	EndDialog(hDlg, IDOK);

	delete data;
}

static void onCancel(HWND hDlg) {
	PresetDialogData *data = (PresetDialogData *)GetWindowLong(hDlg,
		GWL_USERDATA);

	EndDialog(hDlg, IDCANCEL);

	delete data;
}

static void onActionVirtualKeyRadio(HWND hDlg) {
	updateControlStates(hDlg);
}

static void onActionProgramRadio(HWND hDlg) {
	updateControlStates(hDlg);
}

static void onActionSpecialRadio(HWND hDlg) {
	updateControlStates(hDlg);
}

static void onTargetKeyBrowse(HWND hDlg) {
	PresetDialogData *data = (PresetDialogData *)GetWindowLong(hDlg,
		GWL_USERDATA);

	int keyCode = 0;
	if (knceCaptureKey(hDlg, &keyCode)) {
		data->targetKeyCode = keyCode;

		tstring keyName;
		KnceUtil::hardKeyCodeToName(keyName, keyCode);

		HWND hKeyLabel = GetDlgItem(hDlg, IDC_TARGET_KEY_LABEL);
		SetWindowText(hKeyLabel, keyName.c_str());
	}
}

static void onActionProgramPathBrowse(HWND hDlg) {
	PresetDialogData *data = (PresetDialogData *)GetWindowLong(hDlg,
		GWL_USERDATA);

	KnceChooseApplicationParams params = {0};
	if (!knceChooseApplication(hDlg, &params))
		return;

	HWND hPathEdit = GetDlgItem(hDlg, IDC_ACTION_PROGRAM_PATH_EDIT);
	SetWindowText(hPathEdit, params.fileName);
}

static void updateControlStates(HWND hDlg) {
	PresetDialogData *data = (PresetDialogData *)GetWindowLong(hDlg,
		GWL_USERDATA);

	HWND hActionVirtRadio = GetDlgItem(hDlg, IDC_ACTION_VIRTUAL_KEY_RADIO);
	HWND hVirtKeyModsCombo = GetDlgItem(hDlg,
		IDC_ACTION_VIRTUAL_KEY_MODIFIERS_COMBO);
	HWND hVirtKeyCombo = GetDlgItem(hDlg, IDC_ACTION_VIRTUAL_KEY_COMBO);

	if (SendMessage(hActionVirtRadio, BM_GETCHECK, BST_CHECKED, 0) == 0) {
		EnableWindow(hVirtKeyModsCombo, false);
		EnableWindow(hVirtKeyCombo, false);
	}
	else {
		EnableWindow(hVirtKeyModsCombo, true);
		EnableWindow(hVirtKeyCombo, true);
	}

	HWND hActionProgRadio = GetDlgItem(hDlg, IDC_ACTION_PROGRAM_RADIO);
	HWND hPathEdit = GetDlgItem(hDlg, IDC_ACTION_PROGRAM_PATH_EDIT);
	HWND hPathBrowseButton =
		GetDlgItem(hDlg, IDC_ACTION_PROGRAM_PATH_BROWSE_BUTTON);

	if (SendMessage(hActionProgRadio, BM_GETCHECK, BST_CHECKED, 0) == 0) {
		EnableWindow(hPathEdit, false);
		EnableWindow(hPathBrowseButton, false);
	}
	else {
		EnableWindow(hPathEdit, true);
		EnableWindow(hPathBrowseButton, true);
	}

	HWND hActionSpecialRadio = GetDlgItem(hDlg, IDC_ACTION_SPECIAL_RADIO);
	HWND hSpecialActCombo = GetDlgItem(hDlg, IDC_ACTION_SPECIAL_ACTION_COMBO);

	if (SendMessage(hActionSpecialRadio, BM_GETCHECK, BST_CHECKED, 0) == 0)
		EnableWindow(hSpecialActCombo, false);
	else
		EnableWindow(hSpecialActCombo, true);
}
