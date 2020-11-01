#include "kncedlg.h"

#include <algorithm>
#include <map>

using namespace std;

enum {
    NWUS_KEYBD_REPEAT_CHANGED = 2 
};

extern "C" void WINAPI NotifyWinUserSystem(UINT uEvent);

bool showChooseDirectoryDialog(HWND hOwnerWindow,
    KnceChooseDirectoryParams *params);
bool showChooseFileDialog(HWND hOwnerWindow, KnceChooseFileParams *params);
bool showChooseFontDialog(HWND hOwnerWindow, KnceChooseFontParams *params);
bool showCaptureKeyDialog(HWND hOwnerWindow, int *capturedKeyCode);
bool showChooseApplicationDialog(HWND hOwnerWindow,
    KnceChooseApplicationParams *params);
void registerWaitBoxClass();
void unregisterWaitBoxClass();
HWND createWaitBox(HWND hOwnerWindow, const tstring &msg);
void destroyWaitBox(HWND hWnd);

HINSTANCE g_hInstance = NULL;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = (HINSTANCE)hInst;

        registerWaitBoxClass();
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
        unregisterWaitBoxClass();

    return true;
}

KNCEDLG_API int knceChooseDirectory(HWND hOwnerWindow,
    KnceChooseDirectoryParams *params) {

    return showChooseDirectoryDialog(hOwnerWindow, params);
}

KNCEDLG_API int knceChooseFile(HWND hOwnerWindow,
    KnceChooseFileParams *params) {

    return showChooseFileDialog(hOwnerWindow, params);
}

KNCEDLG_API int knceChooseFont(HWND hOwnerWindow,
    KnceChooseFontParams *params) {

    return showChooseFontDialog(hOwnerWindow, params);
}

KNCEDLG_API int knceCaptureKey(HWND hOwnerWindow,
    int *capturedKeyCode) {

    return showCaptureKeyDialog(hOwnerWindow, capturedKeyCode);
}

KNCEDLG_API int knceChooseApplication(HWND hOwnerWindow,
    KnceChooseApplicationParams *params) {

    return showChooseApplicationDialog(hOwnerWindow, params);
}

KNCEDLG_API HWND knceCreateWaitBox(HWND hOwnerWindow,
    const TCHAR *msg) {

    if (msg == NULL)
        return createWaitBox(hOwnerWindow, _T(""));
    else
        return createWaitBox(hOwnerWindow, msg);
}

KNCEDLG_API void knceDestroyWaitBox(HWND hWnd) {
    destroyWaitBox(hWnd);
}

static int CALLBACK enumFontsProc(const LOGFONT *lplf, const TEXTMETRIC *lptm,
    DWORD dwType, LPARAM lpData)
{
    vector<tstring> *names = (vector<tstring> *)lpData;

    if (lplf->lfFaceName[0] != _T('@'))
        names->push_back(lplf->lfFaceName);

    return 1;
}

static int CALLBACK enumFixedFontsProc(const LOGFONT *lplf,
    const TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData)
{
    vector<tstring> *names = (vector<tstring> *)lpData;

    if (lplf->lfFaceName[0] != _T('@') &&
        ((lplf->lfPitchAndFamily & 0x03) == FIXED_PITCH))
    {
        names->push_back(lplf->lfFaceName);
    }

    return 1;
}

static int CALLBACK enumVariableFontsProc(const LOGFONT *lplf,
    const TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData)
{
    vector<tstring> *names = (vector<tstring> *)lpData;

    if (lplf->lfFaceName[0] != _T('@') &&
        ((lplf->lfPitchAndFamily & 0x03) == VARIABLE_PITCH))
    {
        names->push_back(lplf->lfFaceName);
    }

    return 1;
}

static int CALLBACK checkFontProc(const LOGFONT *lplf, const TEXTMETRIC *lptm,
    DWORD dwType, LPARAM lpData)
{
    const tstring *name = (tstring *)lpData;
    if (tstring(lplf->lfFaceName) == *name)
        return 0;

    return 1;
}
