#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <windows.h>
#include <commctrl.h>
#include <knceutil.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_VOLUME_SLIDER = 101,
    IDC_VOLUME_LABEL = 102
};

void hidePanel(HWND hDlg);
static BOOL WINAPI settingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
    LPARAM lParam);
static void onInitDialog(HWND hDlg);
static void onDestroy(HWND hDlg);
static void onActivate(HWND hDlg, int active);
static void onVScroll(HWND hDlg, int code, HWND hScrollBar);
static void updateVolumeState(HWND hDlg);
static void changeWindowLayout(HWND hDlg);

extern HINSTANCE g_hInstance;
extern BOOL g_bOpening;

struct PanelData {
    vector<int> volumeLevels;
    RECT attachedRect;
};

HWND createPanel(HWND hOwnerWindow) {
    HWND hDlg = CreateDialogParam(g_hInstance, _T("PANEL"), hOwnerWindow,
        (DLGPROC)settingDlgProc, NULL);

    //ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    return hDlg;
}

void destroyPanel(HWND hDlg) {
    SendMessage(hDlg, WM_CLOSE, 0, 0);
}

void showPanel(HWND hDlg) {
    updateVolumeState(hDlg);

    changeWindowLayout(hDlg);

    ShowWindow(hDlg, SW_SHOW);

    HWND hSlider = GetDlgItem(hDlg, IDC_VOLUME_SLIDER);
    SetFocus(hSlider);
}

void hidePanel(HWND hDlg) {
    ShowWindow(hDlg, SW_HIDE);
}

void setPanelAttachedRect(HWND hDlg, const RECT *rect) {
    PanelData *data = (PanelData *)GetWindowLong(hDlg, GWL_USERDATA);

    data->attachedRect = *rect;
}

void slidePanelVolumePosition(HWND hDlg, int offset) {
    HWND hSlider = GetDlgItem(hDlg, IDC_VOLUME_SLIDER);
    int pos = SendMessage(hSlider, TBM_GETPOS, NULL, NULL);

    if (offset > 0) {
        if (pos > 0)
            pos--;
    }
    else if (offset < 0) {
        if (pos < 9)
            pos++;
    }

    SendMessage(hSlider, TBM_SETPOS, true, pos);

    onVScroll(hDlg, 0, hSlider);
}

static BOOL WINAPI settingDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
	switch (msg) {
    case WM_INITDIALOG:
        onInitDialog(hDlg);
        return true;
    case WM_DESTROY:
        onDestroy(hDlg);
        return true;
    case WM_COMMAND:
    {
        int id    = LOWORD(wParam); 
        int event = HIWORD(wParam);

        switch (id) {
        case IDOK:
        case IDCANCEL:
            DestroyWindow(hDlg);
            break;
        default:
            return false;
        }

        return true;
    }
    case WM_ACTIVATE:
        onActivate(hDlg, wParam & 0xFFFF);
        return true;
    case WM_VSCROLL:
        onVScroll(hDlg, wParam, (HWND)lParam);
        return true;
	}

	return false;
}

static void onInitDialog(HWND hDlg) {
    int i;

    PanelData *data = new PanelData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    KnceUtil::adjustDialogLayout(hDlg);

    int vols[] = {0x0421, 0x35ad, 0x6739, 0x8841, 0xa107, 0xb9cd, 0xca51,
        0xdad5, 0xeb59, 0xfbdd};
    for (i = 0; i < 10; i++)
        data->volumeLevels.push_back(vols[i]);

    updateVolumeState(hDlg);

    changeWindowLayout(hDlg);
}

static void onDestroy(HWND hDlg) {
    PanelData *data = (PanelData *)GetWindowLong(hDlg, GWL_USERDATA);

    delete data;
}

static void onActivate(HWND hDlg, int active) {
	if (active == WA_INACTIVE){
		hidePanel(hDlg);
		g_bOpening = FALSE;
	}
}

static void onVScroll(HWND hDlg, int code, HWND hScrollBar) {
    PanelData *data = (PanelData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hSlider = GetDlgItem(hDlg, IDC_VOLUME_SLIDER);

    if (hScrollBar == hSlider) {
        int pos = SendMessage(hSlider, TBM_GETPOS, NULL, NULL);
        int level = 9 - pos;

        DWORD vol = data->volumeLevels[level];
        vol |= vol << 16;
        waveOutSetVolume(NULL, vol);

        MessageBeep(-1);

        updateVolumeState(hDlg);
    }
}

static void updateVolumeState(HWND hDlg) {
    PanelData *data = (PanelData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hSlider = GetDlgItem(hDlg, IDC_VOLUME_SLIDER);
    SendMessage(hSlider, TBM_SETRANGE, false, MAKELONG(0, 9));

    DWORD vol = 0;
    waveOutGetVolume(NULL, &vol);
    vol &= 0xffff;

    vector<int>::iterator iter = find_if(data->volumeLevels.begin(),
        data->volumeLevels.end(), bind2nd(greater<int>(), vol));
    int level = distance(data->volumeLevels.begin(), iter) - 1;

    int pos = 9 - level;
    SendMessage(hSlider, TBM_SETPOS, true, pos);

    HWND hLabel = GetDlgItem(hDlg, IDC_VOLUME_LABEL);

    TCHAR posCStr[16];
    _sntprintf(posCStr, 16, _T("%d"), level);
    SetWindowText(hLabel, posCStr);
}

static void changeWindowLayout(HWND hDlg) {
    PanelData *data = (PanelData *)GetWindowLong(hDlg, GWL_USERDATA);

    int baseLeft = (data->attachedRect.left + data->attachedRect.right) / 2;
    int baseTop = data->attachedRect.top;

    RECT windowRect;
    GetClientRect(hDlg, &windowRect);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    SetWindowPos(hDlg, NULL, baseLeft - windowWidth / 2,
        baseTop - windowHeight, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
