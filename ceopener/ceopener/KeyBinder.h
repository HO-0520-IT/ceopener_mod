#ifndef KEYBINDER_H
#define KEYBINDER_H

#include <vector>
#include <map>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

typedef bool (*KeyBinderCallback)(HWND hWnd, int id);

class KeyBinder {
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
        KeyBinderCallback getCallback() const { return m_callback; }
        void setCallback(KeyBinderCallback callback) { m_callback = callback; }
        bool getBindedFn() const { return m_bindedFn; }
        void setBindedFn(bool fn) { m_bindedFn = fn; }
        int getBindedKeyCode() const { return m_bindedKeyCode; }
        void setBindedKeyCode(int keyCode)
            { m_bindedKeyCode = keyCode; }
        bool isActive() const { return m_isActive; }
        void setActive(bool active) { m_isActive = active; }
        int getId() const { return m_id; }
        void setId(int id) { m_id = id; }

    private:
        std::tstring m_category;
        std::tstring m_name;
        bool m_isEnabled;
        bool m_defaultFn;
        int m_defaultKeyCode;
        KeyBinderCallback m_callback;
        bool m_bindedFn;
        int m_bindedKeyCode;
        bool m_isActive;
        int m_id;
    };

    KeyBinder();
    virtual ~KeyBinder();
    void setOwnerWindow(HWND hWnd) { m_hOwnerWindow = hWnd; }
    const std::map<int, Binding *> &getBindingTable()
        { return m_bindingTable; }
    int registerKeyBinding(const std::tstring &category,
        const std::tstring &name, bool enabled, bool defaultFn,
        unsigned int defaultKeyCode, KeyBinderCallback callback, bool active);
    void unregisterKeyBinding(int id);
    void enableKeyBinding(int id);
    void disableKeyBinding(int id);
    void bindKey(int id, bool fn, int keyCode);
    void activateKeyBinding(int id);
    void inactivateKeyBinding(int id);
    void suspend();
    void resume();
    void processHotKey(int atom);
    void loadProperties(const std::map<std::tstring, std::tstring> &props);
    void storeProperties(std::map<std::tstring, std::tstring> &props);
    void setDicKeyTables(const std::vector<int> &dicKeys,
        const std::vector<int> &fnDicKeys);

private:
    class Record {
    public:
        Record();
        ~Record();
        bool isEnabled() const { return m_isEnabled; }
        void setEnabled(bool enabled) { m_isEnabled = enabled; }
        bool getFn() const { return m_fn; }
        void setFn(bool fn) { m_fn = fn; }
        int getKeyCode() const { return m_keyCode; }
        void setKeyCode(int keyCode) { m_keyCode = keyCode; }

    private:
        bool m_isEnabled;
        bool m_fn;
        int m_keyCode;
    };

    void enableHotKey(int id);
    void disableHotKey(int id);
    int obtainNewId();
    HWND m_hOwnerWindow;
    std::map<int, Binding *> m_bindingTable;
    std::map<int, std::vector<Binding *> > m_atomToBinding;
    std::map<int, int> m_keyCodeToAtom;
    std::map<std::tstring, Record *> m_recordTable;
    bool m_isSuspended;
    std::vector<int> m_dicKeyTable;
    std::vector<int> m_fnDicKeyTable;
};

#endif  // KEYBINDER_H
