#ifndef SETTINGDLG_H_
#define SETTINGDLG_H_

#include "transsetdlg.h"
#include "viewingdlg.h"
#include "sesetdlg.h"
#include <string>
#include <windows.h>


#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using std::tstring;

class SettingParams {
public:
	SettingParams();
	virtual ~SettingParams();
	ViewingParams &getViewingParams(void) { return *m_ViewingParams; }
	void setViewingParams(ViewingParams &params) { m_ViewingParams = &params; }
	TransSettingParams &getTransSettingParams(void) { return *m_TransSettingParams; }
	void setTransSettingParams(TransSettingParams &params) { m_TransSettingParams = &params; }
	SESettingParams &getSESettingParams(void) { return *m_SESettingParams; }
	void setSESettingParams(SESettingParams &params) { m_SESettingParams = &params; }

private:
	ViewingParams *m_ViewingParams;
	TransSettingParams *m_TransSettingParams;
	SESettingParams *m_SESettingParams;
};

bool showSettingDialog(HWND hOwnerWindow, SettingParams &params);

#endif  // SETTINGDLG_H_
