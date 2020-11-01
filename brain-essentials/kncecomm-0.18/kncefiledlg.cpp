#include "kncecomm.h"

#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_CHOOSE_FILE_FILE_LIST = 101,
    IDC_CHOOSE_FILE_CURRENT_DIRECTORY_LABEL = 102,
    IDC_CHOOSE_FILE_DIRECTORY_LIST = 103,
    IDC_CHOOSE_FILE_FILE_NAME_BOX = 104,
    IDC_CHOOSE_FILE_TYPE_COMBO = 105
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static bool onInitDialog(HWND hDlg, void *createStruct);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onFileList(HWND hDlg, int event);
static void onDirectoryList(HWND hDlg, int event);
static void onTypeCombo(HWND hDlg, int event);
static void onFileNameBox(HWND hDlg, int event);
static void updateFileList(HWND hDlg);
static void split(vector<tstring> &result, const tstring &str,
    const tstring &delim);
static tstring appendFileExtension(const tstring &fileName,
    const tstring &pat);

extern HINSTANCE g_hInstance;

struct KnceFileDlgData {
    HFONT hFont;
    KnceChooseFileParams *chooseFileParams;
    vector<pair<tstring, tstring> > filters;
    tstring currentPath;
    bool eventProcessing;
};

bool showChooseFileDialog(HWND hOwnerWindow, KnceChooseFileParams *params) {
    int ret = DialogBoxParam(g_hInstance, _T("CHOOSE_FILE"), hOwnerWindow,
        (DLGPROC)dlgProc, (LPARAM)params);

    return ret == IDOK;
}

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
    case WM_INITDIALOG:
        return onInitDialog(hDlg, (void *)lParam);
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
        case IDC_CHOOSE_FILE_FILE_LIST:
            onFileList(hDlg, event);
            break;
        case IDC_CHOOSE_FILE_DIRECTORY_LIST:
            onDirectoryList(hDlg, event);
            break;
        case IDC_CHOOSE_FILE_FILE_NAME_BOX:
            onFileNameBox(hDlg, event);
            break;
        case IDC_CHOOSE_FILE_TYPE_COMBO:
            onTypeCombo(hDlg, event);
            break;
        default:
            return false;
        }

        return true;
    }
	}

	return false;
}

static bool onInitDialog(HWND hDlg, void *params) {
    int i;

    KnceFileDlgData *data = new KnceFileDlgData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->hFont = knceCreateDefaultFont(true, 100, false, false);
    knceSetDialogFont(hDlg, data->hFont);

    data->chooseFileParams = (KnceChooseFileParams *)params;
    data->eventProcessing = false;

    vector<tstring> filterParts;
    split(filterParts, data->chooseFileParams->filters, _T("|"));

    int numFilters = filterParts.size() / 2;
    for (i = 0; i < numFilters; i++) {
        data->filters.push_back(make_pair(filterParts[i * 2],
            filterParts[i * 2 + 1]));
    }

    HWND hFileList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_FILE_LIST);
    HWND hFileNameBox = GetDlgItem(hDlg, IDC_CHOOSE_FILE_FILE_NAME_BOX);

    if (data->chooseFileParams->isSaveFile) {
        SetWindowText(hDlg, _T("Save File"));

        unsigned long style = GetWindowLong(hFileList, GWL_STYLE);
        SetWindowLong(hFileList, GWL_STYLE, style | LBS_NOSEL);
        SetWindowPos(hFileList, NULL, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE |
            SWP_NOZORDER | SWP_FRAMECHANGED));
    }
    else {
        ShowWindow(hFileNameBox, SW_HIDE);

        RECT rect;
        GetWindowRect(hFileList, &rect);
        SetWindowPos(hFileList, NULL, 0, 0, rect.right - rect.left, 155,
            SWP_NOMOVE | SWP_NOZORDER);
    }

    HWND hTypeCombo = GetDlgItem(hDlg, IDC_CHOOSE_FILE_TYPE_COMBO);

    for (i = 0; i < numFilters; i++) {
        SendMessage(hTypeCombo, CB_ADDSTRING, 0,
            (LPARAM)data->filters[i].first.c_str());
    }

    if (numFilters > 0)
        SendMessage(hTypeCombo, CB_SETCURSEL, 0, 0);

    tstring fileName;
    tstring filePath = data->chooseFileParams->fileName;

    if (filePath.empty()) {
        data->currentPath = data->chooseFileParams->initialPath;
        if (data->currentPath.empty())
            data->currentPath = _T("\\");
    }
    else {
        int pos = filePath.rfind(_T('\\'));
        if (pos == 0)
            data->currentPath = _T("\\");
        else
            data->currentPath = filePath.substr(0, pos);

        fileName = filePath.substr(pos + 1);
    }

    updateFileList(hDlg);

    ShowWindow(hDlg, SW_SHOW);

    if (data->chooseFileParams->isSaveFile) {
        if (fileName.empty())
            SetWindowText(hFileNameBox, _T("untitled"));
        else
            SetWindowText(hFileNameBox, fileName.c_str());

        SendMessage(hFileNameBox, EM_SETSEL, 0, -1);
        SetFocus(hFileNameBox);

        return false;
    }
    else
        return true;
}

static void onOk(HWND hDlg) {
    KnceFileDlgData *data =
        (KnceFileDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hDirList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_DIRECTORY_LIST);
    if (GetFocus() == hDirList) {
        onDirectoryList(hDlg, LBN_DBLCLK);
        return;
    }

    tstring fileName;
    if (data->chooseFileParams->isSaveFile) {
        HWND hFileNameBox = GetDlgItem(hDlg, IDC_CHOOSE_FILE_FILE_NAME_BOX);

        TCHAR fileNameCStr[MAX_PATH];
        GetWindowText(hFileNameBox, fileNameCStr, MAX_PATH);
        fileName = fileNameCStr;

        HWND hTypeCombo = GetDlgItem(hDlg, IDC_CHOOSE_FILE_TYPE_COMBO);
        int typeIndex = SendMessage(hTypeCombo, CB_GETCURSEL, 0, 0);
        tstring pat = data->filters[typeIndex].second;

        vector<tstring> exts;
        split(exts, pat, _T(";"));

        if (!exts.empty())
            fileName = appendFileExtension(fileName, exts[0]);
    }
    else {
        HWND hFileList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_FILE_LIST);
        int selected = SendMessage(hFileList, LB_GETCURSEL, 0, 0);

        TCHAR fileNameCStr[MAX_PATH];

        if (SendMessage(hFileList, LB_GETTEXTLEN, selected, 0) < MAX_PATH) {
            SendMessage(hFileList, LB_GETTEXT, selected, (LPARAM)fileNameCStr);
            fileName = fileNameCStr;
        }
    }

    tstring filePath;
    if (data->currentPath == _T("\\"))
        filePath = data->currentPath + fileName;
    else
        filePath = data->currentPath + _T("\\") + fileName;

    _tcsncpy(data->chooseFileParams->fileName, filePath.c_str(), MAX_PATH);

    if (data->chooseFileParams->isSaveFile) {
        FILE *file = _tfopen(filePath.c_str(), _T("r"));
        if (file != NULL) {
            fclose(file);
            tstring msg = _T("File ") + fileName + _T(" already exists. ")
                _T("Overwrite?");

            if (MessageBox(hDlg, msg.c_str(), _T("Confirm"),
                MB_ICONEXCLAMATION | MB_YESNO) == IDNO)
            {
                return;
            }
        }
    }

    DeleteObject(data->hFont);

    EndDialog(hDlg, IDOK);
}

static void onCancel(HWND hDlg) {
    KnceFileDlgData *data =
        (KnceFileDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    DeleteObject(data->hFont);

    EndDialog(hDlg, IDCANCEL);
}

static void onFileList(HWND hDlg, int event) {
    KnceFileDlgData *data =
        (KnceFileDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    if (event != LBN_SELCHANGE || data->eventProcessing)
        return;

    HWND hFileList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_FILE_LIST);
    int selected = SendMessage(hFileList, LB_GETCURSEL, 0, 0);
    if (selected == -1)
        return;

    tstring fileName;
    TCHAR fileNameCStr[MAX_PATH];

    if (SendMessage(hFileList, LB_GETTEXTLEN, selected, 0) < MAX_PATH) {
        SendMessage(hFileList, LB_GETTEXT, selected, (LPARAM)fileNameCStr);
        fileName = fileNameCStr;
    }

    HWND hFileNameBox = GetDlgItem(hDlg,
        IDC_CHOOSE_FILE_FILE_NAME_BOX);

    data->eventProcessing = true;
    SetWindowText(hFileNameBox, fileName.c_str());
    data->eventProcessing = false;

    if (!data->chooseFileParams->isSaveFile) {
        HWND hOkButton = GetDlgItem(hDlg, IDOK);
        EnableWindow(hOkButton, true);
    }
}

static void onDirectoryList(HWND hDlg, int event) {
    const int dirDispLen = MAX_PATH + 2;

    KnceFileDlgData *data =
        (KnceFileDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    if (event != LBN_DBLCLK)
        return;

    HWND hDirList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_DIRECTORY_LIST);

    int selected = SendMessage(hDirList, LB_GETCURSEL, 0, 0);
    if (selected == -1)
        return;

    tstring dirDisp;
    TCHAR dirDispCStr[dirDispLen];

    if (SendMessage(hDirList, LB_GETTEXTLEN, selected, 0) < dirDispLen) {
        SendMessage(hDirList, LB_GETTEXT, selected, (LPARAM)dirDispCStr);
        dirDisp = dirDispCStr;
    }

    if (dirDisp == _T("(Up...)")) {
        int pos = data->currentPath.rfind(_T('\\'));
        if (pos == 0)
            data->currentPath = _T("\\");
        else
            data->currentPath = data->currentPath.substr(0, pos);
    }
    else {
        tstring dirName = dirDisp.substr(1, dirDisp.length() - 2);
        if (data->currentPath == _T("\\"))
            data->currentPath += dirName;
        else
            data->currentPath += _T("\\") + dirName;
    }

    updateFileList(hDlg);
}

static void onFileNameBox(HWND hDlg, int event) {
    KnceFileDlgData *data =
        (KnceFileDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    if (event != EN_CHANGE || data->eventProcessing)
        return;

    HWND hFileList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_FILE_LIST);

    data->eventProcessing = true;
    SendMessage(hFileList, LB_SETCURSEL, -1, 0);
    data->eventProcessing = false;
}

static void onTypeCombo(HWND hDlg, int event) {
    if (event == CBN_SELCHANGE)
        updateFileList(hDlg);
}

static void updateFileList(HWND hDlg) {
    KnceFileDlgData *data =
        (KnceFileDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    bool isRoot = data->currentPath == _T("\\");

    tstring dirName;
    if (isRoot)
        dirName = _T("[My Device]");
    else {
        dirName = _T("[") + data->currentPath.substr(
            data->currentPath.rfind(_T('\\')) + 1) + _T("]");
    }

    HWND hDirLabel = GetDlgItem(hDlg, IDC_CHOOSE_FILE_CURRENT_DIRECTORY_LABEL);
    SetWindowText(hDirLabel, dirName.c_str());

    HWND hTypeCombo = GetDlgItem(hDlg, IDC_CHOOSE_FILE_TYPE_COMBO);
    int typeIndex = SendMessage(hTypeCombo, CB_GETCURSEL, 0, 0);
    tstring pat = data->filters[typeIndex].second;

    HWND hFileList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_FILE_LIST);
    HWND hDirList = GetDlgItem(hDlg, IDC_CHOOSE_FILE_DIRECTORY_LIST);

    SendMessage(hFileList, LB_RESETCONTENT, 0, 0);
    SendMessage(hDirList, LB_RESETCONTENT, 0, 0);

    if (!isRoot)
        SendMessage(hDirList, LB_ADDSTRING, 0, (LPARAM)_T("(Up...)"));

    tstring findPath;
    if (isRoot)
        findPath = _T("\\*.*");
    else
        findPath = data->currentPath + _T("\\*.*");

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(findPath.c_str(), &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            tstring dirDisp = _T("[") + tstring(fd.cFileName) + _T("]");
            SendMessage(hDirList, LB_ADDSTRING, 0, (LPARAM)dirDisp.c_str());
        }
        else if (knceMatchMultiFileExtension(fd.cFileName, pat.c_str()))
            SendMessage(hFileList, LB_ADDSTRING, 0, (LPARAM)fd.cFileName);

        while (FindNextFile(hFind, &fd)) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                tstring dirDisp = _T("[") + tstring(fd.cFileName) + _T("]");
                SendMessage(hDirList, LB_ADDSTRING, 0,
                    (LPARAM)dirDisp.c_str());
            }
            else if (knceMatchMultiFileExtension(fd.cFileName, pat.c_str()))
                SendMessage(hFileList, LB_ADDSTRING, 0, (LPARAM)fd.cFileName);
        }
    }

    FindClose(hFind);

    if (!data->chooseFileParams->isSaveFile) {
        HWND hOkButton = GetDlgItem(hDlg, IDOK);
        EnableWindow(hOkButton, false);
    }

    if (SendMessage(hFileList, LB_GETCOUNT, 0, 0) > 0) {
        SendMessage(hFileList, LB_SETCURSEL, 0, 0);

        onFileList(hDlg, LBN_SELCHANGE);
    }

    int numDirs = SendMessage(hDirList, LB_GETCOUNT, 0, 0);
    if (isRoot)
        SendMessage(hDirList, LB_SETCURSEL, 0, 0);
    else {
        if (numDirs > 1)
            SendMessage(hDirList, LB_SETCURSEL, 1, 0);
        else
            SendMessage(hDirList, LB_SETCURSEL, 0, 0);
    }
}

static void split(vector<tstring> &result, const tstring &str,
    const tstring &delim) {

    tstring strWork = str;

    int cutAt = 0;
    while ((cutAt = strWork.find_first_of(delim)) != tstring::npos) {
        if (cutAt > 0)
            result.push_back(strWork.substr(0, cutAt));
        strWork = strWork.substr(cutAt + 1);
    }

    if (strWork.length() > 0)
        result.push_back(strWork);
}

static tstring appendFileExtension(const tstring &fileName,
    const tstring &pat) {

    if (knceMatchFileExtension(fileName.c_str(), pat.c_str()))
        return fileName;

    if (pat.length() < 2 || pat.substr(0, 2) != _T("*."))
        return fileName;

    return fileName + pat.substr(1);
}
