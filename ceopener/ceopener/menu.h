#ifndef MENU_H
#define MENU_H

#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class MenuParams {
public:
    MenuParams();
    virtual ~MenuParams();
    bool isMain() const { return m_isMain; }
    void setMain(bool isMain) { m_isMain = isMain; }

private:
    bool m_isMain;
};

void registerMenuClass();
void unregisterMenuClass();
HWND createMenu(HWND hMainWindow, MenuParams &params);
void destroyMenu(HWND hWnd);
void showMenu(HWND hWnd);
void hideMenu(HWND hWnd);
void clearMenu(HWND hWnd);
int appendMenuItem(HWND hWnd, const std::tstring &capt, HWND hSubMenuWindow);
void removeMenuItem(HWND hWnd, int id);
void setMenuItemCaption(HWND hWnd, int id, const std::tstring &capt);
void hideMenuAscendant(HWND hWnd, bool isForced);
void setParentMenuWindow(HWND hWnd, HWND hParentMenuWindow);
void setMenuAttachedRect(HWND hWnd, const RECT *rect);

#endif  // MENU_H
