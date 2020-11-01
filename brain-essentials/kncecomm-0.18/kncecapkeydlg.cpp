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
    IDC_KEY_LABEL = 103
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam,
    LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onTimer(HWND hDlg);

extern HINSTANCE g_hInstance;

struct KnceCapKeyDlgData {
    HFONT hFont;
    int *capturedKeyCode;
};

bool showCaptureKeyDialog(HWND hOwnerWindow, int *capturedKeyCode) {
    int ret = DialogBoxParam(g_hInstance, _T("CAPTURE_KEY"), hOwnerWindow,
        (DLGPROC)dlgProc, (LPARAM)capturedKeyCode);

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
        default:
            return false;
        }

        return true;
    }
    case WM_TIMER:
        onTimer(hDlg);
        return true;
	}

	return false;
}

static void onInitDialog(HWND hDlg, void *params) {
    KnceCapKeyDlgData *data = new KnceCapKeyDlgData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->hFont = knceCreateDefaultFont(true, 100, false, false);
    knceSetDialogFont(hDlg, data->hFont);

    data->capturedKeyCode = (int *)params;
    *data->capturedKeyCode = -1;

    HWND hOkButton = GetDlgItem(hDlg, IDOK);
    EnableWindow(hOkButton, false);

    SetTimer(hDlg, 1, 100, NULL);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    KnceCapKeyDlgData *data = (KnceCapKeyDlgData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    KillTimer(hDlg, 1);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    KnceCapKeyDlgData *data = (KnceCapKeyDlgData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    KillTimer(hDlg, 1);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onTimer(HWND hDlg) {
    int i;

    KnceCapKeyDlgData *data = (KnceCapKeyDlgData *)GetWindowLong(hDlg,
        GWL_USERDATA);

    int keyCode = -1;
    for (i = 0; i < 256; i++) {
        if (GetAsyncKeyState(i) != 0) {
            keyCode = i;
            break;
        }
    }

    if (keyCode == -1 || keyCode == 1 ||
        keyCode == HARD_KEY_EXECUTE ||
        keyCode == HARD_KEY_BACK ||
        keyCode == HARD_KEY_LEFT || keyCode == HARD_KEY_UP ||
        keyCode == HARD_KEY_RIGHT || keyCode == HARD_KEY_DOWN)
    {
        return;
    }

    *data->capturedKeyCode = keyCode;

    TCHAR keyNameCStr[128];
    knceRealKeyCodeToName(keyNameCStr, 128, keyCode);

    HWND hKeyLabel = GetDlgItem(hDlg, IDC_KEY_LABEL);
    SetWindowText(hKeyLabel, keyNameCStr);

    HWND hOkButton = GetDlgItem(hDlg, IDOK);
    if (!IsWindowEnabled(hOkButton)) {
        EnableWindow(hOkButton, true);
        SetFocus(hOkButton);
    }
}
