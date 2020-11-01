#include "SubTouchManager.h"

#include <algorithm>
#include <knceutil.h>
#include <brainapi.h>

using namespace std;

static unsigned int g_dicKeyDownMessage =
    RegisterWindowMessage(_T("DicKeyDown"));

SubTouchManager::SubTouch::SubTouch() {
    m_isEnabled = false;
    m_onActive = NULL;
    m_onInactive = NULL;
    m_onDicKeyDown = NULL;
    m_id = 0;
}

SubTouchManager::SubTouch::~SubTouch() {
}

SubTouchManager::SubTouchManager() {
    m_hOwnerWindow = NULL;
    m_currentSubTouch = NULL;
    m_isSuspended = false;
    m_isSuspendedSaved = false;
    m_isPanelInputEnabled = false;
}

SubTouchManager::~SubTouchManager() {
    map<int, SubTouch *>::iterator iter = m_subTouchTable.begin();
    for ( ; iter != m_subTouchTable.end(); iter++)
        delete iter->second;
}

void SubTouchManager::init() {
    enablePanelInput();
}

int SubTouchManager::registerSubTouch(int priority, SubTouchCallback onActive,
    SubTouchCallback onInactive, SubTouchDicKeyDownCallback onDicKeyDown,
    bool enabled) {

    SubTouch *touch = new SubTouch();
    touch->setOnActive(onActive);
    touch->setOnInactive(onInactive);
    touch->setOnDicKeyDown(onDicKeyDown);

    int id = obtainNewId();

    touch->setId(id);
    m_subTouchTable[id] = touch;

    if (priority == PRIORITY_LOWEST)
        m_priorityList.push_front(touch);
    else
        m_priorityList.push_back(touch);

    if (enabled)
        enableSubTouch(id);

    return id;
}

void SubTouchManager::unregisterSubTouch(int id) {
    if (m_subTouchTable.find(id) == m_subTouchTable.end())
        return;

    disableSubTouch(id);

    SubTouch *touch = m_subTouchTable[id];

    list<SubTouch *>::iterator iter = find(m_priorityList.begin(),
        m_priorityList.end(), touch);
    m_priorityList.erase(iter);

    delete touch;

    m_subTouchTable.erase(id);
}

void SubTouchManager::enableSubTouch(int id) {
    if (m_subTouchTable.find(id) == m_subTouchTable.end())
        return;

    SubTouch *targetTouch = m_subTouchTable[id];
    if (targetTouch->isEnabled())
        return;

    bool needActivate = false;
    if (m_currentSubTouch == NULL)
        needActivate = true;
    else {
        int targetOrder = distance(m_priorityList.begin(),
            find(m_priorityList.begin(), m_priorityList.end(), targetTouch));
        int curOrder = distance(m_priorityList.begin(),
            find(m_priorityList.begin(), m_priorityList.end(),
            m_currentSubTouch));

        if (targetOrder > curOrder) {
            if (m_currentSubTouch->getOnInactive()) {
                m_currentSubTouch->getOnInactive()(m_hOwnerWindow,
                    m_currentSubTouch->getId());
            }

            needActivate = true;
        }
    }

    targetTouch->setEnabled(true);

    if (needActivate) {
        m_currentSubTouch = targetTouch;
        enablePanelInput();

        if (targetTouch->getOnActive())
            targetTouch->getOnActive()(m_hOwnerWindow, targetTouch->getId());
    }
}

void SubTouchManager::disableSubTouch(int id) {
    if (m_subTouchTable.find(id) == m_subTouchTable.end())
        return;

    SubTouch *targetTouch = m_subTouchTable[id];
    if (!targetTouch->isEnabled())
        return;

    if (targetTouch == m_currentSubTouch) {
        if (targetTouch->getOnInactive())
            targetTouch->getOnInactive()(m_hOwnerWindow, targetTouch->getId());

        SubTouch *candTouch = NULL;
        list<SubTouch *>::reverse_iterator riter =
            find(m_priorityList.rbegin(), m_priorityList.rend(), targetTouch);
        riter++;

        for ( ; riter != m_priorityList.rend(); riter++) {
            if ((*riter)->isEnabled()) {
                candTouch = *riter;
                break;
            }
        }

        if (candTouch == NULL) {
            m_currentSubTouch = NULL;
            disablePanelInput();
        }
        else {
            if (candTouch->getOnActive())
                candTouch->getOnActive()(m_hOwnerWindow, candTouch->getId());

            m_currentSubTouch = candTouch;
        }
    }

    targetTouch->setEnabled(false);
}

void SubTouchManager::suspend() {
    if (m_isSuspended)
        return;

    if (m_currentSubTouch != NULL) {
        if (m_currentSubTouch->getOnInactive()) {
            m_currentSubTouch->getOnInactive()(m_hOwnerWindow,
                m_currentSubTouch->getId());
        }
    }

    if (m_currentSubTouch != NULL)
        disablePanelInput();

    m_isSuspended = true;
}

void SubTouchManager::resume() {
    if (!m_isSuspended)
        return;

    if (m_currentSubTouch != NULL)
        enablePanelInput();

    if (m_currentSubTouch != NULL) {
        if (m_currentSubTouch->getOnActive()) {
            m_currentSubTouch->getOnActive()(m_hOwnerWindow,
                m_currentSubTouch->getId());
        }
    }

    m_isSuspended = false;
}

void SubTouchManager::processPowerOnOff(int status) {
    if (!brainApisEnabled())
        return;

    if (status == POWER_OFF) {
        m_isSuspendedSaved = m_isSuspended;

        if (!m_isSuspended)
            suspend();

        TPanel_PowerHandler(1);
    }
    else if (status == POWER_ON) {
        TPanel_PowerHandler(0);

        if (!m_isSuspendedSaved)
            resume();
    }
}

void SubTouchManager::processDicKeyDown(int dicKey) {
    if (m_isSuspended)
        return;

    if (m_currentSubTouch != NULL) {
        if (m_currentSubTouch->getOnDicKeyDown()) {
            m_currentSubTouch->getOnDicKeyDown()(m_hOwnerWindow,
                m_currentSubTouch->getId(), dicKey);
        }
    }
}

void SubTouchManager::enablePanelInput() {
    if (!brainApisEnabled())
        return;

    if (m_isPanelInputEnabled)
        return;

    SLCD_on_win();
    TPanel_Initialize_win(m_hOwnerWindow);

    m_isPanelInputEnabled = true;
}

void SubTouchManager::disablePanelInput() {
    if (!brainApisEnabled())
        return;

    if (!m_isPanelInputEnabled)
        return;

    SLCD_off_win();
    TPanel_Terminate_win();

    m_isPanelInputEnabled = false;
}

int SubTouchManager::obtainNewId() {
    int i;

    for (i = 1; i < 501; i++) {
        bool found = false;
        map<int, SubTouch *>::const_iterator iter = m_subTouchTable.begin();

        for ( ; iter != m_subTouchTable.end(); iter++) {
            if (iter->first == i) {
                found = true;
                break;
            }
        }

        if (!found)
            return i;
    }

    return 0;
}
