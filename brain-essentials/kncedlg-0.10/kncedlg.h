#ifndef KNCEDLG_H_
#define KNCEDLG_H_

#include <vector>
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

#ifdef KNCEDLG_EXPORTS
#define KNCEDLG_API __declspec(dllexport)
#else
#define KNCEDLG_API __declspec(dllimport)
#endif

struct KnceChooseDirectoryParams {
    TCHAR initialPath[MAX_PATH];
    TCHAR directory[MAX_PATH];
};

struct KnceChooseFileParams {
    bool isSaveFile;
    const TCHAR *filters;
    TCHAR initialPath[MAX_PATH];
    TCHAR fileName[MAX_PATH];
};

struct KnceChooseFontParams {
    HFONT hFont;
    int isFixedOnly;
};

struct KnceChooseApplicationParams {
    TCHAR title[256];
    TCHAR fileName[MAX_PATH];
};

extern "C" {

KNCEDLG_API int knceChooseDirectory(HWND hOwnerWindow,
    KnceChooseDirectoryParams *params);
KNCEDLG_API int knceChooseFile(HWND hOwnerWindow,
    KnceChooseFileParams *params);
KNCEDLG_API int knceChooseFont(HWND hOwnerWindow,
    KnceChooseFontParams *params);
KNCEDLG_API int knceCaptureKey(HWND hOwnerWindow, int *capturedKeyCode);
KNCEDLG_API int knceChooseApplication(HWND hOwnerWindow,
    KnceChooseApplicationParams *params);
KNCEDLG_API HWND knceCreateWaitBox(HWND hOwnerWindow, const TCHAR *msg);
KNCEDLG_API void knceDestroyWaitBox(HWND hWnd);

}

#endif /* KNCEDLG_H_ */
