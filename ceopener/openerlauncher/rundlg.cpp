#include "rundlg.h"

#include <algorithm>
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
    IDC_PROGRAM_COMBO = 102,
    IDC_PROGRAM_BROWSE = 103
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onProgramBrowse(HWND hDlg);

extern HINSTANCE g_hInstance;

struct RunDialogData {
    RunParams *runParams;
};

RunParams::RunParams() {
}

RunParams::~RunParams() {
}

bool showRunDialog(HWND hOwnerWindow, RunParams &params) {
    int ret = DialogBoxParam(g_hInstance, _T("RUN"),
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
        case IDC_PROGRAM_BROWSE:
            onProgramBrowse(hDlg);
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

    RunDialogData *data = new RunDialogData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->runParams = (RunParams *)params;

    KnceUtil::adjustDialogLayout(hDlg);

    HWND hProgCombo = GetDlgItem(hDlg, IDC_PROGRAM_COMBO);

    vector<tstring> &progs = data->runParams->getProgramHistory();
    int numProgs = progs.size();

    for (i = 0; i < numProgs; i++)
        SendMessage(hProgCombo, CB_ADDSTRING, 0, (LPARAM)progs[i].c_str());

    if (numProgs > 0)
        SendMessage(hProgCombo, CB_SETCURSEL, 0, 0);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    RunDialogData *data = (RunDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hProgCombo = GetDlgItem(hDlg, IDC_PROGRAM_COMBO);

    TCHAR cmdLineCStr[MAX_PATH];
    GetWindowText(hProgCombo, cmdLineCStr, MAX_PATH);
    tstring cmdLine = cmdLineCStr;

    tstring fileName;
    bool isInQuote = false;
    int cmdLineLen = cmdLine.size();

    int pos = 0;
    for ( ; pos < cmdLineLen; pos++) {
        TCHAR ch = cmdLine[pos];
        if (!isInQuote && ch == _T(' '))
            break;
        else if (ch == _T('\"'))
            isInQuote = !isInQuote;
        else
            fileName += ch;
    }

    tstring argStr;
    if (pos < cmdLineLen)
        argStr = cmdLine.substr(pos + 1);

    if ((GetFileAttributes(fileName.c_str()) &
        FILE_ATTRIBUTE_DIRECTORY) != 0) {

        argStr = fileName;
        fileName = _T("\\Windows\\explorer.exe");
    }

    SHELLEXECUTEINFO execInfo = {0};
    execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    execInfo.fMask = 0;
    execInfo.hwnd = hDlg;
    execInfo.lpVerb = _T("open");
    execInfo.lpFile = fileName.c_str();
    execInfo.lpParameters = argStr.c_str();
    execInfo.nShow = SW_SHOWNORMAL;
    execInfo.hInstApp = g_hInstance;

    if (ShellExecuteEx(&execInfo)) {
        vector<tstring> &progs = data->runParams->getProgramHistory();

        vector<tstring>::iterator iter = find(progs.begin(), progs.end(),
            cmdLine);
        if (iter != progs.end())
            progs.erase(iter);

        progs.insert(progs.begin(), cmdLine);
    }

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    RunDialogData *data = (RunDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onProgramBrowse(HWND hDlg) {
    RunDialogData *data = (RunDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hProgCombo = GetDlgItem(hDlg, IDC_PROGRAM_COMBO);

    TCHAR fileNameCStr[MAX_PATH];
    GetWindowText(hProgCombo, fileNameCStr, MAX_PATH);

    KnceChooseFileParams params = {0};
    params.isSaveFile = false;
    params.filters = _T("Programs (*.exe)|*.exe|All files (*.*)|*.*");
    _tcsncpy(params.fileName, fileNameCStr, MAX_PATH);

    if (knceChooseFile(hDlg, &params)) {
        tstring cmdLine = tstring(_T("\"")) + params.fileName + _T("\"");
        SetWindowText(hProgCombo, cmdLine.c_str());
    }
}
