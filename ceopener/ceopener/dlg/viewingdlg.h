#ifndef VIEWINGDLG_H_
#define VIEWINGDLG_H_

#include "transsetdlg.h"
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

using std::tstring;

#define VIEWING_READY WM_APP + 1

class ViewingParams {
public:
	ViewingParams();
	virtual ~ViewingParams();
	HFONT getFont() const { return m_hFont; }
	void setFont(HFONT hFont) { m_hFont = hFont; }
	std::tstring getColorProfileName() const { return m_colorProfileName; }
	void setColorProfileName(const std::tstring &name)
		{ m_colorProfileName = name; }
	std::tstring getWallpaperName() const { return m_wallpaperName; }
	void setWallpaperName(const std::tstring &name) { m_wallpaperName = name; }
	bool isShowPopupsTaskbar() const { return m_isShowPopupsTaskbar; }
	void setShowPopupsTaskbar(bool show) { m_isShowPopupsTaskbar = show; }
	bool isChangeTaskbarHeight() const { return m_isChangeTaskbarHeight; }
	void setChangeTaskbarHeight(bool change) { m_isChangeTaskbarHeight = change; }

private:
	HFONT m_hFont;
	std::tstring m_colorProfileName;
	std::tstring m_wallpaperName;
	bool m_isShowPopupsTaskbar;
	bool m_isChangeTaskbarHeight;
};

HWND showViewingDialog(HWND hOwnerWindow, ViewingParams &params);

#endif  // VIEWINGDLG_H_
