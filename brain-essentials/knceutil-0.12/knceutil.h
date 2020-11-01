#ifndef KNCEUTIL_H_
#define KNCEUTIL_H_

#include <vector>
#include <map>
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

enum {
    HARD_KEY_POWER = 0x11,
    HARD_KEY_WHOLESEARCH = 0x70,
    HARD_KEY_CONTENTS1 = 0x71,
    HARD_KEY_CONTENTS2 = 0x72,
    HARD_KEY_CONTENTS3 = 0x73,
    HARD_KEY_CONTENTS4 = 0x74,
    HARD_KEY_SWITCH = 0x75,
    HARD_KEY_BOOKMARKS = 0x76,
    HARD_KEY_HOME = 0x79,
    HARD_KEY_MENU = 0x24,
    HARD_KEY_HYPHEN = 0xbd,
    HARD_KEY_FUNCTION = 0x14,
    HARD_KEY_CLEAR = 0x23,
    HARD_KEY_BACKSPACE = 0x08,
    HARD_KEY_PAGEUP = 0x21,
    HARD_KEY_PAGEDOWN = 0x22,
    HARD_KEY_VOLUMEUP = 0xdb,
    HARD_KEY_VOLUMEDOWN = 0xdd,
    HARD_KEY_LARGER = 0xbe,
    HARD_KEY_SMALLER = 0xbc,
    HARD_KEY_VOICE = 0xdc,
    HARD_KEY_DESCRIPTION = 0xba,
    HARD_KEY_SJUMP = 0xbf,
    HARD_KEY_BACK = 0x1b,
    HARD_KEY_EXECUTE = 0x0d,
    HARD_KEY_LEFT = 0x25,
    HARD_KEY_UP = 0x26,
    HARD_KEY_RIGHT = 0x27,
    HARD_KEY_DOWN = 0x28,

    KNCE_FONT_PITCH_ALL = 0,
    KNCE_FONT_PITCH_FIXED = 1,
    KNCE_FONT_PITCH_VARIABLE = 2
};

class KnceUtil {
public:
    static std::tstring getCurrentDirectory();
	static std::tstring getFileName(LPCTSTR filePath);
    static int compareFileTime(bool &failed, const std::tstring &fileName1,
        const std::tstring &fileName2);

    static int registerHotKey(HWND hWnd, unsigned int mods,
        unsigned int keyCode);
    static bool unregisterHotKey(HWND hWnd, int id);
    static bool hardKeyCodeToName(std::tstring &name, int code);
    static int hardKeyNameToCode(const std::tstring &name);
    static void changeKeyRepeatSpeed(int initDelay, int repeatRate);
    static void restoreKeyRepeatSpeed();

    static HFONT createFont(const std::tstring &faceName, int pointSize,
        bool isBold = false, bool isItalic = false,
        int pitchType = KNCE_FONT_PITCH_ALL);
    static int obtainFontNames(std::vector<std::tstring> &fontNames,
        int pitchType = KNCE_FONT_PITCH_ALL);
    static bool fontExists(const std::tstring &name);
    static void getFontAttributes(HFONT hFont, std::tstring &faceName,
        int &pointSize, bool &isBold, bool &isItalic);

    static bool matchFileExtension(const std::tstring &fileName,
        const std::tstring &pat);
    static bool matchMultiFileExtension(const std::tstring &fileName,
        const std::tstring &pats);

    static void debugMessageBox(HWND hOwnerWindow, const TCHAR *msg,
        ...);

    static bool matchStringsIgnoreCase(const std::tstring &str1,
        const std::tstring &str2);
    static std::tstring toLowerString(const std::tstring &str);
    static void split(std::vector<std::tstring> &result,
        const std::tstring &str, const std::tstring &delim);

    static void centerWindow(HWND hWnd);
    static void adjustDialogLayout(HWND hDlg);
    static void dialogUnitsToPixels(int &xUnits, int &yUnits);

    static bool readPropertyFile(std::map<std::tstring, std::tstring> &props,
        const std::tstring &fileName);
    static bool writePropertyFile(const std::tstring &fileName,
        const std::map<std::tstring, std::tstring> &props);

private:
    static void createKeyCodeTable();
    static std::map<int, std::tstring> m_hardKeyCodeToNameTable;
    static std::map<std::tstring, int> m_hardKeyNameToCodeTable;
    static int m_savedKeyRepeatRefCount;
    static int m_savedKeyRepeatInitialDelay;
    static int m_savedKeyRepeatRate;
};

#endif /* KNCEUTIL_H_ */
