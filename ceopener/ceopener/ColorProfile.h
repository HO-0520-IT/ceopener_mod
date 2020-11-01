#ifndef COLORPROFILE_H
#define COLORPROFILE_H

#include <string>
#include <map>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class ColorProfile {
public:
    ColorProfile();
    virtual ~ColorProfile();
    void setDefaultProfile(const std::map<std::tstring, COLORREF> &table);
    void loadProfile(const std::tstring &fileName);
    COLORREF getColor(const std::tstring &name);

private:
    void readPropertyFile(std::map<std::tstring, std::tstring> &props,
        const std::tstring &fileName);
    std::map<std::tstring, COLORREF> m_defaultColorTable;
    std::map<std::tstring, COLORREF> m_colorTable;
};

#endif  // COLORPROFILE_H
