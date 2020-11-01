#ifndef SESETTINGDLG_H_
#define SESETTINGDLG_H_

#include "../SEController.h"
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using std::tstring;

#define SESETTING_READY WM_APP + 1

class SESettingParams {
public:
	SESettingParams();
	virtual ~SESettingParams();
	SEController &getSEController(void) { return *m_seController; }
	void setSEController(SEController &controller) { m_seController = &controller; }

private:
	SEController *m_seController;
};

HWND showSESettingDialog(HWND hOwnerWindow, SESettingParams &params);

#endif  // SESETTINGDLG_H_
