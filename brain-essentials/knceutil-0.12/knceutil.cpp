#include "knceutil.h"

#include <algorithm>

using namespace std;

enum {
    NWUS_KEYBD_REPEAT_CHANGED = 2 
};

extern "C" void WINAPI NotifyWinUserSystem(UINT uEvent);

static int CALLBACK enumFontsProc(const LOGFONT *lplf, const TEXTMETRIC *lptm,
    DWORD dwType, LPARAM lpData);
static int CALLBACK enumFixedFontsProc(const LOGFONT *lplf,
    const TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData);
static int CALLBACK enumVariableFontsProc(const LOGFONT *lplf,
    const TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData);
static int CALLBACK checkFontProc(const LOGFONT *lplf, const TEXTMETRIC *lptm,
    DWORD dwType, LPARAM lpData);

map<int, tstring> KnceUtil::m_hardKeyCodeToNameTable;
map<tstring, int> KnceUtil::m_hardKeyNameToCodeTable;
int KnceUtil::m_savedKeyRepeatRefCount = 0;
int KnceUtil::m_savedKeyRepeatInitialDelay = 0;
int KnceUtil::m_savedKeyRepeatRate = 0;

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

tstring KnceUtil::getCurrentDirectory() {
    TCHAR modulePath[MAX_PATH];
    GetModuleFileName(NULL, modulePath, MAX_PATH);
    
    TCHAR *p = _tcschr(modulePath, _T('\0'));
    for ( ; p >= modulePath; p--) {
        if (*p == _T('\\')) {
            *p = _T('\0');
            break;
        }
    }

    return modulePath;
}

tstring KnceUtil::getFileName(LPCTSTR filePath) {
    TCHAR fileName[MAX_PATH];
    
    TCHAR *p = _tcsrchr(filePath, _T('\\')) + 1;
    _tcscpy(fileName, p);
    TCHAR* q = _tcsrchr(fileName, _T('.'));
	*q = _T('\0');

    return fileName;
}

int KnceUtil::compareFileTime(bool &failed, const tstring &fileName1,
    const tstring &fileName2) {

    failed = false;

    WIN32_FIND_DATA fd1;
    HANDLE hFind = FindFirstFile(fileName1.c_str(), &fd1);

    if (hFind == INVALID_HANDLE_VALUE) {
        failed = true;
        return 0;
    }

    FindClose(hFind);

    WIN32_FIND_DATA fd2;
    hFind = FindFirstFile(fileName2.c_str(), &fd2);

    if (hFind == INVALID_HANDLE_VALUE) {
        failed = true;
        return 0;
    }

    int result = CompareFileTime(&fd1.ftLastWriteTime, &fd2.ftLastWriteTime);

    FindClose(hFind);

    return result;
}

int KnceUtil::registerHotKey(HWND hWnd, unsigned int mods,
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

bool KnceUtil::unregisterHotKey(HWND hWnd, int id) {
    if (id == 0)
        return true;

    bool ret = UnregisterHotKey(hWnd, id) != 0;
    GlobalDeleteAtom(id);

    return ret;
}

bool KnceUtil::hardKeyCodeToName(tstring &name, int code) {
    if (m_hardKeyCodeToNameTable.empty())
        createKeyCodeTable();

    if (code < 0 || code > 255)
        return false;

    map<int, tstring>::const_iterator iter =
        m_hardKeyCodeToNameTable.find(code);
    if (iter == m_hardKeyCodeToNameTable.end()) {
        TCHAR nameCStr[256];
        _sntprintf(nameCStr, 256, _T("UNKNOWN%d"), code);
        name = nameCStr;
    }
    else
        name = iter->second.c_str();

    return true;
}

int KnceUtil::hardKeyNameToCode(const tstring &name) {
    if (m_hardKeyNameToCodeTable.empty())
        createKeyCodeTable();

    map<tstring, int>::const_iterator iter =
        m_hardKeyNameToCodeTable.find(name);
    if (iter == m_hardKeyNameToCodeTable.end()) {
        int code = 0;
        if (_stscanf(name.c_str(), _T("UNKNOWN%d"), &code) == 1) {
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

void KnceUtil::changeKeyRepeatSpeed(int initDelay, int repeatRate) {
    if (m_savedKeyRepeatRefCount++ > 0)
        return;

    HKEY hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\keybd"), 0,
        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    DWORD val = 0;
    DWORD size = sizeof(DWORD);
    RegQueryValueEx(hKey, _T("InitialDelay"), NULL, 0,
        (BYTE *)&val, &size);
    m_savedKeyRepeatInitialDelay = val;

    RegQueryValueEx(hKey, _T("RepeatRate"), NULL, 0, (BYTE *)&val, &size);
    m_savedKeyRepeatRate = val;

    val = (DWORD)initDelay;
    RegSetValueEx(hKey, _T("InitialDelay"), 0, REG_DWORD, (const BYTE *)&val,
        sizeof(DWORD));

    val = (unsigned long)repeatRate;
    RegSetValueEx(hKey, _T("RepeatRate"), 0, REG_DWORD, (const BYTE *)&val,
        sizeof(DWORD));

    RegCloseKey(hKey);

    NotifyWinUserSystem(NWUS_KEYBD_REPEAT_CHANGED);
}

void KnceUtil::restoreKeyRepeatSpeed() {
    if (--m_savedKeyRepeatRefCount > 0)
        return;

    HKEY hKey = NULL;
    RegCreateKeyEx(HKEY_CURRENT_USER, _T("ControlPanel\\keybd"), 0,
        NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

    DWORD val = 0;
    DWORD size = sizeof(DWORD);
    val = (DWORD)m_savedKeyRepeatInitialDelay;
    RegSetValueEx(hKey, _T("InitialDelay"), 0, REG_DWORD, (const BYTE *)&val,
        sizeof(DWORD));

    val = (DWORD)m_savedKeyRepeatRate;
    RegSetValueEx(hKey, _T("RepeatRate"), 0, REG_DWORD, (const BYTE *)&val,
        sizeof(DWORD));

    RegCloseKey(hKey);

    NotifyWinUserSystem(NWUS_KEYBD_REPEAT_CHANGED);
}

HFONT KnceUtil::createFont(const tstring &faceName, int pointSize,
    bool isBold, bool isItalic, int pitchType) {

    LOGFONT logFont = {0};

    if (faceName.empty()) {
        vector<tstring> fontNames;
        obtainFontNames(fontNames, pitchType);
        if (!fontNames.empty())
            _tcsncpy(logFont.lfFaceName, fontNames[0].c_str(), LF_FACESIZE);
    }
    else
        _tcsncpy(logFont.lfFaceName, faceName.c_str(), LF_FACESIZE);

    if (isBold)
        logFont.lfWeight = FW_BOLD;
    if (isItalic)
        logFont.lfItalic = true;

    logFont.lfPitchAndFamily = pitchType;

    HDC hDC = GetDC(NULL);
    logFont.lfHeight = -ceil(GetDeviceCaps(hDC, LOGPIXELSY) *
        pointSize / 720.0);
    ReleaseDC(NULL, hDC);

    logFont.lfCharSet = DEFAULT_CHARSET;

    return CreateFontIndirect(&logFont);
}

int KnceUtil::obtainFontNames(vector<tstring> &fontNames, int pitchType) {
    switch (pitchType) {
    case KNCE_FONT_PITCH_FIXED:
        EnumFonts(GetDC(NULL), NULL, enumFixedFontsProc, (LPARAM)&fontNames);
        break;
    case KNCE_FONT_PITCH_VARIABLE:
        EnumFonts(GetDC(NULL), NULL, enumVariableFontsProc, (LPARAM)&fontNames);
        break;
    default:
        EnumFonts(GetDC(NULL), NULL, enumFontsProc, (LPARAM)&fontNames);
        break;
    }

    return fontNames.size();
}

bool KnceUtil::fontExists(const tstring &name) {
    return EnumFonts(GetDC(NULL), NULL, checkFontProc, (LPARAM)&name) == 0;
}

void KnceUtil::getFontAttributes(HFONT hFont, tstring &faceName,
    int &pointSize, bool &isBold, bool &isItalic) {

    LOGFONT logFont;
    GetObject(hFont, sizeof(LOGFONT), &logFont);

    faceName = logFont.lfFaceName;

    HDC hDC = GetDC(NULL);
    pointSize = -ceil(logFont.lfHeight * 720.0 /
        GetDeviceCaps(hDC, LOGPIXELSY));
    ReleaseDC(NULL, hDC);

    isItalic = logFont.lfItalic != 0;
    isBold = logFont.lfWeight >= FW_BOLD;
}

bool KnceUtil::matchFileExtension(const tstring &fileName,
    const tstring &pat) {

    if (pat == _T("*.*"))
        return true;

    if (pat.length() < 2 || pat.substr(0, 2) != _T("*."))
        return false;

    int fileNameLen = fileName.length();
    tstring ext = pat.substr(1);
    int extLen = ext.length();

    if (fileNameLen < extLen)
        return false;

    tstring fileExt = fileName.substr(fileNameLen - extLen, extLen);
    if (!matchStringsIgnoreCase(fileExt, ext))
        return false;

    return true;
}

bool KnceUtil::matchMultiFileExtension(const tstring &fileName,
    const tstring &pats) {

    int i;

    vector<tstring> patList;
    split(patList, pats, _T(";"));

    int numPats = patList.size();
    for (i = 0; i < numPats; i++) {
        if (matchFileExtension(fileName, patList[i].c_str()))
            return true;
    }

    return false;
}

void KnceUtil::debugMessageBox(HWND hOwnerWindow,
    const TCHAR *msg, ...) {

    va_list args;

    va_start(args, msg);

    TCHAR msgCStr[256];
    _vsntprintf(msgCStr, 256, msg, args);

    MessageBox(hOwnerWindow, msgCStr, _T("Debug"),
        MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
    
    va_end(args);
}

bool KnceUtil::matchStringsIgnoreCase(const tstring &str1, const tstring &str2) {
    return toLowerString(str1) == toLowerString(str2);
}

tstring KnceUtil::toLowerString(const tstring &str) {
    tstring newStr = str;
    transform(newStr.begin(), newStr.end(), newStr.begin(), _totlower);

    return newStr;
}

void KnceUtil::split(vector<tstring> &result, const tstring &str,
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

void KnceUtil::createKeyCodeTable() {
    int i;

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
        _T("電源"), _T("一括検索"),
        _T("コンテンツ1"), _T("コンテンツ2"), _T("コンテンツ3"), _T("コンテンツ4"),
        _T("切替"), _T("しおり"), _T("Home"), _T("辞書メニュー"),
        _T("Q"), _T("W"), _T("E"), _T("R"), _T("T"), _T("Y"), _T("U"),
        _T("I"), _T("O"), _T("P"),
        _T("A"), _T("S"), _T("D"), _T("F"), _T("G"), _T("H"), _T("J"),
        _T("K"), _T("L"),
        _T("Z"), _T("X"), _T("C"), _T("V"), _T("B"), _T("N"), _T("M"),
        _T("ハイフン"),
        _T("機能"), _T("クリア"), _T("後退"),
        _T("前見出"), _T("次見出"), _T("音量大"), _T("音量小"),
        _T("文字大"), _T("文字小"),
        _T("音声"), _T("例/解説"), _T("Sジャンプ"),
        _T("戻る"),_T("決定"),
        _T("左"), _T("上"), _T("右"), _T("下")
    };

    int numRealKeys = sizeof(realKeyCodes) / sizeof(int);
    for (i = 0; i < numRealKeys; i++) {
        m_hardKeyCodeToNameTable[realKeyCodes[i]] = realKeyCStrs[i];
        m_hardKeyNameToCodeTable[realKeyCStrs[i]] = realKeyCodes[i];
    }
}

void KnceUtil::centerWindow(HWND hWnd) {
    RECT workAreaRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);

    RECT windowRect = {0};
    GetWindowRect(hWnd, &windowRect);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    MoveWindow(hWnd, (workAreaRect.right - windowWidth) / 2,
        (workAreaRect.bottom - windowHeight) / 2, windowWidth, windowHeight,
        false);
}

void KnceUtil::adjustDialogLayout(HWND hDlg) {
    //long baseUnits = GetDialogBaseUnits();
    int baseX = 7;  //LOWORD(baseUnits);
    int baseY = 14; //HIWORD(baseUnits);

    RECT rect = {0, 0, 1000, 1000};
    MapDialogRect(hDlg, &rect);
    double scaleX = rect.right / 1000.0;
    double scaleY = rect.bottom / 1000.0;

    RECT clientRect;
    GetClientRect(hDlg, &clientRect);

    int newWidth = clientRect.right / scaleX / 4.0 * baseX;
    int newHeight = clientRect.bottom / scaleY / 8.0 * baseY;

    newWidth += GetSystemMetrics(SM_CXEDGE) +
        GetSystemMetrics(SM_CXBORDER) +
        GetSystemMetrics(SM_CXFIXEDFRAME);
    newHeight += GetSystemMetrics(SM_CYEDGE) +
        GetSystemMetrics(SM_CYBORDER) +
        GetSystemMetrics(SM_CYFIXEDFRAME) +
        GetSystemMetrics(SM_CYCAPTION);

    SetWindowPos(hDlg, NULL, 0, 0, newWidth, newHeight,
        SWP_NOMOVE | SWP_NOZORDER);

    TCHAR classNameCStr[256];
    RECT childRect;

    HWND hChild = GetWindow(hDlg, GW_CHILD);
    while (hChild != NULL) {
        GetClassName(hChild, classNameCStr, 256);
        if (_tcsicmp(classNameCStr, _T("combobox")) == 0)
            SendMessage(hChild, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&childRect);
        else
            GetWindowRect(hChild, &childRect);

        POINT pt = {childRect.left, childRect.top};
        ScreenToClient(hDlg, &pt);

        int newLeft = pt.x / scaleX / 4.0 * baseX;
        int newTop = pt.y / scaleY / 8.0 * baseY;
        newWidth = (childRect.right - childRect.left) /
            scaleX / 4.0 * baseX;
        newHeight = (childRect.bottom - childRect.top) /
            scaleY / 8.0 * baseY;

        SetWindowPos(hChild, NULL, newLeft, newTop, newWidth, newHeight,
            SWP_NOZORDER);

        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }

    if ((GetWindowLong(hDlg, GWL_STYLE) & DS_CENTER) != 0)
        centerWindow(hDlg);
}

void KnceUtil::dialogUnitsToPixels(int &xUnits, int &yUnits) {
    //long baseUnits = GetDialogBaseUnits();
    int baseX = 7;  //LOWORD(baseUnits);
    int baseY = 14; //HIWORD(baseUnits);

    xUnits = xUnits / 4.0 * baseX;
    yUnits = yUnits / 8.0 * baseY;
}

bool KnceUtil::readPropertyFile(map<tstring, tstring> &props,
    const tstring &fileName) {

    FILE *file = _tfopen(fileName.c_str(), _T("r"));
    if (file == NULL)
        return false;

    TCHAR lineBuf[1024];
    char mbLineBuf[sizeof(lineBuf) * 2];

    while (true) {
        if (fgets(mbLineBuf, sizeof(mbLineBuf), file) == NULL)
            break;

        char *cr = strchr(mbLineBuf, '\n');
        if (cr != NULL)
            *cr = '\0';

        MultiByteToWideChar(932, 0, mbLineBuf, -1, lineBuf, sizeof(lineBuf));

        tstring line = lineBuf;
        int pos = line.find(_T('='));

        props[line.substr(0, pos)] = line.substr(pos + 1);
    }

    fclose(file);

    return true;
}

bool KnceUtil::writePropertyFile(const tstring &fileName,
    const map<tstring, tstring> &props) {

    FILE *file = _tfopen(fileName.c_str(), _T("w"));
    if (file == NULL)
        return false;

    char mbLineBuf[1024 * 2];
    map<tstring, tstring>::const_iterator iter = props.begin();

    for ( ; iter != props.end(); iter++) {
        tstring line = iter->first + _T("=") + iter->second + _T("\n");

        WideCharToMultiByte(932, 0, line.c_str(), -1, mbLineBuf,
            sizeof(mbLineBuf), NULL, NULL);

        fputs(mbLineBuf, file);
    }

    fclose(file);

    return true;
}
