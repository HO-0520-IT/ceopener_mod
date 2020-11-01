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
    IDC_CHOOSE_FONT_NAME_LIST = 102,
    IDC_CHOOSE_FONT_SIZE_LIST = 104,
    IDC_CHOOSE_FONT_STYLE_LIST = 106,
    IDC_CHOOSE_FONT_EXAMPLE_LABEL = 107,
};

static BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void onInitDialog(HWND hDlg, void *params);
static void onOk(HWND hDlg);
static void onCancel(HWND hDlg);
static void onNameList(HWND hDlg, int event);
static void onSizeList(HWND hDlg, int event);
static void onStyleList(HWND hDlg, int event);
static void updateFont(HWND hDlg);

extern HINSTANCE g_hInstance;

struct KnceFontDlgData {
    HFONT hFont;
    KnceChooseFontParams *chooseFontParams;
    HFONT hCurrentFont;
    bool isInitializing;
};

bool showChooseFontDialog(HWND hOwnerWindow, KnceChooseFontParams *params) {
    int ret = DialogBoxParam(g_hInstance, _T("CHOOSE_FONT"), hOwnerWindow,
        (DLGPROC)dlgProc, (LPARAM)params);

    return ret == IDOK;
}

BOOL WINAPI dlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
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
        case IDC_CHOOSE_FONT_NAME_LIST:
            onNameList(hDlg, event);
            break;
        case IDC_CHOOSE_FONT_SIZE_LIST:
            onSizeList(hDlg, event);
            break;
        case IDC_CHOOSE_FONT_STYLE_LIST:
            onStyleList(hDlg, event);
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
    static int fontSizes[] = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 26,
        28, 36, 48, 72};
    int i;

    KnceFontDlgData *data = new KnceFontDlgData();
    SetWindowLong(hDlg, GWL_USERDATA, (long)data);

    data->hFont = knceCreateDefaultFont(true, 100, false, false);
    knceSetDialogFont(hDlg, data->hFont);

    data->chooseFontParams = (KnceChooseFontParams *)params;

    HWND hFontNameList = GetDlgItem(hDlg, IDC_CHOOSE_FONT_NAME_LIST);

    KnceFontName *fontNames = NULL;
    int numFonts = knceObtainFontNames(&fontNames,
        data->chooseFontParams->isFixedOnly);

    for (i = 0; i < numFonts; i++) {
        SendMessage(hFontNameList, LB_ADDSTRING, 0,
            (LPARAM)fontNames[i].fontName);
    }

    free(fontNames);

    HWND hFontSizeList = GetDlgItem(hDlg, IDC_CHOOSE_FONT_SIZE_LIST);

    TCHAR sizeCStr[16];
    int numSizes = sizeof(fontSizes) / sizeof(int);

    for (i = 0; i < numSizes; i++) {
        _sntprintf(sizeCStr, 16, _T("%d"), fontSizes[i]);
        SendMessage(hFontSizeList, LB_ADDSTRING, 0, (LPARAM)sizeCStr);
    }

    HWND hFontStyleList = GetDlgItem(hDlg, IDC_CHOOSE_FONT_STYLE_LIST);
    SendMessage(hFontStyleList, LB_ADDSTRING, 0, (LPARAM)_T("Normal"));
    SendMessage(hFontStyleList, LB_ADDSTRING, 0, (LPARAM)_T("Italic"));
    SendMessage(hFontStyleList, LB_ADDSTRING, 0, (LPARAM)_T("Bold"));
    SendMessage(hFontStyleList, LB_ADDSTRING, 0, (LPARAM)_T("Bold Italic"));

    data->isInitializing = true;

    HFONT inputFont = data->chooseFontParams->hFont;
    if (inputFont == NULL)
        data->hCurrentFont = knceCreateDefaultFont(false, 120, false, false);
    else {
        LOGFONT logFont;
        GetObject(inputFont, sizeof(LOGFONT), &logFont);
        data->hCurrentFont = CreateFontIndirect(&logFont);
    }

    TCHAR fontNameCStr[256];
    int pointSize = 0;
    int isBold = 0;
    int isItalic = 0;

    knceGetFontAttributes(data->hCurrentFont, fontNameCStr, 256, &pointSize,
        &isBold, &isItalic);

    int itemIndex = SendMessage(hFontNameList, LB_FINDSTRINGEXACT, -1,
        (LPARAM)fontNameCStr);
    SendMessage(hFontNameList, LB_SETCURSEL, itemIndex, 0);

    _sntprintf(sizeCStr, 16, _T("%d"), pointSize / 10);
    itemIndex = SendMessage(hFontSizeList, LB_FINDSTRINGEXACT, -1,
        (LPARAM)sizeCStr);
    SendMessage(hFontSizeList, LB_SETCURSEL, itemIndex, 0);

    itemIndex = (isItalic ? 0x01 : 0x00) | (isBold ? 0x02 : 0x00);
    SendMessage(hFontStyleList, LB_SETCURSEL, itemIndex, 0);

    HWND hExampleLabel = GetDlgItem(hDlg, IDC_CHOOSE_FONT_EXAMPLE_LABEL);
    SendMessage(hExampleLabel, WM_SETFONT, (WPARAM)data->hCurrentFont, 0);

    data->isInitializing = false;

    updateFont(hDlg);

    ShowWindow(hDlg, SW_SHOW);
}

static void onOk(HWND hDlg) {
    KnceFontDlgData *data =
        (KnceFontDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    data->chooseFontParams->hFont = data->hCurrentFont;

    DeleteObject(data->hFont);

    EndDialog(hDlg, IDOK);
}

static void onCancel(HWND hDlg) {
    KnceFontDlgData *data =
        (KnceFontDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    DeleteObject(data->hCurrentFont);

    DeleteObject(data->hFont);

    EndDialog(hDlg, IDCANCEL);
}

static void onNameList(HWND hDlg, int event) {
    if (event == LBN_SELCHANGE)
        updateFont(hDlg);
}

static void onSizeList(HWND hDlg, int event) {
    if (event == LBN_SELCHANGE)
        updateFont(hDlg);
}

static void onStyleList(HWND hDlg, int event) {
    if (event == LBN_SELCHANGE)
        updateFont(hDlg);
}

static void updateFont(HWND hDlg) {
    KnceFontDlgData *data =
        (KnceFontDlgData *)GetWindowLong(hDlg, GWL_USERDATA);

    if (data->isInitializing)
        return;

    DeleteObject(data->hCurrentFont);

    HWND hFontNameList = GetDlgItem(hDlg, IDC_CHOOSE_FONT_NAME_LIST);
    int fontNameIndex = SendMessage(hFontNameList, LB_GETCURSEL, 0, 0);

    TCHAR *fontNameBuf = new TCHAR[SendMessage(hFontNameList, LB_GETTEXTLEN,
        fontNameIndex, 0) + 1];
    SendMessage(hFontNameList, LB_GETTEXT, fontNameIndex, (LPARAM)fontNameBuf);

    HWND hFontSizeList = GetDlgItem(hDlg, IDC_CHOOSE_FONT_SIZE_LIST);
    int fontSizeIndex = SendMessage(hFontSizeList, LB_GETCURSEL, 0, 0);

    TCHAR *sizeBuf = new TCHAR[SendMessage(hFontSizeList, LB_GETTEXTLEN,
        fontSizeIndex, 0) + 1];
    SendMessage(hFontSizeList, LB_GETTEXT, fontSizeIndex, (LPARAM)sizeBuf);
    int pointSize = _ttoi(sizeBuf) * 10;

    HWND hFontStyleList = GetDlgItem(hDlg, IDC_CHOOSE_FONT_STYLE_LIST);
    int fontStyleIndex = SendMessage(hFontStyleList, LB_GETCURSEL, 0, 0);
    bool isBold = (fontStyleIndex & 0x02) != 0;
    bool isItalic = (fontStyleIndex & 0x01) != 0;

    data->hCurrentFont = knceCreateFont(fontNameBuf, pointSize, isBold,
        isItalic);

    delete [] fontNameBuf;
    delete [] sizeBuf;

    HWND hExampleLabel = GetDlgItem(hDlg, IDC_CHOOSE_FONT_EXAMPLE_LABEL);
    SendMessage(hExampleLabel, WM_SETFONT, (WPARAM)data->hCurrentFont, 0);
}
