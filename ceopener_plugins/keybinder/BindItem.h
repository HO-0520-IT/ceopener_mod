#ifndef BINDITEM_H
#define BINDITEM_H

#include <string>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class BindItem {
public:
    enum {
        ACTION_VIRTUAL_KEY = 0,
        ACTION_PROGRAM = 1,
        ACTION_SPECIAL = 2,
        SPECIAL_ACTION_FOREGROUND_ED_WINDOW = 0x01,
        SPECIAL_ACTION_VOLUME_DOWN = 0x11,
        SPECIAL_ACTION_VOLUME_UP = 0x12,
    };

    BindItem();
    BindItem(const BindItem &other);
    virtual ~BindItem();
    std::tstring getName() const { return m_name; }
    void setName(const std::tstring &name) { m_name = name; }
    bool getTargetKeyFn() const { return m_targetKeyFn; }
    void setTargetKeyFn(bool fn) { m_targetKeyFn = fn; }
    int getTargetKeyCode() const { return m_targetKeyCode; }
    void setTargetKeyCode(int code) { m_targetKeyCode = code; }
    int getActionType() const { return m_actionType; }
    void setActionType(int type) { m_actionType = type; }
    int getVirtualKeyModifiers() const { return m_virtualKeyModifiers; }
    void setVirtualKeyModifiers(int mods) { m_virtualKeyModifiers = mods; }
    int getVirtualKeyCode() const { return m_virtualKeyCode; }
    void setVirtualKeyCode(int code) { m_virtualKeyCode = code; }
    std::tstring getProgramFileName() const { return m_programFileName; }
    void setProgramFileName(const std::tstring &fileName)
        { m_programFileName = fileName; }
    int getSpecialAction() const { return m_specialAction; }
    void setSpecialAction(int act) { m_specialAction = act; }

private:
    std::tstring m_name;
    bool m_targetKeyFn;
    int m_targetKeyCode;
    int m_actionType;
    int m_virtualKeyModifiers;
    int m_virtualKeyCode;
    std::tstring m_programFileName;
    int m_specialAction;
};

#endif  // BINDITEM_H

