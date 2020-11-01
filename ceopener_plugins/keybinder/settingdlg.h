#ifndef SETTINGDLG_H
#define SETTINGDLG_H

#include <windows.h>

class SettingDialogParams {
public:
	SettingDialogParams();
	virtual ~SettingDialogParams();
	bool isDisableAtEd(void) const { return m_isDisableAtEd; }
	void setDisableAtEd(bool enabled) { m_isDisableAtEd = enabled; }
	bool isStoreData(void) const { return m_isStoreData; }
	void setStoreData(bool enabled) { m_isStoreData = enabled; }
private:
	bool m_isDisableAtEd;
	bool m_isStoreData;
};

bool showSettingDialog(HWND hOwnerWindow, SettingDialogParams &params);

#endif  // BINDDLG_H
