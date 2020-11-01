#ifndef SPECIALICONDLG_H
#define SPECIALICONDLG_H

#include <windows.h>

class SpecialIconDialogParams {
public:
    enum {
        ICON_MY_DEVICE = 0,
        ICON_CONTROL_PANEL = 1,
        ICON_NAND3 = 2,
        ICON_STORAGE_CARD = 3
    };

    SpecialIconDialogParams();
    virtual ~SpecialIconDialogParams();
    int getIconType() const { return m_iconType; }
    void setIconType(int type) { m_iconType = type; }

private:
    int m_iconType;
};

bool showSpecialIconDialog(HWND hOwnerWindow, SpecialIconDialogParams &params);

#endif  // SPECIALICONDLG_H
