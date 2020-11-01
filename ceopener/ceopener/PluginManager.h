#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <vector>
#include <map>
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class PluginManager {
public:
    PluginManager();
    virtual ~PluginManager();
    void setMainWindow(HWND hMainWindow) { m_hMainWindow = hMainWindow; }
    void setDirectory(const std::tstring &dir) { m_directory = dir; }
    void addExcludedFileName(const std::tstring &fileName);
    void loadPlugin(const std::tstring &fileName);
    void unloadPlugin(const std::tstring &fileName);
    void loadAllPlugins();
    void unloadAllPlugins();

private:
    HMODULE loadPluginInternal(const std::tstring &fileName);
    void unloadPluginInternal(const std::tstring &fileName);
    HWND m_hMainWindow;
    std::tstring m_directory;
    std::vector<std::tstring> m_excludeFileNames;
    std::map<std::tstring, HMODULE> m_hPluginModules;
};

#endif  // PLUGINMANAGER_H
