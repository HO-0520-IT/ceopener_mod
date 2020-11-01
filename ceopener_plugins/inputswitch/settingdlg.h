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
	enum {
	};

	SettingParams();
	virtual ~SettingParams();
	int getFontSize() const { return m_fontSize; }
	void setFontSize(int size) { m_fontSize = size; }
	bool isSubTouchUseEnabled() const { return m_isSubTouchUseEnabled; }
	void setSubTouchUseEnabled(bool enabled)
		{ m_isSubTouchUseEnabled = enabled; }
	bool isLargePad() const { return m_isLargePad; }
	void setLargePad(bool enabled)
		{ m_isLargePad = enabled; }
	bool isDisableAtEd() const { return m_isDisableAtEd; }
	void setDisableAtEd(bool enabled)
		{ m_isDisableAtEd = enabled; }
	bool isSwapHiphen() const { return m_isSwapHiphen; }
	void setSwapHiphen(bool enabled)
		{ m_isSwapHiphen = enabled; }

private:
	int m_fontSize;
	bool m_isSubTouchUseEnabled;
	bool m_isLargePad;
	bool m_isDisableAtEd;
	bool m_isSwapHiphen;
};

bool showSettingDialog(HWND hOwnerWindow, SettingParams &params);

#endif  // SETTINGDLG_H_
