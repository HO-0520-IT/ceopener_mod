#include "BindItem.h"

using namespace std;

BindItem::BindItem() {
    m_targetKeyFn = false;
    m_targetKeyCode = 0;
    m_actionType = 0;
    m_virtualKeyModifiers = 0;
    m_virtualKeyCode = 0;
    m_specialAction = 0;
}

BindItem::BindItem(const BindItem &other) {
    m_name = other.getName();
    m_targetKeyFn = other.getTargetKeyFn();
    m_targetKeyCode = other.getTargetKeyCode();
    m_actionType = other.getActionType();
    m_virtualKeyModifiers = other.getVirtualKeyModifiers();
    m_virtualKeyCode = other.getVirtualKeyCode();
    m_programFileName = other.getProgramFileName();
    m_specialAction = other.getSpecialAction();
}

BindItem::~BindItem() {
}
