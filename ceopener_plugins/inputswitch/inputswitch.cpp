#include <string>
#include <windows.h>
#include <keybd.h>
#include <knceutil.h>
#include <brainapi.h>
#include <ceopener_plugin.h>
#include "settingdlg.h"
#include "PadInput.h"
#include "inputwindow.h"
#include "CandidateController.h"
#include "EditController.h"
#include "CandidateCache.h"
#include "Dictionary.h"

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

#ifdef INPUTSWITCH_EXPORTS
#define INPUTSWITCH_API __declspec(dllexport)
#else
#define INPUTSWITCH_API __declspec(dllimport)
#endif

enum {
	INPUT_MODE_GENERIC = 0,
	INPUT_MODE_NUMBER = 1,
	INPUT_MODE_SYMBOL = 2,
	MODE_CHANGING_DEFAULT = 0,
	MODE_CHANGING_KEY_BINDER_ONLY = 1,
	SYMBOL_INPUT_SUB_TOUCH = 0,
	SYMBOL_INPUT_INPUT_WINDOW = 1,
	KEY_BINDING_MODE_CHANGE = 0x1000,		//切り替え画面を出すキー
	KEY_BINDING_MODE_ALPHA_HALF = 0x1000 + 1,
	KEY_BINDING_MODE_NUMBER_HALF = 0x1000 + 2,
	KEY_BINDING_MODE_SYMBOL_HALF = 0x1000 + 3,
	KEY_BINDING_MODE_HIRA = 0x1000 + 4,
	KEY_BINDING_MODE_NUMBER_FULL = 0x1000 + 5,
	KEY_BINDING_MODE_SYMBOL_FULL = 0x1000 + 6,
	KEY_BINDING_INPUT_COMMA = 0x1000 + 11,
	KEY_BINDING_INPUT_PERIOD = 0x1000 + 12,
	SYMBOL_HALF = 1,
	SYMBOL_FULL = 2,
	TIMER_POWERON = 100
};

static void onMenuItemSelected(HWND hCont, HWND hMenu, int id);
static void onTrayIconItemClicked(HWND hCont, int id);
static int onBindedKeyPress(HWND hCont, int id);
static void onSubTouchActive(HWND hCont, int id);
static void onSubTouchInactive(HWND hCont, int id);
static void onSubTouchDicKeyDown(HWND hCont, int id, int dicKey);
static void onEdClosedOpened(HWND hCont, unsigned long params);
static void onEdPowerOnOff(HWND hCont, unsigned long params);
static void onTaskSwitched(HWND hCont, unsigned long params);
static void onTimer(HWND hCont, int id);
static void setupDictionary(HWND hCont);
static void setupKeyBindings(HWND hCont);
static void removeKeyBindings(HWND hCont);
static void showSettings(HWND hCont);
static void startInputMode(HWND hCont);
static void endInputMode(HWND hCont);
static void switchInputMode();
static void handleBindedKey(HWND hCont, int id);
static void startInputGeneric(HWND hCont);
static void endInputGeneric(HWND hCont);
static void handleHotKeyGeneric(HWND hCont, int id);
static void startInputNumber(HWND hCont);
static void endInputNumber(HWND hCont);
static void handleHotKeyNumber(int id);
static void startInputSymbol(HWND hCont);
static void endInputSymbol(HWND hCont);
static void handleHotKeySymbol(HWND hCont, int id);
static void processPadInputSymbol(HWND hCont, int id);
static void sendTextToTarget(const tstring &text);
static void beginEditing(HWND hCont, TCHAR ch);
static void cancelEditing(HWND hCont);
static void commitEditing(HWND hCont);
static void convertKanji();
static void activateArrowBindings(HWND hCont);
static void inactivateArrowBindings(HWND hCont);
static int registerKeyBinding(HWND hCont, bool fn, unsigned int keyCode,
	bool active = false);
static int registerSpecialKeyBinding(HWND hCont, const tstring &name, bool fn,
	unsigned int keyCode, bool active = false);
static void saveCurrentSettings(HWND hCont);

HINSTANCE g_hInstance = NULL;
HFONT g_hMainFont = NULL;

static int g_trayId = 0;
static int g_pluginMenuItemId = 0;
static int g_subTouchId = 0;
static PadInput g_padInput;
static HWND g_hInputWindow = NULL;
static int g_modeChangingMethod = MODE_CHANGING_DEFAULT;
static int g_symbolInputDevice = SYMBOL_INPUT_SUB_TOUCH;
static bool g_isSubTouchUseEnabled = true;
static CandidateController g_candidateController;
static EditController g_editController;
static CandidateCache g_candidateCache;
static Dictionary g_dictionary;
static vector<tstring> g_halfSymbolCandidateList;
static vector<tstring> g_fullSymbolCandidateList;
static vector<tstring> g_InputModeList;
static vector<tstring> g_currentCandidates;
static bool g_isInputKana = false;
static int g_inputMode = INPUT_MODE_GENERIC;
static int g_prevInputMode = INPUT_MODE_GENERIC;
static int g_symbolMode = SYMBOL_HALF;
static vector<HICON> g_hInputModeIcons;
static map<int, int> g_bindedIdTable;
static vector<int> g_bindedAlphaIds;
static vector<int> g_bindedAlphaFnIds;
static vector<int> g_bindedNumberIds;
static bool g_isEditing = false;
static bool g_isLargePad = false;
static bool g_isDisableAtEd = false;
static bool g_isSwapHiphen = false;

static unsigned int g_dicKeyDownMessage =
	RegisterWindowMessage(_T("DicKeyDown"));

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH)
		g_hInstance = (HINSTANCE)hInst;

	return true;
}

extern "C" INPUTSWITCH_API int pluginInit(HWND hCont) {
	int i;

	if (!loadBrainApis())
		return false;

	registerInputWindowClass();

	g_hMainFont = (HFONT)GetStockObject(SYSTEM_FONT);

	PluginTrayIconItem trayItem = {0};
	trayItem.onClicked = onTrayIconItemClicked;

	g_trayId = pluginRegisterTrayIconItem(hCont, g_hInstance, &trayItem);

	g_hInputModeIcons.push_back((HICON)LoadImage(g_hInstance,
		_T("ALPHA_HALF"), IMAGE_ICON, 0, 0, 0));
	g_hInputModeIcons.push_back((HICON)LoadImage(g_hInstance,
		_T("NUMBER_HALF"), IMAGE_ICON, 0, 0, 0));
	g_hInputModeIcons.push_back((HICON)LoadImage(g_hInstance,
		_T("SYMBOL_HALF"), IMAGE_ICON, 0, 0, 0));
	g_hInputModeIcons.push_back((HICON)LoadImage(g_hInstance,
		_T("HIRA"), IMAGE_ICON, 0, 0, 0));
	g_hInputModeIcons.push_back((HICON)LoadImage(g_hInstance,
		_T("NUMBER_FULL"), IMAGE_ICON, 0, 0, 0));
	g_hInputModeIcons.push_back((HICON)LoadImage(g_hInstance,
		_T("SYMBOL_FULL"), IMAGE_ICON, 0, 0, 0));
	g_hInputModeIcons.push_back((HICON)LoadImage(g_hInstance,
		_T("NOMODE"), IMAGE_ICON, 0, 0, 0));		//SHARP SIMの時に選択される入力モード

	pluginSetTrayIcon(hCont, g_trayId, g_hInputModeIcons[0]);

	HWND hPluginMenu = pluginGetPluginMenu(hCont);

	PluginMenuItem menuItem = {0};
	_tcscpy(menuItem.caption, _T("InputSwitchの設定..."));
	menuItem.onItemSelected = onMenuItemSelected;

	g_pluginMenuItemId = pluginAppendMenuItem(hCont, hPluginMenu, &menuItem);

	InputWindowParams inputWindowParams;
	inputWindowParams.setCandidateController(&g_candidateController);
	inputWindowParams.setEditController(&g_editController);

	g_hInputWindow = createInputWindow(inputWindowParams);

	tstring persistDir = _T("\\Nand2\\.ceopener");
	tstring propFileName = persistDir + _T("\\inputswitch.dat");

	int fontSize = 0;
	if (GetFileAttributes(propFileName.c_str()) == -1) {
		fontSize = 100;
		g_isSubTouchUseEnabled = true;
		g_isDisableAtEd = true;
		g_isSwapHiphen = true;
	}
	else {
		map<tstring, tstring> props;
		KnceUtil::readPropertyFile(props, propFileName);

		fontSize = _ttoi(props[_T("fontSize")].c_str());
		g_isSubTouchUseEnabled =
			_ttoi(props[_T("subTouchUseEnabled")].c_str()) != 0;
		g_isLargePad =
			_ttoi(props[_T("subTouchLargePad")].c_str()) != 0;
		g_isDisableAtEd =
			_ttoi(props[_T("disableAtEdApp")].c_str()) != 0;
		g_isSwapHiphen =
			_ttoi(props[_T("SwapHiphen")].c_str()) != 0;
	}

	setInputWindowFontSize(g_hInputWindow, fontSize);

	static TCHAR halfSyms[] = {
		_T('!'), _T('"'), _T('#'), _T('$'), _T('%'), _T('&'),
		_T('\''), _T('('), _T(')'), _T('*'), _T('+'), _T(','),
		_T('-'), _T('.'), _T('/'), _T(':'), _T(';'), _T('<'),
		_T('='), _T('>'), _T('?'), _T('@'), _T('['), _T('\\'),
		_T(']'), _T('^'), _T('_'), _T('`'), _T('{'), _T('|'),
		_T('}'), _T('~')
	};

	static TCHAR fullSyms[] = {
		0xff01, 0x201d, 0xff03, 0xff04, 0xff05, 0xff06,
		0x2019, 0xff08, 0xff09, 0xff0a, 0xff0b, 0x3001,
		0x30fc, 0x3002, 0x30fb, 0xff1a, 0xff1b, 0xff1c,
		0xff1d, 0xff1e, 0xff1f, 0xff20, 0x300c, 0xffe5,
		0x300d, 0xff3e, 0xff3f, 0x2018, 0xff5b, 0xff5c,
		0xff5d, 0xff5e
	};

	int numSyms = sizeof(halfSyms) / sizeof(TCHAR);
	for (i = 0; i < numSyms; i++)
		g_halfSymbolCandidateList.push_back(tstring(1, halfSyms[i]));

	numSyms = sizeof(fullSyms) / sizeof(TCHAR);
	for (i = 0; i < numSyms; i++)
		g_fullSymbolCandidateList.push_back(tstring(1, fullSyms[i]));

	setupDictionary(hCont);

	setupKeyBindings(hCont);

	startInputMode(hCont);

	PluginSubTouch touch = {0};
	touch.priority = PLUGIN_SUB_TOUCH_PRIORITY_LOWEST;
	touch.isEnabled = false;
	touch.onActive = onSubTouchActive;
	touch.onInactive = onSubTouchInactive;
	touch.onDicKeyDown = onSubTouchDicKeyDown;

	g_subTouchId = pluginRegisterSubTouch(hCont, &touch);
	g_padInput.init(hCont, g_isLargePad);

	if (g_isSubTouchUseEnabled)
		pluginEnableSubTouch(hCont, g_subTouchId);

	pluginRegisterCallback(hCont, PLUGIN_CALLBACK_ED_CLOSED_OPENED,
		onEdClosedOpened);
	pluginRegisterCallback(hCont, PLUGIN_CALLBACK_TASK_SWITCHED,
		onTaskSwitched);
	pluginRegisterCallback(hCont, PLUGIN_CALLBACK_POWER_ON_OFF,
		onEdPowerOnOff);

	return true;
}

extern "C" INPUTSWITCH_API void pluginTerminate(HWND hCont) {
	int i;

	g_padInput.terminate();
	pluginUnregisterSubTouch(hCont, g_subTouchId);

	removeKeyBindings(hCont);

	DestroyWindow(g_hInputWindow);

	UnregisterClass(_T("InputSwitchNoticeWindow"), g_hInstance);

	pluginUnregisterTrayIconItem(hCont, g_trayId);

	int numModeIcons = g_hInputModeIcons.size();
	for (i = 0; i < numModeIcons; i++)
		DestroyIcon(g_hInputModeIcons[i]);

	HWND hPluginMenu = pluginGetPluginMenu(hCont);
	pluginRemoveMenuItem(hCont, hPluginMenu, g_pluginMenuItemId);

	DeleteObject(g_hMainFont);

	unregisterInputWindowClass();

	freeBrainApis();
}

static void onMenuItemSelected(HWND hCont, HWND hMenu, int id) {
	showSettings(hCont);
}

static void onTrayIconItemClicked(HWND hCont, int id) {
	showSettings(hCont);
}

static int onBindedKeyPress(HWND hCont, int id) {
	handleBindedKey(hCont, id);

	return true;
}

static void onSubTouchActive(HWND hCont, int id) {
	if (!g_padInput.isEnabled())
		g_padInput.enable();
}

static void onSubTouchInactive(HWND hCont, int id) {
	if (g_padInput.isEnabled())
		g_padInput.disable();
}

static void onSubTouchDicKeyDown(HWND hCont, int id, int dicKey) {
	int buttonId = g_padInput.processButtonDown(dicKey);
	if (buttonId == 0)
		return;

	if (buttonId == 0x8fff)   // for screen shot
		SSHOT_WriteSubLcdBMP2File_win();
	else if (buttonId == PadInput::BUTTON_ALPHA_HALF ||
		buttonId == PadInput::BUTTON_NUMBER_HALF ||
		buttonId == PadInput::BUTTON_SYMBOL_HALF ||
		buttonId == PadInput::BUTTON_HIRA ||
		buttonId == PadInput::BUTTON_NUMBER_FULL ||
		buttonId == PadInput::BUTTON_SYMBOL_FULL) {

		if(!(g_isLargePad && buttonId == PadInput::BUTTON_SYMBOL_HALF && buttonId == PadInput::BUTTON_SYMBOL_FULL))
			endInputMode(hCont);

		g_prevInputMode = g_inputMode;

		switch (buttonId) {
		case PadInput::BUTTON_ALPHA_HALF:
			g_isInputKana = false;
			g_inputMode = INPUT_MODE_GENERIC;
			break;
		case PadInput::BUTTON_NUMBER_HALF:
			g_isInputKana = false;
			g_inputMode = INPUT_MODE_NUMBER;
			break;
		case PadInput::BUTTON_SYMBOL_HALF:
			g_symbolMode = SYMBOL_HALF;
			if(g_isLargePad) {
				g_padInput.switchPad(PadInput::PAD_HALF_SYMBOL);
			}
			else {
				g_inputMode = INPUT_MODE_SYMBOL;
				g_isInputKana = false;
			}
			break;
		case PadInput::BUTTON_HIRA:
			g_isInputKana = true;
			g_inputMode = INPUT_MODE_GENERIC;
			break;
		case PadInput::BUTTON_NUMBER_FULL:
			g_isInputKana = true;
			g_inputMode = INPUT_MODE_NUMBER;
			break;
		case PadInput::BUTTON_SYMBOL_FULL:
			g_symbolMode = SYMBOL_FULL;
			if(g_isLargePad) {
				g_padInput.switchPad(PadInput::PAD_FULL_SYMBOL);
			}
			else {
				g_inputMode = INPUT_MODE_SYMBOL;
				g_isInputKana = true;
			}

			break;
		}

		if(!(g_isLargePad && buttonId == PadInput::BUTTON_SYMBOL_HALF && buttonId == PadInput::BUTTON_SYMBOL_FULL))
			startInputMode(hCont);
	}
	else {
		if (g_isLargePad || g_inputMode == INPUT_MODE_SYMBOL)
			processPadInputSymbol(hCont, buttonId);
	}
}

static void onEdClosedOpened(HWND hCont, unsigned long params) {
	if (params == PLUGIN_ED_AFTER_OPENED)
		setupDictionary(hCont);
}

static void onEdPowerOnOff(HWND hCont, unsigned long params) {
	if(params != 1) {
		SetTimer(hCont, TIMER_POWERON, 500, NULL);
	}
}

static void onTaskSwitched(HWND hCont, unsigned long params) {
	PluginCallbackTaskSwitchedParams *data =
		(PluginCallbackTaskSwitchedParams*)params;

	if(g_isDisableAtEd) {
		HWND hEdWnd = FindWindow(_T("SHARP SIM"), NULL);

		if(data->hWindowTo == hEdWnd)
			endInputMode(hCont);
		else if(data->hWindowFrom == hEdWnd)
			startInputMode(hCont);
	}
}

static void onTimer(HWND hCont, int id) {
	switch(id) {
		case TIMER_POWERON:
			KillTimer(hCont, TIMER_POWERON);
			if(g_isLargePad) {
				g_padInput.switchPad(g_symbolMode == SYMBOL_FULL ? PadInput::PAD_FULL_SYMBOL : PadInput::PAD_HALF_SYMBOL);
			}
			break;
	}
}

static void setupDictionary(HWND hCont) {
	tstring pluginDir = KnceUtil::getCurrentDirectory() + _T("\\plugins");
	tstring dicFileName = pluginDir + _T("\\inputswitch.dict");

	if (GetFileAttributes(dicFileName.c_str()) != -1)
		g_dictionary.setDictionaryFile(dicFileName);
	else {
		MessageBox(hCont, _T("辞書ファイルを開くことができませんでした。"), _T("エラー"),
			MB_ICONEXCLAMATION);
	}
}

static void setupKeyBindings(HWND hCont) {
	int i;

	g_bindedIdTable[HARD_KEY_HYPHEN] = registerKeyBinding(hCont,
		false, HARD_KEY_HYPHEN);
	g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000] = registerKeyBinding(hCont,
		true, HARD_KEY_HYPHEN);
	g_bindedIdTable[0x20] = registerKeyBinding(hCont,
		false, 0x20);		//スペースキーの割り当て
	g_bindedIdTable[0x20 | 0x8000] = registerKeyBinding(hCont,
		true, 0x20);		//Shift+スペースキーの割り当て

	g_bindedIdTable[HARD_KEY_BACK] = registerKeyBinding(hCont,
		false, HARD_KEY_BACK);
	g_bindedIdTable[HARD_KEY_BACKSPACE] = registerKeyBinding(hCont,
		false, HARD_KEY_BACKSPACE);
	g_bindedIdTable[HARD_KEY_EXECUTE] = registerKeyBinding(hCont,
		false, HARD_KEY_EXECUTE);
	g_bindedIdTable[HARD_KEY_EXECUTE | 0x8000] = registerKeyBinding(hCont,
		true, HARD_KEY_EXECUTE);

	g_bindedIdTable[HARD_KEY_LEFT] = registerKeyBinding(hCont,
		false, HARD_KEY_LEFT);
	g_bindedIdTable[HARD_KEY_RIGHT] = registerKeyBinding(hCont,
		false, HARD_KEY_RIGHT);
	g_bindedIdTable[HARD_KEY_UP] = registerKeyBinding(hCont,
		false, HARD_KEY_UP);
	g_bindedIdTable[HARD_KEY_DOWN] = registerKeyBinding(hCont,
		false, HARD_KEY_DOWN);

	for (i = 0; i < 26; i++) {
		int keyCode = _T('A') + i;
		int bindedId = registerKeyBinding(hCont, false, keyCode);
		g_bindedAlphaIds.push_back(bindedId);

		bindedId = registerKeyBinding(hCont, true, keyCode);
		g_bindedAlphaFnIds.push_back(bindedId);
	}

	static int numKeyCodes[] = {'P', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
		'O'};

	for (i = 0; i < 10; i++) {
		int keyCode = numKeyCodes[i];
		int bindedId = registerKeyBinding(hCont, false, keyCode);
		g_bindedNumberIds.push_back(bindedId);
	}

	g_bindedIdTable[KEY_BINDING_MODE_CHANGE] = registerSpecialKeyBinding(
		hCont, _T("入力文字切替"), false, 0, true);

	g_bindedIdTable[KEY_BINDING_MODE_ALPHA_HALF] = registerSpecialKeyBinding(
		hCont, _T("半角英字"), false, 0, true);
	g_bindedIdTable[KEY_BINDING_MODE_NUMBER_HALF] = registerSpecialKeyBinding(
		hCont, _T("半角数字"), false, 0, true);
	g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_HALF] = registerSpecialKeyBinding(
		hCont, _T("半角記号"), false, 0, true);
	g_bindedIdTable[KEY_BINDING_MODE_HIRA] = registerSpecialKeyBinding(
		hCont, _T("ひらがな"), false, 0, true);
	g_bindedIdTable[KEY_BINDING_MODE_NUMBER_FULL] = registerSpecialKeyBinding(
		hCont, _T("全角数字"), false, 0, true);
	g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_FULL] = registerSpecialKeyBinding(
		hCont, _T("全角記号"), false, 0, true);

	g_bindedIdTable[KEY_BINDING_INPUT_COMMA] = registerSpecialKeyBinding(
		hCont, _T("カンマの入力"), false, 0, true);
	g_bindedIdTable[KEY_BINDING_INPUT_PERIOD] = registerSpecialKeyBinding(
		hCont, _T("ピリオドの入力"), false, 0, true);
}

static void removeKeyBindings(HWND hCont) {
	int i;

	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[HARD_KEY_HYPHEN]);
	pluginUnregisterKeyBinding(hCont,
		g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000]);
	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[0x20]);		//スペースキーの割り当て解除
	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[0x20 | 0x8000]);

	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACK]);
	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACKSPACE]);
	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[HARD_KEY_EXECUTE]);
	pluginUnregisterKeyBinding(hCont,
		g_bindedIdTable[HARD_KEY_EXECUTE | 0x8000]);
	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[HARD_KEY_LEFT]);
	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[HARD_KEY_RIGHT]);
	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[HARD_KEY_UP]);
	pluginUnregisterKeyBinding(hCont, g_bindedIdTable[HARD_KEY_DOWN]);

	for (i = 0; i < 26; i++) {
		pluginUnregisterKeyBinding(hCont, g_bindedAlphaIds[i]);
		pluginUnregisterKeyBinding(hCont, g_bindedAlphaFnIds[i]);
	}

	for (i = 0; i < 10; i++)
		pluginUnregisterKeyBinding(hCont, g_bindedNumberIds[i]);

	pluginUnregisterKeyBinding(hCont, KEY_BINDING_MODE_CHANGE);

	pluginUnregisterKeyBinding(hCont, KEY_BINDING_MODE_ALPHA_HALF);
	pluginUnregisterKeyBinding(hCont, KEY_BINDING_MODE_NUMBER_HALF);
	pluginUnregisterKeyBinding(hCont, KEY_BINDING_MODE_SYMBOL_HALF);
	pluginUnregisterKeyBinding(hCont, KEY_BINDING_MODE_HIRA);
	pluginUnregisterKeyBinding(hCont, KEY_BINDING_MODE_NUMBER_FULL);
	pluginUnregisterKeyBinding(hCont, KEY_BINDING_MODE_SYMBOL_FULL);

	pluginUnregisterKeyBinding(hCont, KEY_BINDING_INPUT_COMMA);
	pluginUnregisterKeyBinding(hCont, KEY_BINDING_INPUT_PERIOD);
}

static void showSettings(HWND hCont) {
	SettingParams params;
	params.setFontSize(getInputWindowFontSize(g_hInputWindow));
	params.setSubTouchUseEnabled(g_isSubTouchUseEnabled);
	params.setLargePad(g_isLargePad);
	params.setDisableAtEd(g_isDisableAtEd);
	params.setSwapHiphen(g_isSwapHiphen);

	if (!showSettingDialog(hCont, params))
		return;

	setInputWindowFontSize(g_hInputWindow, params.getFontSize());
	g_isSubTouchUseEnabled = params.isSubTouchUseEnabled();
	g_isLargePad = params.isLargePad();
	g_isDisableAtEd = params.isDisableAtEd();
	g_isSwapHiphen = params.isSwapHiphen();
	g_padInput.setIsLargePad(g_isLargePad);

	if (g_isSubTouchUseEnabled)
		pluginEnableSubTouch(hCont, g_subTouchId);
	else
		pluginDisableSubTouch(hCont, g_subTouchId);

	endInputMode(hCont);

	g_isInputKana = false;
	g_inputMode = INPUT_MODE_GENERIC;
	g_symbolMode = SYMBOL_HALF;
	
	saveCurrentSettings(hCont);
	startInputMode(hCont);
}

static void startInputMode(HWND hCont) {
	pluginSetTrayIcon(hCont, g_trayId, g_hInputModeIcons[g_inputMode +
		(g_isInputKana ? 3 : 0)]);

	pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_HYPHEN]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[0x20]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[0x20 | 0x8000]);
	
	//入力切替用のキーとかを有効化する関数たち
	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_CHANGE]);

	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_ALPHA_HALF]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_NUMBER_HALF]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_HALF]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_HIRA]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_NUMBER_FULL]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_FULL]);

	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_INPUT_COMMA]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_INPUT_PERIOD]);
	//以上

	switch (g_inputMode) {
	case INPUT_MODE_GENERIC:
		startInputGeneric(hCont);
		break;
	case INPUT_MODE_NUMBER:
		startInputNumber(hCont);
		break;
	case INPUT_MODE_SYMBOL:
		startInputSymbol(hCont);
		break;
	}
}

static void endInputMode(HWND hCont) {
	pluginSetTrayIcon(hCont, g_trayId, g_hInputModeIcons[6]);

	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_HYPHEN]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[0x20]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[0x20 | 0x8000]);

	//入力切替用のキーとかを無効化する関数たち
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_CHANGE]);

	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_ALPHA_HALF]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_NUMBER_HALF]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_HALF]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_HIRA]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_NUMBER_FULL]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_FULL]);

	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_INPUT_COMMA]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[KEY_BINDING_INPUT_PERIOD]);
	//以上

	switch (g_inputMode) {
	case INPUT_MODE_GENERIC:
		endInputGeneric(hCont);
		break;
	case INPUT_MODE_NUMBER:
		endInputNumber(hCont);
		break;
	case INPUT_MODE_SYMBOL:
		endInputSymbol(hCont);
		break;
	}
}
static void switchInputMode(){
	if (g_prevInputMode == INPUT_MODE_GENERIC) {
		g_inputMode = INPUT_MODE_NUMBER;
	}
	else if (g_prevInputMode == INPUT_MODE_NUMBER) {
		if (g_isInputKana == false){
			if(g_isLargePad) {
				g_symbolMode = SYMBOL_HALF;
				g_padInput.switchPad(PadInput::PAD_HALF_SYMBOL);
			}
			else {
				g_isInputKana = false;
				g_inputMode = INPUT_MODE_SYMBOL;
			}
		} else {
			if(g_isLargePad) {
				g_symbolMode = SYMBOL_FULL;
				g_padInput.switchPad(PadInput::PAD_FULL_SYMBOL);
			}
			else {
				g_isInputKana = true;
				g_inputMode = INPUT_MODE_SYMBOL;
			}
		}
	}
	else if (g_prevInputMode == INPUT_MODE_SYMBOL) {
		g_isInputKana = !g_isInputKana;
		g_inputMode = INPUT_MODE_GENERIC;
	}
}
static void handleBindedKey(HWND hCont, int id) {
	HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);

	if (id >= g_bindedIdTable[KEY_BINDING_MODE_CHANGE] &&
		id <= g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_FULL]) {

		if(!(g_isLargePad && id == g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_HALF] 
		&& id == g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_FULL]))
			endInputMode(hCont);

		g_prevInputMode = g_inputMode;

		if (id == g_bindedIdTable[KEY_BINDING_MODE_CHANGE]) {
			switchInputMode();
		}
		else if (id == g_bindedIdTable[KEY_BINDING_MODE_ALPHA_HALF]) {
			g_isInputKana = false;
			g_inputMode = INPUT_MODE_GENERIC;
		}
		else if (id == g_bindedIdTable[KEY_BINDING_MODE_NUMBER_HALF]) {
			g_isInputKana = false;
			g_inputMode = INPUT_MODE_NUMBER;
		}
		else if (id == g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_HALF]) {
			if(g_isLargePad) {
				g_symbolMode = SYMBOL_HALF;
				g_padInput.switchPad(PadInput::PAD_HALF_SYMBOL);
			}
			else {
				g_isInputKana = false;
				g_inputMode = INPUT_MODE_SYMBOL;
			}
		}
		else if (id == g_bindedIdTable[KEY_BINDING_MODE_HIRA]) {
			g_isInputKana = true;
			g_inputMode = INPUT_MODE_GENERIC;
		}
		else if (id == g_bindedIdTable[KEY_BINDING_MODE_NUMBER_FULL]) {
			g_isInputKana = true;
			g_inputMode = INPUT_MODE_NUMBER;
		}
		else if (id == g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_FULL]) {
			if(g_isLargePad) {
				g_symbolMode = SYMBOL_FULL;
				g_padInput.switchPad(PadInput::PAD_FULL_SYMBOL);
			}
			else {
				g_isInputKana = true;
				g_inputMode = INPUT_MODE_SYMBOL;
			}
		}

		if(!(g_isLargePad && id == g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_HALF] 
		&& id == g_bindedIdTable[KEY_BINDING_MODE_SYMBOL_FULL]))
			startInputMode(hCont);
	}
	else if (id == g_bindedIdTable[KEY_BINDING_INPUT_COMMA]) {
		if (g_isInputKana) {
			if(g_isEditing) {
				g_editController.append(_T('、'));
				InvalidateRect(hCont, NULL, true);
			}
			else
				sendTextToTarget(tstring(1, 0x3001));   // "、"
		}
		else
			sendTextToTarget(_T(","));
	}
	else if (id == g_bindedIdTable[KEY_BINDING_INPUT_PERIOD]) {
		if (g_isInputKana)
			if(g_isEditing) {
				g_editController.append(_T('。'));
				InvalidateRect(hCont, NULL, true);
			}
			else
				sendTextToTarget(tstring(1, 0x3002));   // "。"
		else
			sendTextToTarget(_T("."));
	}
	else if (((!(g_isSwapHiphen) && id == g_bindedIdTable[HARD_KEY_HYPHEN]) || 
		(g_isSwapHiphen && id == g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000])) &&
		!(g_isInputKana && g_isEditing)) {

		if (hEdWindow != NULL && hEdWindow == GetForegroundWindow()) {
			int dicKey = pluginHardKeyToDicKey(hCont, false, HARD_KEY_HYPHEN);
			SendMessage(hEdWindow, g_dicKeyDownMessage, dicKey, 0);
		}
		else {
			if (g_isInputKana)
				sendTextToTarget(tstring(1, 0x3000));   // "　"
			else
				sendTextToTarget(_T(" "));
		}
	}
	else if ((id == g_bindedIdTable[0x20]) &&
		!(g_isInputKana && g_isEditing)) {

		if (hEdWindow != NULL && hEdWindow == GetForegroundWindow()) {
			int dicKey = pluginHardKeyToDicKey(hCont, false, HARD_KEY_HYPHEN);
			SendMessage(hEdWindow, g_dicKeyDownMessage, dicKey, 0);
		}
		else {
			if (g_isInputKana)
				sendTextToTarget(tstring(1, 0x3000));   // "　"
			else
				sendTextToTarget(_T(" "));
		}
	}
	else if ((id == g_bindedIdTable[0x8020]) &&
		!(g_isInputKana && g_isEditing)) {

		if (hEdWindow != NULL && hEdWindow == GetForegroundWindow()) {
			int dicKey = pluginHardKeyToDicKey(hCont, false, HARD_KEY_HYPHEN);
			SendMessage(hEdWindow, g_dicKeyDownMessage, dicKey, 0);
		}
		else {
			if (g_isInputKana)
				sendTextToTarget(tstring(1, 0x3000));   // "　"
			else
				sendTextToTarget(_T(" "));
		}
	}
	else if ((!(g_isSwapHiphen) && id == g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000]) || 
		(g_isSwapHiphen && id == g_bindedIdTable[HARD_KEY_HYPHEN]) &&
		!(g_isInputKana && g_isEditing)) {		//変換モードに入るキーを押された時の処理

		if (hEdWindow != NULL && hEdWindow == GetForegroundWindow()) {
			int dicKey = pluginHardKeyToDicKey(hCont, true, HARD_KEY_HYPHEN);
			SendMessage(hEdWindow, g_dicKeyDownMessage, dicKey, 0);
		}
		else {
			if (g_isInputKana)
				sendTextToTarget(tstring(1, 0x30fc));   // "ー"
			else
				sendTextToTarget(_T("-"));
		}
	}
	else {
		switch (g_inputMode) {
		case INPUT_MODE_GENERIC:
			handleHotKeyGeneric(hCont, id);
			break;
		case INPUT_MODE_NUMBER:
			handleHotKeyNumber(id);
			break;
		case INPUT_MODE_SYMBOL:
			handleHotKeySymbol(hCont, id);
			break;
		}
	}
}

static void startInputGeneric(HWND hCont) {
	int i;

	if (!g_isInputKana)
		return;

	for (i = 0; i < 26; i++) {
		pluginActivateKeyBinding(hCont, g_bindedAlphaIds[i]);
		pluginActivateKeyBinding(hCont, g_bindedAlphaFnIds[i]);
	}

	setInputWindowControlType(g_hInputWindow, INPUT_WINDOW_CONTROL_EDIT);
	g_isEditing = false;

	g_editController.clear();
}

static void endInputGeneric(HWND hCont) {
	int i;

	if (!g_isInputKana)
		return;

	hideInputWindow(g_hInputWindow);

	for (i = 0; i < 26; i++) {
		pluginInactivateKeyBinding(hCont, g_bindedAlphaIds[i]);
		pluginInactivateKeyBinding(hCont, g_bindedAlphaFnIds[i]);
	}

	if (g_isEditing) {
		pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACK]);
		pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACKSPACE]);
		pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_EXECUTE]);

		inactivateArrowBindings(hCont);
	}
}
static void handleHotKeyGeneric(HWND hCont, int id) {
	int i;

	bool isControlCand = getInputWindowControlType(g_hInputWindow) ==
		INPUT_WINDOW_CONTROL_CANDIDATE;
	int convMode = g_editController.getConvertMode();

	int alphaId = -1;
	bool alphaFn = false;

	if (g_isInputKana) {
		for (i = 0; i < 26; i++) {
			if (g_bindedAlphaIds[i] == id) {
				alphaFn = false;
				alphaId = i;
				break;
			}
			else if (g_bindedAlphaFnIds[i] == id) {
				alphaFn = true;
				alphaId = i;
				break;
			}
		}
	}

	if (g_isInputKana && g_isEditing) {
		if (isControlCand) {
			if (id == g_bindedIdTable[HARD_KEY_BACK] ||
				id == g_bindedIdTable[HARD_KEY_BACKSPACE]) {

				setInputWindowControlType(g_hInputWindow,
					INPUT_WINDOW_CONTROL_EDIT);

				g_editController.setConvertMode(
					EditController::CONVERT_MODE_NONE);
			}
			else if (id == g_bindedIdTable[HARD_KEY_HYPHEN] ||
				id == g_bindedIdTable[HARD_KEY_LEFT] ||
				id == g_bindedIdTable[HARD_KEY_RIGHT] ||
				id == g_bindedIdTable[HARD_KEY_UP] ||
				id == g_bindedIdTable[HARD_KEY_DOWN] ||
				id == g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000] ||
				id == g_bindedIdTable[0x20] ||
				id == g_bindedIdTable[0x8020])
			{
				if (id == g_bindedIdTable[HARD_KEY_LEFT])
					g_candidateController.prevCandidate();
				else if (id == g_bindedIdTable[HARD_KEY_HYPHEN] ||
					id == g_bindedIdTable[HARD_KEY_RIGHT])
				{
					g_candidateController.nextCandidate();
				}
				else if (id == g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000])
					g_candidateController.nextCandidate();
				else if (id == g_bindedIdTable[0x20])
					g_candidateController.nextCandidate();
				else if (id == g_bindedIdTable[0x20 | 0x8000])
					g_candidateController.prevCandidate();
				else if (id == g_bindedIdTable[HARD_KEY_UP])
					g_candidateController.prevCandidate(true);
				else if (id == g_bindedIdTable[HARD_KEY_DOWN])
					g_candidateController.nextCandidate(true);
			}
			else if (id == g_bindedIdTable[HARD_KEY_EXECUTE] ||
				alphaId != -1) {

				int selectedIndex = g_candidateController.getSelectedIndex();
				tstring selectedCand = g_currentCandidates[
					g_candidateController.getSelectedIndex()];

				g_currentCandidates.erase(g_currentCandidates.begin() +
					selectedIndex);
				g_currentCandidates.insert(g_currentCandidates.begin(),
					selectedCand);

				g_candidateCache.updateCandidates(
					g_editController.obtainRomaString(), g_currentCandidates);

				g_editController.setKanjiString(selectedCand);
				commitEditing(hCont);
			}
		}
		else {
			if (id == g_bindedIdTable[HARD_KEY_BACK]) {
				if (convMode == EditController::CONVERT_MODE_NONE)
					cancelEditing(hCont);
				else {
					setInputWindowControlType(g_hInputWindow,
						INPUT_WINDOW_CONTROL_EDIT);

					g_editController.setConvertMode(
						EditController::CONVERT_MODE_NONE);
				}
			}
			else if (id == g_bindedIdTable[HARD_KEY_BACKSPACE]) {
				 if (convMode == EditController::CONVERT_MODE_NONE) {
					  g_editController.chop();
					  if (g_editController.isEmpty())
						  cancelEditing(hCont);
				 }
				 else {
					 g_editController.setConvertMode(
						 EditController::CONVERT_MODE_NONE);
				 }
			}
			else if (id == g_bindedIdTable[HARD_KEY_HYPHEN])
				if (g_isSwapHiphen == true) {
					beginEditing(hCont, _T('ー'));
				} else {
					convertKanji();
				}
			else if (id == g_bindedIdTable[HARD_KEY_HYPHEN | 0x8000]) {
				if (g_isSwapHiphen == true) {
					convertKanji();
				} else {
					beginEditing(hCont, _T('ー'));
				}
			}
			else if (id == g_bindedIdTable[0x20]) 
				convertKanji();
			else if (id == g_bindedIdTable[HARD_KEY_RIGHT])
				convertKanji();
			else if (id == g_bindedIdTable[HARD_KEY_LEFT])
				convertKanji();
			else if (id == g_bindedIdTable[HARD_KEY_UP] ||
				id == g_bindedIdTable[HARD_KEY_DOWN])
			{
				int newMode = 0;
				switch (convMode) {
				case EditController::CONVERT_MODE_NONE:
				case EditController::CONVERT_MODE_KANJI:
					if (id == g_bindedIdTable[HARD_KEY_UP])
						newMode = EditController::CONVERT_MODE_FULL_ALPHA;
					else
						newMode = EditController::CONVERT_MODE_FULL_KATAKANA;
					break;
				case EditController::CONVERT_MODE_FULL_KATAKANA:
					if (id == g_bindedIdTable[HARD_KEY_UP])
						newMode = EditController::CONVERT_MODE_FULL_ALPHA;
					else
						newMode = EditController::CONVERT_MODE_HALF_KATAKANA;
					break;
				case EditController::CONVERT_MODE_HALF_KATAKANA:
					if (id == g_bindedIdTable[HARD_KEY_UP])
						newMode = EditController::CONVERT_MODE_FULL_KATAKANA;
					else
						newMode = EditController::CONVERT_MODE_FULL_ALPHA;
					break;
				case EditController::CONVERT_MODE_FULL_ALPHA:
					if (id == g_bindedIdTable[HARD_KEY_UP])
						newMode = EditController::CONVERT_MODE_HALF_KATAKANA;
					else
						newMode = EditController::CONVERT_MODE_FULL_KATAKANA;
					break;
				}

				g_editController.setConvertMode(newMode);
			}
			else if (id == g_bindedIdTable[HARD_KEY_EXECUTE] ||
				(convMode != EditController::CONVERT_MODE_NONE &&
					alphaId != -1)) {

				commitEditing(hCont);
			}
		}
	}

	if (g_isInputKana && alphaId != -1) {
		TCHAR ch = (alphaFn ? _T('A') : _T('a')) + alphaId;
		beginEditing(hCont, ch);
	}

	InvalidateRect(g_hInputWindow, NULL, false);
}

static void startInputNumber(HWND hCont) {
	int i;

	for (i = 0; i < 10; i++)
		pluginActivateKeyBinding(hCont, g_bindedNumberIds[i]);
}

static void endInputNumber(HWND hCont) {
	int i;

	for (i = 0; i < 10; i++)
		pluginInactivateKeyBinding(hCont, g_bindedNumberIds[i]);
}

static void handleHotKeyNumber(int id) {
	int i;

	int num = -1;
	for (i = 0; i < 10; i++) {
		if (g_bindedNumberIds[i] == id) {
			num = i;
			break;
		}
	}

	if (num == -1)
		return;

	if (g_isInputKana)
		sendTextToTarget(tstring(1, 0xff10 + num));
	else {
		unsigned char key = 0x30 + num;
		sendTextToTarget(tstring(1, key));
	}
}

static void startInputSymbol(HWND hCont) {
	if (g_isSubTouchUseEnabled) {
		if (g_isInputKana)
			g_padInput.switchPad(PadInput::PAD_FULL_SYMBOL);
		else
			g_padInput.switchPad(PadInput::PAD_HALF_SYMBOL);
	}
	else {
		pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACK]);
		pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_EXECUTE]);
		pluginActivateKeyBinding(hCont,
			g_bindedIdTable[HARD_KEY_EXECUTE | 0x8000]);

		activateArrowBindings(hCont);

		showInputWindow(g_hInputWindow);

		setInputWindowControlType(g_hInputWindow,
			INPUT_WINDOW_CONTROL_CANDIDATE);

		if (g_isInputKana) {
			if (!g_candidateCache.getCandidates(g_currentCandidates,
				_T("__FULL_SYMBOLS__"))) {

				g_currentCandidates = g_fullSymbolCandidateList;
			}
		}
		else {
			if (!g_candidateCache.getCandidates(g_currentCandidates,
				_T("__HALF_SYMBOLS__"))) {

				g_currentCandidates = g_halfSymbolCandidateList;
			}
		}

		g_candidateController.setCandidateList(&g_currentCandidates, false);
	}
}

static void endInputSymbol(HWND hCont) {
	if (g_isSubTouchUseEnabled) {
		if(!g_isLargePad)
			g_padInput.switchPad(PadInput::PAD_MODE);
	}
	else {
		hideInputWindow(g_hInputWindow);

		pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACK]);
		pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_EXECUTE]);
		pluginInactivateKeyBinding(hCont,
			g_bindedIdTable[HARD_KEY_EXECUTE | 0x8000]);

		inactivateArrowBindings(hCont);
	}
}

static void handleHotKeySymbol(HWND hCont, int id) {
	if (id == g_bindedIdTable[HARD_KEY_BACK]) {
		endInputMode(hCont);

		g_inputMode = INPUT_MODE_GENERIC;
		startInputMode(hCont);
	}
	else if (id == g_bindedIdTable[HARD_KEY_LEFT])
		g_candidateController.prevCandidate();
	else if (id == g_bindedIdTable[HARD_KEY_RIGHT])
		g_candidateController.nextCandidate();
	else if (id == g_bindedIdTable[HARD_KEY_UP])
		g_candidateController.prevCandidate(true);
	else if (id == g_bindedIdTable[HARD_KEY_DOWN])
		g_candidateController.nextCandidate(true);
	else if (id == g_bindedIdTable[HARD_KEY_EXECUTE] ||
		id == g_bindedIdTable[HARD_KEY_EXECUTE | 0x8000]) {

		int selectedIndex = g_candidateController.getSelectedIndex();
		tstring selectedCand = g_currentCandidates[
			g_candidateController.getSelectedIndex()];

		g_currentCandidates.erase(g_currentCandidates.begin() +
			selectedIndex);
		g_currentCandidates.insert(g_currentCandidates.begin(),
			selectedCand);

		if (g_isInputKana) {
			g_candidateCache.updateCandidates(_T("__FULL_SYMBOLS__"),
				g_currentCandidates);
		}
		else {
			g_candidateCache.updateCandidates(_T("__HALF_SYMBOLS__"),
				g_currentCandidates);
		}

		sendTextToTarget(selectedCand);

		if (id == g_bindedIdTable[HARD_KEY_EXECUTE]) {
			endInputMode(hCont);

			g_inputMode = INPUT_MODE_GENERIC;
			startInputMode(hCont);
		}
		else
			g_candidateController.firstCandidate();
	}

	InvalidateRect(g_hInputWindow, NULL, false);
}

static void processPadInputSymbol(HWND hCont, int id) {
	bool isFuncPressed = GetAsyncKeyState(HARD_KEY_FUNCTION) != 0;

	if (id == PadInput::BUTTON_SYMBOL_PAD_CLOSE) {
		endInputMode(hCont);

		g_inputMode = g_prevInputMode;
		startInputMode(hCont);
	}
	else if (id >= PadInput::BUTTON_SYMBOL_BASE) {
		if(g_isLargePad) {
			if(g_isInputKana && g_isEditing) {
				if(g_symbolMode == SYMBOL_FULL) {
					beginEditing(hCont, *g_fullSymbolCandidateList[
						id - PadInput::BUTTON_SYMBOL_BASE].c_str());
				}
				else {
					beginEditing(hCont, *g_halfSymbolCandidateList[
						id - PadInput::BUTTON_SYMBOL_BASE].c_str());
				}
				InvalidateRect(g_hInputWindow, NULL, true);
			}
			else {
				if(g_symbolMode == SYMBOL_FULL) {
					sendTextToTarget(g_fullSymbolCandidateList[
						id - PadInput::BUTTON_SYMBOL_BASE]);
				}
				else {
					sendTextToTarget(g_halfSymbolCandidateList[
						id - PadInput::BUTTON_SYMBOL_BASE]);
				}
			}
		}
		else {
			if (g_isInputKana) {
				sendTextToTarget(g_fullSymbolCandidateList[
					id - PadInput::BUTTON_SYMBOL_BASE]);
			}
			else {
				sendTextToTarget(g_halfSymbolCandidateList[
					id - PadInput::BUTTON_SYMBOL_BASE]);
			}

			if (!isFuncPressed) {
				endInputMode(hCont);

				g_inputMode = g_prevInputMode;
				startInputMode(hCont);
			}
		}
	}
}

static void sendTextToTarget(const tstring &text) {
	int i;

	int len = text.length();

	unsigned int *stateFlags = new unsigned int[len];
	unsigned int *textBuf = new unsigned int[len];

	for (i = 0; i < len; i++) {
		stateFlags[i] = KeyStateDownFlag;
		textBuf[i] = (unsigned int)text[i];
	}

	PostKeybdMessage((HWND)-1, 0, stateFlags[0], len, stateFlags, textBuf);

	delete [] stateFlags;
	delete [] textBuf;
}

static void beginEditing(HWND hCont, TCHAR ch) {
	g_editController.append(ch);

	if (!g_isEditing) {
		pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACK]);
		pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACKSPACE]);
		pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_EXECUTE]);


		activateArrowBindings(hCont);

		showInputWindow(g_hInputWindow);

		g_isEditing = true;
	}
}

static void cancelEditing(HWND hCont) {
	hideInputWindow(g_hInputWindow);

	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACK]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACKSPACE]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_EXECUTE]);

	inactivateArrowBindings(hCont);

	setInputWindowControlType(g_hInputWindow, INPUT_WINDOW_CONTROL_EDIT);
	g_isEditing = false;

	g_editController.clear();
}

static void commitEditing(HWND hCont) {
	sendTextToTarget(g_editController.getText());

	hideInputWindow(g_hInputWindow);

	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACK]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_BACKSPACE]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_EXECUTE]);

	inactivateArrowBindings(hCont);

	setInputWindowControlType(g_hInputWindow, INPUT_WINDOW_CONTROL_EDIT);
	g_isEditing = false;

	g_editController.clear();
}

static void convertKanji() {
	int i, j;

	int convMode = g_editController.getConvertMode();
	if (convMode == EditController::CONVERT_MODE_KANJI) {
		setInputWindowControlType(g_hInputWindow,
			INPUT_WINDOW_CONTROL_CANDIDATE);

		g_candidateController.setCandidateList(&g_currentCandidates, true);
		g_candidateController.nextCandidate();
	}
	else {
		if (!g_candidateCache.getCandidates(g_currentCandidates,
			g_editController.obtainRomaString())) {

			g_editController.makeProposedList();

			const vector<EditController::Proposed *> &propList =
				g_editController.getProposedList();

			if (propList.empty())
				return;

			g_currentCandidates.clear();

			vector<tstring> singleCands;
			int propListSize = propList.size();

			for (i = 0; i < propListSize; i++) {
				EditController::Proposed *prop = propList[i];
				if (!g_dictionary.find(singleCands, prop->getProposedString()))
					continue;

				int numCands = singleCands.size();
				for (j = 0; j < numCands; j++) {
					g_currentCandidates.push_back(singleCands[j] +
						prop->getFollowString());
				}
			}

			g_currentCandidates.push_back(
				g_editController.obtainConvertedString(
				EditController::CONVERT_MODE_FULL_KATAKANA));
			g_currentCandidates.push_back(
				g_editController.obtainConvertedString(
				EditController::CONVERT_MODE_NONE));
		}

		g_editController.setConvertMode(EditController::CONVERT_MODE_KANJI);

		g_editController.setKanjiString(g_currentCandidates[0]);
	}
}

static void activateArrowBindings(HWND hCont) {
	pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_LEFT]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_RIGHT]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_UP]);
	pluginActivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_DOWN]);
}

static void inactivateArrowBindings(HWND hCont) {
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_LEFT]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_RIGHT]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_UP]);
	pluginInactivateKeyBinding(hCont, g_bindedIdTable[HARD_KEY_DOWN]);
}

static int registerKeyBinding(HWND hCont, bool fn, unsigned int keyCode,
	bool active) {

	PluginKeyBinding bind = {0};
	bind.isEnabled = true;
	bind.defaultFn = fn;
	bind.defaultKeyCode = keyCode;
	bind.onKeyPress = onBindedKeyPress;
	bind.isActive = active;

	return pluginRegisterKeyBinding(hCont, &bind);
}

static int registerSpecialKeyBinding(HWND hCont, const tstring &name, bool fn,
	unsigned int keyCode, bool active) {

	PluginKeyBinding bind = {0};
	_tcscpy(bind.category, _T("InputSwitch"));
	_tcsncpy(bind.name, name.c_str(), 256);
	bind.isEnabled = false;
	bind.defaultFn = fn;
	bind.defaultKeyCode = keyCode;
	bind.onKeyPress = onBindedKeyPress;
	bind.isActive = active;

	return pluginRegisterKeyBinding(hCont, &bind);
}

static void saveCurrentSettings(HWND hCont) {
	int fontSize = getInputWindowFontSize(g_hInputWindow);

	map<tstring, tstring> props;
	TCHAR valCStr[256];

	_sntprintf(valCStr, 256, _T("%d"), fontSize);
	props[_T("fontSize")] = valCStr;

	props[_T("subTouchUseEnabled")] = g_isSubTouchUseEnabled ?
		_T("1") : _T("0");
	props[_T("subTouchLargePad")] = g_isLargePad ?
		_T("1") : _T("0");
	props[_T("disableAtEdApp")] = g_isDisableAtEd ?
		_T("1") : _T("0");
	props[_T("SwapHiphen")] = g_isSwapHiphen ?
		_T("1") : _T("0");

	tstring persistDir = _T("\\Nand2\\.ceopener");
	if (GetFileAttributes(persistDir.c_str()) == -1)
		CreateDirectory(persistDir.c_str(), NULL);

	tstring propFileName = persistDir + _T("\\inputswitch.dat");
	KnceUtil::writePropertyFile(propFileName, props);
}