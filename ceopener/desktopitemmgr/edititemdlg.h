#ifndef EDITITEMDLG_H
#define EDITITEMDLG_H

#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class EditItemDialogParams {
public:
    EditItemDialogParams();
    virtual ~EditItemDialogParams();
    std::tstring getCaption() const { return m_caption; }
    void setCaption(std::tstring caption) { m_caption = caption; }
    std::tstring getApplicationPath() const { return m_applicationPath; }
    void setApplicationPath(std::tstring path) { m_applicationPath = path; }
    bool isBrowsingOnStartup() const { return m_browsingOnStartup; }
    void setBrowsingOnStartup(bool browsing)
        { m_browsingOnStartup = browsing; }

private:
    std::tstring m_caption;
    std::tstring m_applicationPath;
    bool m_browsingOnStartup;
};

bool showEditItemDialog(HWND hOwnerWindow, EditItemDialogParams &params);

#endif  // EDITITEMDLG_H
