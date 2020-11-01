#ifndef KNCECOMM_H_
#define KNCECOMM_H_

#include <vector>
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

#ifdef KNCECOMM_EXPORTS
#define KNCECOMM_API __declspec(dllexport)
#else
#define KNCECOMM_API __declspec(dllimport)
#endif

#define HARD_KEY_POWER 0x11
#define HARD_KEY_WHOLESEARCH 0x70
#define HARD_KEY_CONTENTS1 0x71
#define HARD_KEY_CONTENTS2 0x72
#define HARD_KEY_CONTENTS3 0x73
#define HARD_KEY_CONTENTS4 0x74
#define HARD_KEY_SWITCH 0x75
#define HARD_KEY_BOOKMARKS 0x76
#define HARD_KEY_HOME 0x79
#define HARD_KEY_MENU 0x24
#define HARD_KEY_HYPHEN 0xbd
#define HARD_KEY_FUNCTION 0x14
#define HARD_KEY_CLEAR 0x23
#define HARD_KEY_BACKSPACE 0x08
#define HARD_KEY_PAGEUP 0x21
#define HARD_KEY_PAGEDOWN 0x22
#define HARD_KEY_VOLUMEUP 0xdb
#define HARD_KEY_VOLUMEDOWN 0xdd
#define HARD_KEY_LARGER 0xbe
#define HARD_KEY_SMALLER 0xbc
#define HARD_KEY_VOICE 0xdc
#define HARD_KEY_DESCRIPTION 0xba
#define HARD_KEY_SJUMP 0xbf
#define HARD_KEY_BACK 0x1b
#define HARD_KEY_EXECUTE 0x0d
#define HARD_KEY_LEFT 0x25
#define HARD_KEY_UP 0x26
#define HARD_KEY_RIGHT 0x27
#define HARD_KEY_DOWN 0x28

#define KNCE_FONT_PITCH_ALL 0
#define KNCE_FONT_PITCH_FIXED 1
#define KNCE_FONT_PITCH_VARIABLE 2

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

struct KnceFontName {
    TCHAR fontName[LF_FACESIZE];
};

extern "C" {

KNCECOMM_API int knceChooseDirectory(HWND hOwnerWindow,
    KnceChooseDirectoryParams *params);
KNCECOMM_API int knceChooseFile(HWND hOwnerWindow,
    KnceChooseFileParams *params);
KNCECOMM_API int knceChooseFont(HWND hOwnerWindow,
    KnceChooseFontParams *params);
KNCECOMM_API int knceCaptureKey(HWND hOwnerWindow, int *capturedKeyCode);
KNCECOMM_API int knceChooseApplication(HWND hOwnerWindow,
    KnceChooseApplicationParams *params);
KNCECOMM_API HWND knceCreateWaitBox(HWND hOwnerWindow, const TCHAR *msg);
KNCECOMM_API void knceDestroyWaitBox(HWND hWnd);

KNCECOMM_API void knceGetCurrentDirectory(TCHAR *path, int pathSize);
KNCECOMM_API int knceCompareFileTime(int *failed, const TCHAR *fileName1,
    const TCHAR *fileName2);

KNCECOMM_API int knceRegisterHotKey(HWND hWnd, unsigned int mods,
    unsigned int keyCode);
KNCECOMM_API int knceUnregisterHotKey(HWND hWnd, int id);
KNCECOMM_API int knceRealKeyCodeToName(TCHAR *name, int nameSize, int code);
KNCECOMM_API int knceRealKeyNameToCode(const TCHAR *name);
KNCECOMM_API void knceChangeKeyRepeatSpeed(int initDelay, int repeatRate);
KNCECOMM_API void knceRestoreKeyRepeatSpeed();

KNCECOMM_API HFONT knceCreateFont(const TCHAR *faceName,
    int pointSize, int isBold, int isItalic);
KNCECOMM_API void knceGetDefaultFontName(TCHAR *fontName, int fontNameLen);
KNCECOMM_API void knceSetDefaultFontName(const TCHAR *fontName);
KNCECOMM_API HFONT knceCreateDefaultFont(int isVariable,
    int pointSize, int isBold, int isItalic);
KNCECOMM_API int knceObtainFontNames(KnceFontName **fontNames, int pitchType);
KNCECOMM_API int knceFontExists(const TCHAR *name);
KNCECOMM_API void knceGetFontAttributes(HFONT hFont, TCHAR *faceName,
    int faceNameSize, int *pointSize, int *isBold, int *isItalic);
KNCECOMM_API void knceSetDialogFont(HWND hDlg, HFONT hFont);

KNCECOMM_API int knceMatchFileExtension(const TCHAR *fileName,
    const TCHAR *pat);
KNCECOMM_API int knceMatchMultiFileExtension(const TCHAR *fileName,
    const TCHAR *pats);

KNCECOMM_API void knceDebugMessageBox(HWND hOwnerWindow,
    const TCHAR *msg, ...);

}

#endif /* KNCECOMM_H_ */
