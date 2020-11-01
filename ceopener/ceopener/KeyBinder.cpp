#include "KeyBinder.h"

#include <algorithm>
#include <knceutil.h>
#include <brainapi.h>

using namespace std;

static unsigned int g_dicKeyDownMessage =
    RegisterWindowMessage(_T("DicKeyDown"));

KeyBinder::Binding::Binding() {
    m_isEnabled = false;
    m_defaultFn = false;
    m_defaultKeyCode = 0;
    m_callback = NULL;
    m_bindedFn = false;
    m_bindedKeyCode = 0;
    m_isActive = false;
    m_id = 0;
}

KeyBinder::Binding::~Binding() {
}

KeyBinder::Record::Record() {
    m_isEnabled = false;
    m_fn = false;
    m_keyCode = 0;
}

KeyBinder::Record::~Record() {
}

KeyBinder::KeyBinder() {
    m_hOwnerWindow = NULL;
    m_isSuspended = false;
}

KeyBinder::~KeyBinder() {
    map<int, Binding *>::iterator binditer = m_bindingTable.begin();
    for ( ; binditer != m_bindingTable.end(); binditer++) {
        inactivateKeyBinding(binditer->first);
        disableKeyBinding(binditer->first);

        delete binditer->second;
    }

    map<tstring, Record *>::iterator recordIter = m_recordTable.begin();
    for ( ; recordIter != m_recordTable.end(); recordIter++)
        delete recordIter->second;
}

int KeyBinder::registerKeyBinding(const tstring &category, const tstring &name,
    bool enabled, bool defaultFn, unsigned int defaultKeyCode,
    KeyBinderCallback callback, bool active) {

    Binding *bind = new Binding();
    bind->setCategory(category);
    bind->setName(name);
    bind->setDefaultFn(defaultFn);
    bind->setDefaultKeyCode(defaultKeyCode);
    bind->setCallback(callback);

    int id = obtainNewId();

    bind->setId(id);
    m_bindingTable[id] = bind;

    tstring recordKey = category + _T("\\") + name;
    if (m_recordTable.find(recordKey) == m_recordTable.end()) {
        if (enabled)
            enableKeyBinding(id);
    }
    else {
        Record *record = m_recordTable[recordKey];
        if (record->isEnabled())
            enableKeyBinding(id);

        bindKey(id, record->getFn(), record->getKeyCode());
    }

    if (active)
        activateKeyBinding(id);

    return id;
}

void KeyBinder::unregisterKeyBinding(int id) {
    if (m_bindingTable.find(id) == m_bindingTable.end())
        return;

    inactivateKeyBinding(id);
    disableKeyBinding(id);

    Binding *bind = m_bindingTable[id];
    delete bind;

    m_bindingTable.erase(id);
}

void KeyBinder::enableKeyBinding(int id) {
    if (m_bindingTable.find(id) == m_bindingTable.end())
        return;

    Binding *bind = m_bindingTable[id];
    if (!bind->isEnabled()) {
        if (bind->isActive())
            enableHotKey(id);

        bind->setEnabled(true);
    }
}

void KeyBinder::disableKeyBinding(int id) {
    if (m_bindingTable.find(id) == m_bindingTable.end())
        return;

    Binding *bind = m_bindingTable[id];
    if (bind->isEnabled()) {
        if (bind->isActive())
            disableHotKey(id);

        bind->setEnabled(false);
    }
}

void KeyBinder::bindKey(int id, bool fn, int keyCode) {
    if (m_bindingTable.find(id) == m_bindingTable.end())
        return;

    Binding *bind = m_bindingTable[id];
    if (bind->getBindedFn() == fn && bind->getBindedKeyCode() == keyCode)
        return;

    bool isActive = bind->isActive();
    if (isActive)
        inactivateKeyBinding(id);

    bind->setBindedFn(fn);
    bind->setBindedKeyCode(keyCode);

    if (isActive)
        activateKeyBinding(id);
}

void KeyBinder::activateKeyBinding(int id) {
    if (m_bindingTable.find(id) == m_bindingTable.end())
        return;

    Binding *bind = m_bindingTable[id];
    if (!bind->isActive()) {
        if (bind->isEnabled())
            enableHotKey(id);

        bind->setActive(true);
    }
}

void KeyBinder::inactivateKeyBinding(int id) {
    if (m_bindingTable.find(id) == m_bindingTable.end())
        return;

    Binding *bind = m_bindingTable[id];
    if (bind->isActive())
        if (bind->isEnabled()) {
            disableHotKey(id);

        bind->setActive(false);
    }
}

void KeyBinder::suspend() {
    if (m_isSuspended)
        return;

    map<int, Binding *>::iterator iter = m_bindingTable.begin();
    for ( ; iter != m_bindingTable.end(); iter++) {
        Binding *bind = iter->second;
        if (bind->isEnabled() && bind->isActive())
            disableHotKey(iter->first);
    }

    m_isSuspended = true;
}

void KeyBinder::resume() {
    if (!m_isSuspended)
        return;

    map<int, Binding *>::iterator iter = m_bindingTable.begin();
    for ( ; iter != m_bindingTable.end(); iter++) {
        Binding *bind = iter->second;
        if (bind->isEnabled() && bind->isActive())
            enableHotKey(iter->first);
    }

    m_isSuspended = false;
}

void KeyBinder::processHotKey(int atom) {
    int i;

    bool isFuncPressed = GetAsyncKeyState(HARD_KEY_FUNCTION) != 0;

    const vector<Binding *> &binds = m_atomToBinding[atom];
    int numBinds = binds.size();

    bool processed = false;
    for (i = 0; i < numBinds; i++) {
        Binding *bind = binds[i];
        bool fn = bind->getBindedKeyCode() == 0 ?
            bind->getDefaultFn() : bind->getBindedFn();

        if ((fn && isFuncPressed) || (!fn && !isFuncPressed)) {
            if (bind->getCallback() &&
                bind->getCallback()(m_hOwnerWindow, bind->getId())) {

                processed = true;
                break;
            }
        }
    }

    if (processed)
        return;

    UnregisterHotKey(m_hOwnerWindow, atom);

    int keyCode = 0;
    map<int, int>::const_iterator iter = m_keyCodeToAtom.begin();

    for ( ; iter != m_keyCodeToAtom.end(); iter++) {
        if (iter->second == atom)
            keyCode = iter->first;
    }

    HWND hEdWindow = FindWindow(_T("SHARP SIM"), NULL);
    if (hEdWindow != NULL && hEdWindow == GetForegroundWindow()) {
        if (!m_dicKeyTable.empty()) {
            int dicKey = isFuncPressed ? m_fnDicKeyTable[keyCode] :
                m_dicKeyTable[keyCode];
            SendMessage(hEdWindow, g_dicKeyDownMessage, dicKey, 0);
        }
    }
    else {
        keybd_event(keyCode, 0, 0, NULL);
        keybd_event(keyCode, 0, KEYEVENTF_KEYUP, NULL);
    }

    RegisterHotKey(m_hOwnerWindow, atom, 0, keyCode);
}

void KeyBinder::loadProperties(const map<tstring, tstring> &props) {
    int i;

    map<tstring, Record *>::iterator iter = m_recordTable.begin();
    for ( ; iter != m_recordTable.end(); iter++)
        delete iter->second;

    m_recordTable.clear();

    map<tstring, tstring> &propsRef = (map<tstring, tstring> &)props;

    TCHAR keyCStr[256];
    int numBinds = _ttoi(propsRef[_T("keyBinding.numBindings")].c_str());

    for (i = 0; i < numBinds; i++) {
        Record *record = new Record();

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.category"), i);
        tstring category = propsRef[keyCStr];

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.name"), i);
        tstring name = propsRef[keyCStr];

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.enabled"), i);
        record->setEnabled(_ttoi(propsRef[keyCStr].c_str()) != 0);

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.fn"), i);
        record->setFn(_ttoi(propsRef[keyCStr].c_str()) != 0);

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.keyCode"), i);
        record->setKeyCode(_ttoi(propsRef[keyCStr].c_str()));

        m_recordTable[category + _T("\\") + name] = record;
    }
}

void KeyBinder::storeProperties(map<tstring, tstring> &props) {
    TCHAR keyCStr[256];
    TCHAR valCStr[256];

    int index = 0;
    map<int, Binding *>::iterator iter = m_bindingTable.begin();

    for ( ; iter != m_bindingTable.end(); iter++) {
        Binding *bind = iter->second;
        if (bind->getCategory().empty())
            continue;

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.category"), index);
        props[keyCStr] = bind->getCategory();

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.name"), index);
        props[keyCStr] = bind->getName();

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.enabled"), index);
        _sntprintf(valCStr, 256, _T("%d"), bind->isEnabled());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.fn"), index);
        _sntprintf(valCStr, 256, _T("%d"), bind->getBindedFn());
        props[keyCStr] = valCStr;

        _sntprintf(keyCStr, 256, _T("keyBinding.binding.%d.keyCode"), index);
        _sntprintf(valCStr, 256, _T("%d"), bind->getBindedKeyCode());
        props[keyCStr] = valCStr;

        index++;
    }

    _sntprintf(valCStr, 256, _T("%d"), index);
    props[_T("keyBinding.numBindings")] = valCStr;
}

void KeyBinder::setDicKeyTables(const std::vector<int> &dicKeys,
    const std::vector<int> &fnDicKeys) {

    m_dicKeyTable.resize(256);
    copy(dicKeys.begin(), dicKeys.end(), m_dicKeyTable.begin());

    m_fnDicKeyTable.resize(256);
    copy(fnDicKeys.begin(), fnDicKeys.end(), m_fnDicKeyTable.begin());
}

void KeyBinder::enableHotKey(int id) {
    Binding *bind = m_bindingTable[id];

    int keyCode = bind->getBindedKeyCode();
    if (keyCode == 0)
        keyCode = bind->getDefaultKeyCode();

    if (m_keyCodeToAtom.find(keyCode) == m_keyCodeToAtom.end()) {
        int atom = KnceUtil::registerHotKey(m_hOwnerWindow, 0, keyCode);
        if (atom == 0)
            return;

        m_atomToBinding[atom] = vector<Binding *>(1, bind);
        m_keyCodeToAtom[keyCode] = atom;
    }
    else {
        int atom = m_keyCodeToAtom[keyCode];
        m_atomToBinding[atom].push_back(bind);
    }
}

void KeyBinder::disableHotKey(int id) {
    Binding *bind = m_bindingTable[id];

    int keyCode = bind->getBindedKeyCode();
    if (keyCode == 0)
        keyCode = bind->getDefaultKeyCode();

    int atom = m_keyCodeToAtom[keyCode];

    vector<Binding *> &binds = m_atomToBinding[atom];
    vector<Binding *>::iterator iter = binds.begin();

    for ( ; iter != binds.end(); iter++) {
        if (*iter == bind) {
            binds.erase(iter);
            break;
        }
    }

    if (binds.empty()) {
        m_atomToBinding.erase(atom);
        m_keyCodeToAtom.erase(keyCode);

        KnceUtil::unregisterHotKey(m_hOwnerWindow, atom);
    }
}

int KeyBinder::obtainNewId() {
    const int MIN_ID = 1;
    const int MAX_ID = MIN_ID + 1000;
    static int currentId = MIN_ID;

    while (true) {
        bool found = false;
        map<int, Binding *>::const_iterator iter = m_bindingTable.begin();

        for ( ; iter != m_bindingTable.end(); iter++) {
            if (iter->first == currentId) {
                found = true;
                break;
            }
        }

        if (!found)
            break;

        if (++currentId > MAX_ID)
            currentId = MIN_ID;
    }

    int retId = currentId;
    if (++currentId > MAX_ID)
        currentId = MIN_ID;

    return retId;
}
