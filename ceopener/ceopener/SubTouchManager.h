#ifndef SUBTOUCHMANAGER_H
#define SUBTOUCHMANAGER_H

#include <list>
#include <map>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

typedef void (*SubTouchCallback)(HWND hWnd, int id);
typedef void (*SubTouchDicKeyDownCallback)(HWND hWnd, int id, int dicKey);

class SubTouchManager {
public:
    enum {
        PRIORITY_LOWEST = -1,
        PRIORITY_NORMAL = 0,
        POWER_OFF = 1,
        POWER_ON = 2
    };

    SubTouchManager();
    virtual ~SubTouchManager();
    void setOwnerWindow(HWND hWnd) { m_hOwnerWindow = hWnd; }
    void init();
    int registerSubTouch(int priority, SubTouchCallback onActive,
        SubTouchCallback onInactive, SubTouchDicKeyDownCallback onDicKeyDown,
        bool enabled);
    void unregisterSubTouch(int id);
    void enableSubTouch(int id);
    void disableSubTouch(int id);
    void suspend();
    void resume();
    void processPowerOnOff(int status);
    void processDicKeyDown(int dicKey);

private:
    class SubTouch {
    public:
        SubTouch();
        virtual ~SubTouch();
        SubTouchCallback getOnActive() const { return m_onActive; }
        void setOnActive(SubTouchCallback onActive) { m_onActive = onActive; }
        SubTouchCallback getOnInactive() const { return m_onInactive; }
        void setOnInactive(SubTouchCallback onInactive)
            { m_onInactive = onInactive; }
        SubTouchDicKeyDownCallback getOnDicKeyDown() const
            { return m_onDicKeyDown; }
        void setOnDicKeyDown(SubTouchDicKeyDownCallback onDicKeyDown)
            { m_onDicKeyDown = onDicKeyDown; }
        bool isEnabled() const { return m_isEnabled; }
        void setEnabled(bool enabled) { m_isEnabled = enabled; }
        int getId() const { return m_id; }
        void setId(int id) { m_id = id; }

    private:
        bool m_isEnabled;
        SubTouchCallback m_onActive;
        SubTouchCallback m_onInactive;
        SubTouchDicKeyDownCallback m_onDicKeyDown;
        int m_id;
    };

    void enablePanelInput();
    void disablePanelInput();
    int obtainNewId();
    HWND m_hOwnerWindow;
    std::map<int, SubTouch *> m_subTouchTable;
    std::list<SubTouch *> m_priorityList;
    SubTouch* m_currentSubTouch;
    bool m_isSuspended;
    bool m_isSuspendedSaved;
    bool m_isPanelInputEnabled;
};

#endif  // SUBTOUCHMANAGER_H
