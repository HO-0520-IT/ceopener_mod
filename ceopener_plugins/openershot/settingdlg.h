#ifndef SETTINGDLG_H_
#define SETTINGDLG_H_

#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class SettingParams {
public:
    SettingParams();
    virtual ~SettingParams();
    bool getCaptureKeyFn() const { return m_captureKeyFn; }
    void setCaptureKeyFn(bool fn) { m_captureKeyFn = fn; }
    int getCaptureKeyCode() const { return m_captureKeyCode; }
    void setCaptureKeyCode(int code) { m_captureKeyCode = code; }
    std::tstring getStoreDirectory() const { return m_storeDirectory; }
    void setStoreDirectory(const std::tstring &dir) { m_storeDirectory = dir; }
    bool isNeedBegin() const { return m_isNeedBegin; }
    void setNeedBegin(bool isNeed) { m_isNeedBegin = isNeed; }

private:
    bool m_captureKeyFn;
    int m_captureKeyCode;
    std::tstring m_storeDirectory;
    bool m_isNeedBegin;
};

bool showSettingDialog(HWND hOwnerWindow, SettingParams &params);

#endif  // SETTINGDLG_H_
