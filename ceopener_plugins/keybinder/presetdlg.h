#ifndef PRESETDLG_H
#define PRESETDLG_H

#include <windows.h>

#include "BindItem.h"

class PresetDialogParams {
public:
    PresetDialogParams();
    virtual ~PresetDialogParams();
    BindItem *getBindItem() const { return m_bindItem; }
    void setBindItem(BindItem *item) { m_bindItem = item; }

private:
    BindItem *m_bindItem;
};

bool showPresetDialog(HWND hOwnerWindow, PresetDialogParams &params);

#endif  // PRESETDLG_H
