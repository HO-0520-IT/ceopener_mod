#ifndef BINDDLG_H
#define BINDDLG_H

#include <vector>
#include <windows.h>

#include "BindItem.h"

class BindDialogParams {
public:
    BindDialogParams();
    virtual ~BindDialogParams();
    std::vector<BindItem *> &getBindItems() { return m_bindItems; }
	bool isDisableAtEd(void) const { return m_isDisableAtEd; }
	void setDisableAtEd(bool enabled) { m_isDisableAtEd = enabled; }
	bool isStoreData(void) const { return m_isStoreData; }
	void setStoreData(bool enabled) { m_isStoreData = enabled; }

private:
    std::vector<BindItem *> m_bindItems;
	bool m_isDisableAtEd;
	bool m_isStoreData;
};

bool showBindDialog(HWND hOwnerWindow, BindDialogParams &params);

#endif  // BINDDLG_H
