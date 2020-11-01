#ifndef KEYBINDDLG_H_
#define KEYBINDDLG_H_

#include <vector>
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class KeyBindingDialogParams {
public:
    class Binding {
    public:
        Binding();
        virtual ~Binding();
        std::tstring getCategory() const { return m_category; }
        void setCategory(const std::tstring &category)
            { m_category = category; }
        std::tstring getName() const { return m_name; }
        void setName(const std::tstring &name) { m_name = name; }
        bool isEnabled() const { return m_isEnabled; }
        void setEnabled(bool enabled) { m_isEnabled = enabled; }
        bool getDefaultFn() const { return m_defaultFn; }
        void setDefaultFn(bool fn) { m_defaultFn = fn; }
        int getDefaultKeyCode() const { return m_defaultKeyCode; }
        void setDefaultKeyCode(int keyCode)
            { m_defaultKeyCode = keyCode; }
        bool getBindedFn() const { return m_bindedFn; }
        void setBindedFn(bool fn) { m_bindedFn = fn; }
        int getBindedKeyCode() const { return m_bindedKeyCode; }
        void setBindedKeyCode(int keyCode)
            { m_bindedKeyCode = keyCode; }

    private:
        std::tstring m_category;
        std::tstring m_name;
        bool m_isEnabled;
        bool m_defaultFn;
        int m_defaultKeyCode;
        bool m_bindedFn;
        int m_bindedKeyCode;
    };

    KeyBindingDialogParams();
    virtual ~KeyBindingDialogParams();
    std::vector<Binding *> &getBindings() { return m_bindings; }

private:
    std::vector<Binding *> m_bindings;
};

bool showKeyBindingDialog(HWND hOwnerWindow, KeyBindingDialogParams &params);

#endif  // KEYBINDDLG_H_
