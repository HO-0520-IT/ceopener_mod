#include "ctrldlg.h"

#include <string>
#include <windows.h>
#include <commctrl.h>
#include <knceutil.h>
#include <brainapi.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using namespace std;

enum {
    IDC_BRIGHT_LABEL = 102,
    IDC_BRIGHT_SLIDER = 103,
    IDC_CPU_LABEL = 112,
    IDC_CPU_SLIDER = 113,
    IDC_BACKLIGHT_PICTURE = 122,
    IDC_BACKLIGHT_DIM_ON_RADIO = 123,
    IDC_BACKLIGHT_DIM_OFF_RADIO = 124,
    IDC_POWER_DOWN_PICTURE = 132,
    IDC_POWER_DOWN_ON_RADIO = 133,
    IDC_POWER_DOWN_OFF_RADIO = 134,
    IDC_MR_SENSOR_PICTURE = 142,
    IDC_MR_SENSOR_ON_RADIO = 143,
    IDC_MR_SENSOR_OFF_RADIO = 144
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onHScroll(HWND hDlg, int code, HWND hScrollBar);
static void updateBrightnessLabel(HWND hDlg);
static void updateCpuLabel(HWND hDlg);
static void updateBrightness(int bright);

extern HINSTANCE g_hInstance;
extern HFONT g_hMainFont;
extern HANDLE g_hBacklightKeepEvent;

struct ControlDialogData {
    ControlDialogParams *dialogParams;
};

ControlDialogParams::ControlDialogParams() {
    m_brightness = 0;
    m_clockGear = 0;
    m_isBacklightDimmingEnabled = false;
    m_isPowerDownEnabled = false;
    m_isMrSensorEnabled = false;
}

ControlDialogParams::~ControlDialogParams() {
}

bool showControlDialog(HWND hOwnerWindow, ControlDialogParams &params) {
    int ret = DialogBoxParam(g_hInstance, _T("CONTROL"), hOwnerWindow,
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
        default:
            return false;
        }

        return true;
    }
    case WM_HSCROLL:
        onHScroll(hDlg, wParam, (HWND)lParam);
        return true;
	}

	return false;
}

static void onInitDialog(HWND hDlg, void *params) {
    ControlDialogData *data = new ControlDialogData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->dialogParams = (ControlDialogParams *)params;

    KnceUtil::adjustDialogLayout(hDlg);

    HWND hBrightSlider = GetDlgItem(hDlg, IDC_BRIGHT_SLIDER);
    SendMessage(hBrightSlider, TBM_SETRANGE, false, MAKELONG(1, 5));

    HWND hCpuSlider = GetDlgItem(hDlg, IDC_CPU_SLIDER);
    SendMessage(hCpuSlider, TBM_SETRANGE, false, MAKELONG(0, 3));

    SendMessage(hBrightSlider, TBM_SETPOS, true,
        data->dialogParams->getBrightness());

    SendMessage(hCpuSlider, TBM_SETPOS, true,
        3 - data->dialogParams->getClockGear());

    HWND hBacklightPicture = GetDlgItem(hDlg, IDC_BACKLIGHT_PICTURE);
    SendMessage(hBacklightPicture, STM_SETIMAGE, IMAGE_ICON,
        (LPARAM)LoadImage(g_hInstance, _T("LIGHT_DIM"), IMAGE_ICON,
        0, 0, 0));
    SetWindowPos(hBacklightPicture, NULL, 0, 0, 16, 16,
        SWP_NOZORDER | SWP_NOMOVE);

    HWND hPowerDownPicture = GetDlgItem(hDlg, IDC_POWER_DOWN_PICTURE);
    SendMessage(hPowerDownPicture, STM_SETIMAGE, IMAGE_ICON,
        (LPARAM)LoadImage(g_hInstance, _T("POWER_OFF"), IMAGE_ICON,
        0, 0, 0));
    SetWindowPos(hPowerDownPicture, NULL, 0, 0, 16, 16,
        SWP_NOZORDER | SWP_NOMOVE);

    HWND hMrSensorPicture = GetDlgItem(hDlg, IDC_MR_SENSOR_PICTURE);
    SendMessage(hMrSensorPicture, STM_SETIMAGE, IMAGE_ICON,
        (LPARAM)LoadImage(g_hInstance, _T("COVER_CLOSE"), IMAGE_ICON,
        0, 0, 0));
    SetWindowPos(hMrSensorPicture, NULL, 0, 0, 16, 16,
        SWP_NOZORDER | SWP_NOMOVE);

    if (data->dialogParams->isBacklightDimmingEnabled()) {
        HWND hBacklightOnRadio = GetDlgItem(hDlg, IDC_BACKLIGHT_DIM_ON_RADIO);
        SendMessage(hBacklightOnRadio, BM_SETCHECK, true, 0);
    }
    else {
        HWND hBacklightOffRadio = GetDlgItem(hDlg, IDC_BACKLIGHT_DIM_OFF_RADIO);
        SendMessage(hBacklightOffRadio, BM_SETCHECK, true, 0);
    }

    if (data->dialogParams->isPowerDownEnabled()) {
        HWND hPowerDownOnRadio = GetDlgItem(hDlg, IDC_POWER_DOWN_ON_RADIO);
        SendMessage(hPowerDownOnRadio, BM_SETCHECK, true, 0);
    }
    else {
        HWND hPowerDownOffRadio = GetDlgItem(hDlg, IDC_POWER_DOWN_OFF_RADIO);
        SendMessage(hPowerDownOffRadio, BM_SETCHECK, true, 0);
    }

    if (data->dialogParams->isMrSensorEnabled()) {
        HWND hMrSensorOnRadio = GetDlgItem(hDlg, IDC_MR_SENSOR_ON_RADIO);
        SendMessage(hMrSensorOnRadio, BM_SETCHECK, true, 0);
    }
    else {
        HWND hMrSensorOffRadio = GetDlgItem(hDlg, IDC_MR_SENSOR_OFF_RADIO);
        SendMessage(hMrSensorOffRadio, BM_SETCHECK, true, 0);
    }

    updateBrightnessLabel(hDlg);
    updateCpuLabel(hDlg);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    ControlDialogData *data =
        (ControlDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    HWND hBrightSlider = GetDlgItem(hDlg, IDC_BRIGHT_SLIDER);
    data->dialogParams->setBrightness(
        SendMessage(hBrightSlider, TBM_GETPOS, 0, 0));

    HWND hCpuSlider = GetDlgItem(hDlg, IDC_CPU_SLIDER);
    data->dialogParams->setClockGear(
        3 - SendMessage(hCpuSlider, TBM_GETPOS, 0, 0));

    HWND hBacklightOnRadio = GetDlgItem(hDlg, IDC_BACKLIGHT_DIM_ON_RADIO);
    data->dialogParams->setBacklightDimmingEnabled(
        SendMessage(hBacklightOnRadio, BM_GETCHECK, 0, 0) != 0);

    HWND hPowerDownOnRadio = GetDlgItem(hDlg, IDC_POWER_DOWN_ON_RADIO);
    data->dialogParams->setPowerDownEnabled(
        SendMessage(hPowerDownOnRadio, BM_GETCHECK, 0, 0) != 0);

    HWND hMrSensorOnRadio = GetDlgItem(hDlg, IDC_MR_SENSOR_ON_RADIO);
    data->dialogParams->setMrSensorEnabled(
        SendMessage(hMrSensorOnRadio, BM_GETCHECK, 0, 0) != 0);

    EndDialog(hDlg, IDOK);

    delete data;
}

static void onCancel(HWND hDlg) {
    ControlDialogData *data =
        (ControlDialogData *)GetWindowLong(hDlg, GWL_USERDATA);

    int bright = data->dialogParams->getBrightness();
    EdSetBacklightBright((bright - 1) * 25);

    EndDialog(hDlg, IDCANCEL);

    delete data;
}

static void onHScroll(HWND hDlg, int code, HWND hScrollBar) {
    int ctrlId = GetDlgCtrlID(hScrollBar);
    if (ctrlId == IDC_BRIGHT_SLIDER) {
        updateBrightness(SendMessage(hScrollBar, TBM_GETPOS, NULL, NULL));
        updateBrightnessLabel(hDlg);
    }
    else if (ctrlId == IDC_CPU_SLIDER)
        updateCpuLabel(hDlg);
}

static void updateBrightnessLabel(HWND hDlg) {
    HWND hBrightSlider = GetDlgItem(hDlg, IDC_BRIGHT_SLIDER);
    int pos = SendMessage(hBrightSlider, TBM_GETPOS, NULL, NULL);

    HWND hBrightLabel = GetDlgItem(hDlg, IDC_BRIGHT_LABEL);

    TCHAR posCStr[16];
    _sntprintf(posCStr, 16, _T("%d"), pos);
    SetWindowText(hBrightLabel, posCStr);
}

static void updateCpuLabel(HWND hDlg) {
    HWND hCpuSlider = GetDlgItem(hDlg, IDC_CPU_SLIDER);
    int pos = SendMessage(hCpuSlider, TBM_GETPOS, NULL, NULL);

    HWND hCpuLabel = GetDlgItem(hDlg, IDC_CPU_LABEL);

    switch (pos) {
    case 0:
        SetWindowText(hCpuLabel, _T("x 1/8"));
        break;
    case 1:
        SetWindowText(hCpuLabel, _T("x 1/4"));
        break;
    case 2:
        SetWindowText(hCpuLabel, _T("x 1/2"));
        break;
    case 3:
        SetWindowText(hCpuLabel, _T("x 1/1"));
        break;
    }
}

static void updateBrightness(int bright) {
    int edBright = 0;
    switch (bright) {
    case 1:
        edBright = 1;
        break;
    case 2:
        edBright = 25;
        break;
    case 3:
        edBright = 50;
        break;
    case 4:
        edBright = 75;
        break;
    case 5:
        edBright = 100;
        break;
    }

    EdSetBacklightBright(edBright);
}
