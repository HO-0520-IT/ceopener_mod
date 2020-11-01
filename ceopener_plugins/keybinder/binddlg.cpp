#include "binddlg.h"

#include <map>
#include <string>
#include <windows.h>
#include <knceutil.h>
#include <kncedlg.h>
#include "presetdlg.h"
#include "settingdlg.h"

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_BIND_LIST = 101,
    IDC_ADD_BUTTON = 111,
    IDC_EDIT_BUTTON = 112,
    IDC_REMOVE_BUTTON = 113,
    IDC_IMPORT_BUTTON = 121,
    IDC_EXPORT_BUTTON = 122,
	IDC_SETTING_BUTTON = 131
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onBindList(HWND hDlg, int event);
static void onAdd(HWND hDlg);
static void onEdit(HWND hDlg);
static void onRemove(HWND hDlg);
static void onImport(HWND hDlg);
static void onExport(HWND hDlg);
static void updateControlStates(HWND hDlg);
static tstring obtainNewBindingName(HWND hDlg);
static void readPropertyFile(map<tstring, tstring> &props,
    const tstring &fileName);
static void writePropertyFile(const tstring &fileName,
    const map<tstring, tstring> &props);
static void showSettings(HWND hDlg);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;

struct BindDialogData {
    BindDialogParams *dialogParams;
    vector<BindItem *> bindItems;
};

BindDialogParams::BindDialogParams() {
}

BindDialogParams::~BindDialogParams() {
}

bool showBindDialog(HWND hOwnerWindow, BindDialogParams &params) {
    int ret = DialogBoxParam(g_hInstance, _T("BIND"), hOwnerWindow,
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
        int id    = LOWORD(wParam); 
        int event = HIWORD(wParam);

        switch (id) {
        case IDOK:
            onOk(hDlg);
            break;
        case IDCANCEL:
            onCancel(hDlg);
            break;
        case IDC_BIND_LIST:
            onBindList(hDlg, event);
            break;
        case IDC_ADD_BUTTON:
            onAdd(hDlg);
            break;
        case IDC_EDIT_BUTTON:
            onEdit(hDlg);
            break;
        case IDC_REMOVE_BUTTON:
            onRemove(hDlg);
            break;
        case IDC_IMPORT_BUTTON:
            onImport(hDlg);
            break;
        case IDC_EXPORT_BUTTON:
            onExport(hDlg);
            break;
		case IDC_SETTING_BUTTON:
			showSettings(hDlg);
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

    BindDialogData *data = new BindDialogData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->dialogParams = (BindDialogParams *)params;

    KnceUtil::adjustDialogLayout(hDlg);

    HWND hBindList = GetDlgItem(hDlg, IDC_BIND_LIST);

    vector<BindItem *> &bindItems = data->dialogParams->getBindItems();
    int numItems = bindItems.size();

    for (i = 0; i < numItems; i++) {
        BindItem *bindItem = bindItems[i];

        data->bindItems.push_back(new BindItem(*bindItem));  // cloned

        SendMessage(hBindList, LB_ADDSTRING, 0,
            (LPARAM)bindItem->getName().c_str());
    }

    if (numItems > 0)
        SendMessage(hBindList, LB_SETCURSEL, 0, 0);

    updateControlStates(hDlg);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    int i;

    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    vector<BindItem *> &bindItems = data->dialogParams->getBindItems();

    int numItems = bindItems.size();
    for (i = 0; i < numItems; i++)
        delete bindItems[i];

    bindItems.clear();

    numItems = data->bindItems.size();
    for (i = 0; i < numItems; i++)
        bindItems.push_back(data->bindItems[i]);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    int i;

    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    int numItems = data->bindItems.size();
    for (i = 0; i < numItems; i++)
        delete data->bindItems[i];

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onBindList(HWND hDlg, int event) {
    if (event == LBN_SELCHANGE)
        updateControlStates(hDlg);
}

static void onAdd(HWND hDlg) {
    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    BindItem *bindItem = new BindItem();
    bindItem->setName(obtainNewBindingName(hDlg));
    bindItem->setVirtualKeyCode(VK_LBUTTON);
    bindItem->setSpecialAction(BindItem::SPECIAL_ACTION_FOREGROUND_ED_WINDOW);

    PresetDialogParams params;
    params.setBindItem(bindItem);

    if (!showPresetDialog(hDlg, params)) {
        delete bindItem;

        return;
    }

    data->bindItems.push_back(bindItem);

    HWND hBindList = GetDlgItem(hDlg, IDC_BIND_LIST);
    SendMessage(hBindList, LB_ADDSTRING, 0,
        (LPARAM)bindItem->getName().c_str());

    SendMessage(hBindList, LB_SETCURSEL, data->bindItems.size() - 1, 0);

    updateControlStates(hDlg);
}

static void onEdit(HWND hDlg) {
    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hBindList = GetDlgItem(hDlg, IDC_BIND_LIST);
    int index = SendMessage(hBindList, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR)
        return;

    BindItem *bindItem = data->bindItems[index];

    PresetDialogParams params;
    params.setBindItem(bindItem);

    if (!showPresetDialog(hDlg, params))
        return;

    SendMessage(hBindList, LB_DELETESTRING, index, 0);
    SendMessage(hBindList, LB_INSERTSTRING, index,
        (LPARAM)bindItem->getName().c_str());

    SendMessage(hBindList, LB_SETCURSEL, index, 0);

    updateControlStates(hDlg);
}

static void onRemove(HWND hDlg) {
    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hBindList = GetDlgItem(hDlg, IDC_BIND_LIST);
    int index = SendMessage(hBindList, LB_GETCURSEL, 0, 0);
    if (index == LB_ERR)
        return;

    BindItem *bindItem = data->bindItems[index];
    delete bindItem;

    SendMessage(hBindList, LB_DELETESTRING, index, 0);

    vector<BindItem *>::iterator iter =
        data->bindItems.erase(data->bindItems.begin() + index);

    if (iter != data->bindItems.end()) {
        index = distance(data->bindItems.begin(), iter);
        SendMessage(hBindList, LB_SETCURSEL, index, 0);
    }

    updateControlStates(hDlg);
}

static void onImport(HWND hDlg) {
    int i;

    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    KnceChooseFileParams params = {0};
    params.isSaveFile = false;
    params.filters = _T("定義ファイル (*.keybind)|*.keybind|")
        _T("すべてのファイル (*.*)|*.*");

    if (!knceChooseFile(hDlg, &params))
        return;

    int numItems = data->bindItems.size();
    for (i = 0; i < numItems; i++)
        delete data->bindItems[i];

    data->bindItems.clear();

    HWND hBindList = GetDlgItem(hDlg, IDC_BIND_LIST);
    SendMessage(hBindList, LB_RESETCONTENT, 0, 0);

    map<tstring, tstring> props;
    readPropertyFile(props, params.fileName);

    TCHAR keyCStr[256];
    numItems = _ttoi(props[_T("numBindings")].c_str());

    for (i = 0; i < numItems; i++) {
        BindItem *bindItem = new BindItem();

        _sntprintf(keyCStr, 256, _T("binding.%d.name"), i);
        bindItem->setName(props[keyCStr]);

        _sntprintf(keyCStr, 256, _T("binding.%d.targetKeyFn"), i);
        bindItem->setTargetKeyFn(_ttoi(props[keyCStr].c_str()) != 0);

        _sntprintf(keyCStr, 256, _T("binding.%d.targetKeyCode"), i);
        bindItem->setTargetKeyCode(_ttoi(props[keyCStr].c_str()));

        _sntprintf(keyCStr, 256, _T("binding.%d.actionType"), i);
        bindItem->setActionType(_ttoi(props[keyCStr].c_str()));

        _sntprintf(keyCStr, 256, _T("binding.%d.virtualKeyModifiers"), i);
        bindItem->setVirtualKeyModifiers(_ttoi(props[keyCStr].c_str()));

        _sntprintf(keyCStr, 256, _T("binding.%d.virtualKeyCode"), i);
        bindItem->setVirtualKeyCode(_ttoi(props[keyCStr].c_str()));

        _sntprintf(keyCStr, 256, _T("binding.%d.programFileName"), i);
        bindItem->setProgramFileName(props[keyCStr]);

        _sntprintf(keyCStr, 256, _T("binding.%d.specialAction"), i);
        bindItem->setSpecialAction(_ttoi(props[keyCStr].c_str()));

        data->bindItems.push_back(bindItem);

        SendMessage(hBindList, LB_ADDSTRING, 0,
            (LPARAM)bindItem->getName().c_str());
    }

    if (numItems > 0)
        SendMessage(hBindList, LB_SETCURSEL, 0, 0);

    updateControlStates(hDlg);

    tstring msg = tstring(_T("'")) + params.fileName + _T("' をインポートしました。");
    MessageBox(hDlg, msg.c_str(), _T("情報"), MB_ICONINFORMATION);
}

static void onExport(HWND hDlg) {
    int i;

    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    KnceChooseFileParams params = {0};
    params.isSaveFile = true;
    params.filters = _T("定義ファイル (*.keybind)|*.keybind|")
        _T("すべてのファイル (*.*)|*.*");

    if (!knceChooseFile(hDlg, &params))
        return;

    map<tstring, tstring> props;
    TCHAR keyCStr[256];
    TCHAR valCStr[256];

    int numItems = data->bindItems.size();
    _sntprintf(valCStr, 256, _T("%d"), numItems);
    props[_T("numBindings")] = valCStr;

    for (i = 0; i < numItems; i++) {
        BindItem *item = data->bindItems[i];

        _sntprintf(keyCStr, 256, _T("binding.%d.name"), i);
        props[keyCStr] = item->getName();

        _sntprintf(keyCStr, 256, _T("binding.%d.targetKeyFn"), i);
        _sntprintf(valCStr, 256, _T("%d"), item->getTargetKeyFn());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("binding.%d.targetKeyCode"), i);
        _sntprintf(valCStr, 256, _T("%d"), item->getTargetKeyCode());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("binding.%d.actionType"), i);
        _sntprintf(valCStr, 256, _T("%d"), item->getActionType());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("binding.%d.virtualKeyModifiers"), i);
        _sntprintf(valCStr, 256, _T("%d"), item->getVirtualKeyModifiers());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("binding.%d.virtualKeyCode"), i);
        _sntprintf(valCStr, 256, _T("%d"), item->getVirtualKeyCode());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("binding.%d.programFileName"), i);
        props[keyCStr] = item->getProgramFileName();

        _sntprintf(keyCStr, 256, _T("binding.%d.specialAction"), i);
        _sntprintf(valCStr, 256, _T("%d"), item->getSpecialAction());
        props[keyCStr] = valCStr;
    }

    writePropertyFile(params.fileName, props);

    tstring msg = tstring(_T("'")) + params.fileName + _T("'にエクスポートしました。");
    MessageBox(hDlg, msg.c_str(), _T("情報"), MB_ICONINFORMATION);
}

static void updateControlStates(HWND hDlg) {
    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hEditButton = GetDlgItem(hDlg, IDC_EDIT_BUTTON);
    HWND hRemoveButton = GetDlgItem(hDlg, IDC_REMOVE_BUTTON);

    HWND hBindList = GetDlgItem(hDlg, IDC_BIND_LIST);
    int index = SendMessage(hBindList, LB_GETCURSEL, 0, 0);

    if (index == LB_ERR) {
        EnableWindow(hEditButton, false);
        EnableWindow(hRemoveButton, false);
    }
    else {
        EnableWindow(hEditButton, true);
        EnableWindow(hRemoveButton, true);
    }
}

static tstring obtainNewBindingName(HWND hDlg) {
    int i;

    BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hBindList = GetDlgItem(hDlg, IDC_BIND_LIST);
    TCHAR nameCStr[256];

    for (i = 0; i < 1000; i++) {
        _sntprintf(nameCStr, 256, _T("Binding %d"), i + 1);
        int index = SendMessage(hBindList, LB_FINDSTRINGEXACT, 0,
            (LPARAM)nameCStr);
        if (index == CB_ERR)
            return nameCStr;
    }

    return _T("");
}

static void readPropertyFile(map<tstring, tstring> &props,
    const tstring &fileName) {

    FILE *file = _tfopen(fileName.c_str(), _T("r"));

    TCHAR lineBuf[1024];
    char mbLineBuf[sizeof(lineBuf) * 2];

    while (true) {
        if (fgets(mbLineBuf, sizeof(mbLineBuf), file) == NULL)
            break;

        char *cr = strchr(mbLineBuf, '\n');
        if (cr != NULL)
            *cr = '\0';

        MultiByteToWideChar(932, 0, mbLineBuf, -1, lineBuf, sizeof(lineBuf));

        tstring line = lineBuf;
        int pos = line.find(_T('='));

        props[line.substr(0, pos)] = line.substr(pos + 1);
    }

    fclose(file);
}

static void writePropertyFile(const tstring &fileName,
    const map<tstring, tstring> &props) {

    FILE *file = _tfopen(fileName.c_str(), _T("w"));

    char mbLineBuf[1024 * 2];
    map<tstring, tstring>::const_iterator iter = props.begin();

    for ( ; iter != props.end(); iter++) {
        tstring line = iter->first + _T("=") + iter->second + _T("\n");

        WideCharToMultiByte(932, 0, line.c_str(), -1, mbLineBuf,
            sizeof(mbLineBuf), NULL, NULL);

        fputs(mbLineBuf, file);
    }

    fclose(file);
}

static void showSettings(HWND hDlg) {
	BindDialogData *data = (BindDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

	SettingDialogParams params;
	params.setDisableAtEd(data->dialogParams->isDisableAtEd());
	params.setStoreData(data->dialogParams->isStoreData());
	
	showSettingDialog(hDlg, params);

	data->dialogParams->setDisableAtEd(params.isDisableAtEd());
	data->dialogParams->setStoreData(params.isStoreData());
}