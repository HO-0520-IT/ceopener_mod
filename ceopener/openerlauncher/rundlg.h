#ifndef SETTINGDLG_H_
#define SETTINGDLG_H_

#include <vector>
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class RunParams {
public:
    RunParams();
    virtual ~RunParams();
    std::vector<std::tstring> &getProgramHistory() { return m_programHistory; }

private:
    std::vector<std::tstring> m_programHistory;
};

bool showRunDialog(HWND hOwnerWindow, RunParams &params);

#endif  // SETTINGDLG_H_
