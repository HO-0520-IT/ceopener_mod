#include "kncecomm.h"

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

static int CALLBACK enumFontsProc(const LOGFONT *lplf, const TEXTMETRIC *lptm,
    DWORD dwType, LPARAM lpData);
static int CALLBACK enumFixedFontsProc(const LOGFONT *lplf,
    const TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData);
static int CALLBACK enumVariableFontsProc(const LOGFONT *lplf,
    const TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData);
static int CALLBACK checkFontProc(const LOGFONT *lplf, const TEXTMETRIC *lptm,
    DWORD dwType, LPARAM lpData);
static bool matchStringsIgnoreCase(const tstring &str1, const tstring &str2);
static std::tstring toLowerString(const std::tstring &str);
static void split(vector<tstring> &result, const tstring &str,
    const tstring &delim);

HINSTANCE g_hInstance = NULL;
static map<int, tstring> g_realKeyCodeToNameTable;
static map<tstring, int> g_realKeyNameToCodeTable;
static int g_savedKeyRepeatRefCount = 0;
static int g_savedKeyRepeatInitialDelay = 0;
static int g_savedKeyRepeatRate = 0;
static vector<tstring> g_localizedFixedFontNames;
static vector<tstring> g_localizedPropFontNames;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    int i;

    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = (HINSTANCE)hInst;

        registerWaitBoxClass();

        g_localizedFixedFontNames.push_back(_T("MS Gothic"));
        g_localizedFixedFontNames.push_back(_T("VL Gothic"));
        g_localizedFixedFontNames.push_back(_T("IPAGothic"));
        g_localizedFixedFontNames.push_back(_T("Sazanami Gothic"));
        g_localizedFixedFontNames.push_back(_T("Kochi Gothic"));
        g_localizedFixedFontNames.push_back(_T("mikachan"));

        g_localizedPropFontNames.push_back(_T("MS PGothic"));
        g_localizedPropFontNames.push_back(_T("VL PGothic"));
        g_localizedPropFontNames.push_back(_T("IPAPGothic"));
        g_localizedPropFontNames.push_back(_T("Sazanami Gothic"));
        g_localizedPropFontNames.push_back(_T("Kochi Gothic"));
        g_localizedPropFontNames.push_back(_T("mikachan-P"));
        g_localizedPropFontNames.push_back(_T("mikachan-PB"));
        g_localizedPropFontNames.push_back(_T("mikachan-PS"));

        static int realKeyCodes[] = {
            0x11, 0x70,
            0x71, 0x72, 0x73, 0x74,
            0x75, 0x76, 0x79, 0x24,
            0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55,
            0x49, 0x4f, 0x50,
            0x41, 0x53, 0x44, 0x46, 0x47, 0x48, 0x4a,
            0x4b, 0x4c,
            0x5a, 0x58, 0x43, 0x56, 0x42, 0x4e, 0x4d,
            0xbd,
            0x14, 0x23, 0x08,
            0x21, 0x22, 0xdb, 0xdd,
            0xbe, 0xbc,
            0xdc, 0xba, 0xbf,
            0x1b, 0x0d,
            0x25, 0x26, 0x27, 0x28
        };

        static TCHAR *realKeyCStrs[] = {
            _T("POWER"), _T("WHOLESEARCH"),
            _T("CONTENTS1"), _T("CONTENTS2"), _T("CONTENTS3"), _T("CONTENTS4"),
            _T("SWITCH"), _T("BOOKMARKS"), _T("LIBRARY"), _T("MENU"),
            _T("Q"), _T("W"), _T("E"), _T("R"), _T("T"), _T("Y"), _T("U"),
            _T("I"), _T("O"), _T("P"),
            _T("A"), _T("S"), _T("D"), _T("F"), _T("G"), _T("H"), _T("J"),
            _T("K"), _T("L"),
            _T("Z"), _T("X"), _T("C"), _T("V"), _T("B"), _T("N"), _T("M"),
            _T("HYPHEN"),
            _T("FUNCTION"), _T("CLEAR"), _T("BACKSPACE"),
            _T("PAGEUP"), _T("PAGEDOWN"), _T("VOLUMEUP"), _T("VOLUMEDOWN"),
            _T("LARGER"), _T("SMALLER"),
            _T("VOICE"), _T("DESCRIPTION"), _T("SJUMP"),
            _T("BACK"),_T("EXECUTE"),
            _T("LEFT"), _T("UP"), _T("RIGHT"), _T("DOWN")
        };

        int numRealKeys = sizeof(realKeyCodes) / sizeof(int);
        for (i = 0; i < numRealKeys; i++) {
            g_realKeyCodeToNameTable[realKeyCodes[i]] = realKeyCStrs[i];
            g_realKeyNameToCodeTable[realKeyCStrs[i]] = realKeyCodes[i];
        }
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
        unregisterWaitBoxClass();

    return true;
}

KNCECOMM_API int knceChooseDirectory(HWND hOwnerWindow,
    KnceChooseDirectoryParams *params) {

    return showChooseDirectoryDialog(hOwnerWindow, params);
}

KNCECOMM_API int knceChooseFile(HWND hOwnerWindow,
    KnceChooseFileParams *params) {

    return showChooseFileDialog(hOwnerWindow, params);
}

KNCECOMM_API int knceChooseFont(HWND hOwnerWindow,
    KnceChooseFontParams *params) {

    return showChooseFontDialog(hOwnerWindow, params);
}

KNCECOMM_API int knceCaptureKey(HWND hOwnerWindow,
    int *capturedKeyCode) {

    return showCaptureKeyDialog(hOwnerWindow, capturedKeyCode);
}

KNCECOMM_API int knceChooseApplication(HWND hOwnerWindow,
    KnceChooseApplicationParams *params) {

    return showChooseApplicationDialog(hOwnerWindow, params);
}

KNCECOMM_API HWND knceCreateWaitBox(HWND hOwnerWindow,
    const TCHAR *msg) {

    if (msg == NULL)
        return createWaitBox(hOwnerWindow, _T(""));
    else
        return createWaitBox(hOwnerWindow, msg);
}

KNCECOMM_API void knceDestroyWaitBox(HWND hWnd) {
    destroyWaitBox(hWnd);
}

KNCECOMM_API void knceGetCurrentDirectory(TCHAR *path, int pathSize) {
    TCHAR modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, MAX_PATH);
    
    TCHAR *p = _tcschr(modulePath, _T('\0'));
    for ( ; p >= modulePath; p--) {
        if (*p == _T('\\')) {
            *p = _T('\0');
            break;
        }
    }

    _tcsncpy(path, modulePath, pathSize);
}

KNCECOMM_API int knceCompareFileTime(int *failed, const TCHAR *fileName1,
    const TCHAR *fileName2) {

    failed = 0;

    WIN32_FIND_DATA fd1;
    HANDLE hFind = FindFirstFile(fileName1, &fd1);

    if (hFind == INVALID_HANDLE_VALUE) {
        *failed = 1;
        return 0;
    }

    FindClose(hFind);

    WIN32_FIND_DATA fd2;
    hFind = FindFirstFile(fileName2, &fd2);

    if (hFind == INVALID_HANDLE_VALUE) {
        *failed = 1;
        return 0;
    }

    int result = CompareFileTime(&fd1.ftLastWriteTime, &fd2.ftLastWriteTime);

    FindClose(hFind);

    return result;
}

KNCECOMM_API int knceRegisterHotKey(HWND hWnd, unsigned int mods,
    unsigned int keyCode)
{
    TCHAR hotKeyNameCStr[64];
    _sntprintf(hotKeyNameCStr, 64, _T("KNCECOMM_HOTKEY_%d_%d"), keyCode, mods);

    int hotKeyId = GlobalAddAtom(hotKeyNameCStr);
    if (!RegisterHotKey(hWnd, hotKeyId, mods, keyCode)) {
        GlobalDeleteAtom(hotKeyId);
        return 0;
    }

    return hotKeyId;
}

KNCECOMM_API int knceUnregisterHotKey(HWND hWnd, int id) {
    if (id == 0)
        return 1;

    int ret = UnregisterHotKey(hWnd, id) != 0;
    GlobalDeleteAtom(id);

    return ret;
}

KNCECOMM_API int knceRealKeyCodeToName(TCHAR *name, int nameSize, int code) {
    if (code < 0 || code > 255)
        return 0;

    map<int, tstring>::const_iterator iter =
        g_realKeyCodeToNameTable.find(code);
    if (iter == g_realKeyCodeToNameTable.end())
        _sntprintf(name, nameSize, _T("UNKNOWN%d"), code);
    else
        _tcsncpy(name, iter->second.c_str(), nameSize);

    return 1;
}

KNCECOMM_API int knceRealKeyNameToCode(const TCHAR *name) {
    map<tstring, int>::const_iterator iter =
        g_realKeyNameToCodeTable.find(name);
    if (iter == g_realKeyNameToCodeTable.end()) {
        int code = 0;
        if (_stscanf(name, _T("UNKNOWN%d"), &code) == 1) {
            if (code < 0 || code > 255)
                return -1;
            else
                return code;
        }
        else
            return -1;
    }
    else
        return iter->second;
}

KNCECOMM_API void knceChangeKeyRepeatSpeed(int initDelay, int repeatRate) {
    if (g_savedKeyRepeatRefCount++ > 0)
        return;

    HKEY hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\keybd"), 0,
        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    DWORD val = 0;
    DWORD size = sizeof(DWORD);
    RegQueryValueEx(hKey, _T("InitialDelay"), NULL, 0,
        (BYTE *)&val, &size);
    g_savedKeyRepeatInitialDelay = val;

    RegQueryValueEx(hKey, _T("RepeatRate"), NULL, 0, (BYTE *)&val, &size);
    g_savedKeyRepeatRate = val;

    val = (DWORD)initDelay;
    RegSetValueEx(hKey, _T("InitialDelay"), 0, REG_DWORD, (const BYTE *)&val,
        sizeof(DWORD));

    val = (unsigned long)repeatRate;
    RegSetValueEx(hKey, _T("RepeatRate"), 0, REG_DWORD, (const BYTE *)&val,
        sizeof(DWORD));

    RegCloseKey(hKey);

    NotifyWinUserSystem(NWUS_KEYBD_REPEAT_CHANGED);
}

KNCECOMM_API void knceRestoreKeyRepeatSpeed() {
    if (--g_savedKeyRepeatRefCount > 0)
        return;

    HKEY hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\keybd"), 0,
        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    DWORD val = 0;
    DWORD size = sizeof(DWORD);
    val = (DWORD)g_savedKeyRepeatInitialDelay;
    RegSetValueEx(hKey, _T("InitialDelay"), 0, REG_DWORD, (const BYTE *)&val,
        sizeof(DWORD));

    val = (DWORD)g_savedKeyRepeatRate;
    RegSetValueEx(hKey, _T("RepeatRate"), 0, REG_DWORD, (const BYTE *)&val,
        sizeof(DWORD));

    RegCloseKey(hKey);

    NotifyWinUserSystem(NWUS_KEYBD_REPEAT_CHANGED);
}

KNCECOMM_API HFONT knceCreateFont(const TCHAR *faceName, int pointSize,
    int isBold, int isItalic) {

    LOGFONT logFont = {0};

    _tcsncpy(logFont.lfFaceName, faceName, LF_FACESIZE);

    if (isBold)
        logFont.lfWeight = FW_BOLD;
    if (isItalic)
        logFont.lfItalic = true;

    HDC hDC = GetDC(NULL);
    logFont.lfHeight = -ceil(GetDeviceCaps(hDC, LOGPIXELSY) *
        pointSize / 720.0);
    ReleaseDC(NULL, hDC);

    logFont.lfCharSet = DEFAULT_CHARSET;

    return CreateFontIndirect(&logFont);
}

KNCECOMM_API void knceGetDefaultFontName(TCHAR *fontName, int fontNameLen) {
    tstring selectedFaceName;

    HKEY hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\knatech\\kncecomm"), 0,
        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    TCHAR defaultFontNameCStr[LF_FACESIZE];
    DWORD size = LF_FACESIZE;

    if (RegQueryValueEx(hKey, _T("DefaultFontName"), NULL, NULL,
        (BYTE *)defaultFontNameCStr, &size) == 0) {

        selectedFaceName = defaultFontNameCStr;
    }

    _tcsncpy(fontName, selectedFaceName.c_str(), fontNameLen);
}

KNCECOMM_API void knceSetDefaultFontName(const TCHAR *fontName) {
    HKEY hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\knatech\\kncecomm"), 0,
        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    RegSetValueEx(hKey, _T("DefaultFontName"), 0, REG_SZ,
        (const BYTE *)fontName, (_tcslen(fontName) + 1) * sizeof(TCHAR));

    RegCloseKey(hKey);
}

KNCECOMM_API HFONT knceCreateDefaultFont(int isVariable, int pointSize,
    int isBold, int isItalic) {

    int i, j;

    tstring selectedFaceName;

    HKEY hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\knatech\\kncecomm"), 0,
        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    TCHAR defaultFontNameCStr[LF_FACESIZE];
    DWORD size = LF_FACESIZE;

    if (RegQueryValueEx(hKey, _T("DefaultFontName"), NULL, NULL,
        (BYTE *)defaultFontNameCStr, &size) == 0) {

        selectedFaceName = defaultFontNameCStr;
    }

    RegCloseKey(hKey);

    if (selectedFaceName.empty()) {
        vector<tstring> fontNames;
        EnumFonts(GetDC(NULL), NULL, enumFontsProc, (LPARAM)&fontNames);
        int numFonts = fontNames.size();

        if (isVariable) {
            // try localized
            int numLocalized = g_localizedPropFontNames.size();
            for (i = 0; i < numLocalized; i++) {
                tstring testFontName = g_localizedPropFontNames[i];

                for (j = 0; j < numFonts; j++) {
                    if (fontNames[j] == testFontName) {
                        selectedFaceName = testFontName;
                        break;
                    }
                }

                if (!selectedFaceName.empty())
                    break;
            }

            // fallback to collected variable fonts
            if (selectedFaceName.empty()) {
                vector<tstring> variableFontNames;
                EnumFonts(GetDC(NULL), NULL, enumVariableFontsProc,
                    (LPARAM)&variableFontNames);

                if (!variableFontNames.empty())
                    selectedFaceName = variableFontNames[0];

                if (selectedFaceName == _T("Tahoma")) {
                    if (variableFontNames.size() > 1)
                        selectedFaceName = variableFontNames[1];
                    else
                        selectedFaceName = _T("");
                }
            }
        }
        else {
            // try localized
            int numLocalized = g_localizedFixedFontNames.size();
            for (i = 0; i < numLocalized; i++) {
                tstring testFontName = g_localizedFixedFontNames[i];

                for (j = 0; j < numFonts; j++) {
                    if (fontNames[j] == testFontName) {
                        selectedFaceName = testFontName;
                        break;
                    }
                }

                if (!selectedFaceName.empty())
                    break;
            }

            // fallback to collected fixed fonts
            if (selectedFaceName.empty()) {
                vector<tstring> fixedFontNames;
                EnumFonts(GetDC(NULL), NULL, enumFixedFontsProc,
                    (LPARAM)&fixedFontNames);

                if (!fixedFontNames.empty())
                    selectedFaceName = fixedFontNames[0];
            }
        }

        // finally, fallback to all collected fonts
        if (selectedFaceName.empty()) {
            if (numFonts > 0)
                selectedFaceName = fontNames[0];

            if (selectedFaceName == _T("Tahoma")) {
                if (numFonts > 1)
                    selectedFaceName = fontNames[1];
                else
                    selectedFaceName = _T("");
            }
        }
    }

    LOGFONT logFont = {0};

    if (!selectedFaceName.empty())
        _tcscpy(logFont.lfFaceName, selectedFaceName.c_str());

    logFont.lfPitchAndFamily = isVariable ? VARIABLE_PITCH : FIXED_PITCH;

    if (isBold)
        logFont.lfWeight = FW_BOLD;
    if (isItalic)
        logFont.lfItalic = true;

    HDC hDC = GetDC(NULL);
    logFont.lfHeight = -ceil(GetDeviceCaps(hDC, LOGPIXELSY) *
        pointSize / 720.0);
    ReleaseDC(NULL, hDC);

    logFont.lfCharSet = DEFAULT_CHARSET;

    return CreateFontIndirect(&logFont);
}

KNCECOMM_API int knceObtainFontNames(KnceFontName **fontNames, int pitchType) {
    int i;

    vector<tstring> names;
    switch (pitchType) {
    case KNCE_FONT_PITCH_FIXED:
        EnumFonts(GetDC(NULL), NULL, enumFixedFontsProc, (LPARAM)&names);
        break;
    case KNCE_FONT_PITCH_VARIABLE:
        EnumFonts(GetDC(NULL), NULL, enumVariableFontsProc, (LPARAM)&names);
        break;
    default:
        EnumFonts(GetDC(NULL), NULL, enumFontsProc, (LPARAM)&names);
        break;
    }

    int numNames = names.size();
    *fontNames = (KnceFontName *)malloc(numNames * sizeof(KnceFontName));

    for (i = 0; i < numNames; i++)
        _tcsncpy((*fontNames)[i].fontName, names[i].c_str(), LF_FACESIZE);

    return numNames;
}

KNCECOMM_API int knceFontExists(const TCHAR *name) {
    return EnumFonts(GetDC(NULL), NULL, checkFontProc,
        (LPARAM)&tstring(name)) == 0;
}

KNCECOMM_API void knceGetFontAttributes(HFONT hFont, TCHAR *faceName,
    int faceNameSize, int *pointSize, int *isBold, int *isItalic) {

    LOGFONT logFont;
    GetObject(hFont, sizeof(LOGFONT), &logFont);

    _tcsncpy(faceName, logFont.lfFaceName, faceNameSize);

    HDC hDC = GetDC(NULL);
    *pointSize = -ceil(logFont.lfHeight * 720.0 /
        GetDeviceCaps(hDC, LOGPIXELSY));
    ReleaseDC(NULL, hDC);

    *isItalic = logFont.lfItalic != 0 ? 1 : 0;
    *isBold = logFont.lfWeight >= FW_BOLD ? 1 : 0;
}

KNCECOMM_API void knceSetDialogFont(HWND hDlg, HFONT hFont) {
    SendMessage(hDlg, WM_SETFONT, (WPARAM)hFont, 0);

    HWND hChild = GetWindow(hDlg, GW_CHILD);
    if (hChild != NULL) {
        SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, 0);

        while (true) {
            hChild = GetWindow(hChild, GW_HWNDNEXT);
            if (hChild == NULL)
                break;

            SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, 0);
        }
    }
}

KNCECOMM_API int knceMatchFileExtension(const TCHAR *fileName,
    const TCHAR *pat) {

    tstring fileNameStr = fileName;
    tstring patStr = pat;

    if (patStr == _T("*.*"))
        return 1;

    if (patStr.length() < 2 || patStr.substr(0, 2) != _T("*."))
        return 0;

    int fileNameLen = fileNameStr.length();
    tstring ext = patStr.substr(1);
    int extLen = ext.length();

    if (fileNameLen < extLen)
        return 0;

    tstring fileExt = fileNameStr.substr(fileNameLen - extLen, extLen);
    if (!matchStringsIgnoreCase(fileExt, ext))
        return 0;

    return 1;
}

KNCECOMM_API int knceMatchMultiFileExtension(const TCHAR *fileName,
    const TCHAR *pats) {

    int i;

    vector<tstring> patList;
    split(patList, pats, _T(";"));

    int numPats = patList.size();
    for (i = 0; i < numPats; i++) {
        if (knceMatchFileExtension(fileName, patList[i].c_str()))
            return 1;
    }

    return 0;
}

KNCECOMM_API void knceDebugMessageBox(HWND hOwnerWindow,
    const TCHAR *msg, ...) {

    va_list args;

    va_start(args, msg);

    TCHAR msgCStr[256];
    _vsntprintf(msgCStr, 256, msg, args);

    MessageBox(hOwnerWindow, msgCStr, _T("Debug"),
        MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
    
    va_end(args);
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

static bool matchStringsIgnoreCase(const tstring &str1, const tstring &str2) {
    return toLowerString(str1) == toLowerString(str2);
}

static std::tstring toLowerString(const std::tstring &str) {
    tstring newStr = str;
    transform(newStr.begin(), newStr.end(), newStr.begin(), _totlower);

    return newStr;
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
