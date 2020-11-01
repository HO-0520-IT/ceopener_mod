#include "PluginManager.h"

#include <algorithm>

#include <knceutil.h>

using namespace std;

struct PluginInfo {
};

struct PluginMenuItem;
struct PluginTrayItem;

typedef int (WINAPI *_pluginInit)(HWND hCont);
typedef void (WINAPI *_pluginTerminate)(HWND hCont);

PluginManager::PluginManager() {
    m_hMainWindow = NULL;
}

PluginManager::~PluginManager() {
}

void PluginManager::addExcludedFileName(const tstring &fileName) {
    m_excludeFileNames.push_back(fileName);
}

void PluginManager::loadPlugin(const tstring &fileName) {
    HMODULE hModule = loadPluginInternal(fileName);
    if (hModule == NULL)
        return;

    m_hPluginModules[fileName] = hModule;
}

void PluginManager::unloadPlugin(const tstring &fileName) {
    if (m_hPluginModules.find(fileName) == m_hPluginModules.end())
        return;

    unloadPluginInternal(fileName);

    m_hPluginModules.erase(fileName);
}

void PluginManager::loadAllPlugins() {
    tstring findPath = m_directory + _T("\\*.dll");

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(findPath.c_str(), &fd);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        tstring fileName = m_directory + _T("\\") + fd.cFileName;
        if (find(m_excludeFileNames.begin(), m_excludeFileNames.end(),
            fileName) == m_excludeFileNames.end()) {

            m_hPluginModules[fileName] = loadPluginInternal(fileName);
        }

        while (FindNextFile(hFind, &fd)) {
            tstring fileName = m_directory + _T("\\") + fd.cFileName;
            if (find(m_excludeFileNames.begin(), m_excludeFileNames.end(),
                fileName) == m_excludeFileNames.end()) {

                m_hPluginModules[fileName] = loadPluginInternal(fileName);
            }
        }
    }

    FindClose(hFind);
}

void PluginManager::unloadAllPlugins() {
    map<tstring, HMODULE>::iterator iter = m_hPluginModules.begin();
    for ( ; iter != m_hPluginModules.end(); iter++) {
        tstring fileName = iter->first;
        if (find(m_excludeFileNames.begin(), m_excludeFileNames.end(),
            fileName) == m_excludeFileNames.end()) {

            unloadPluginInternal(fileName);
        }
    }
	m_hPluginModules.clear();
}

extern FILE *g_logFile;
void writeLog(const tstring &line);

HMODULE PluginManager::loadPluginInternal(const tstring &fileName) {
    HMODULE hModule = LoadLibrary(fileName.c_str());
    if (hModule == NULL)
        return NULL;

    writeLog(_T("Load plugin: ") + fileName);

    bool funcLacked = false;
    funcLacked = funcLacked ||
        GetProcAddress(hModule, _T("pluginInit")) == NULL;
    funcLacked = funcLacked ||
        GetProcAddress(hModule, _T("pluginTerminate")) == NULL;

    if (funcLacked) {
        FreeLibrary(hModule);
        return NULL;
    }

    _pluginInit pluginInit = (_pluginInit)GetProcAddress(hModule,
        _T("pluginInit"));

	if(pluginInit != NULL){
		if (pluginInit(m_hMainWindow) == 0) {
			FreeLibrary(hModule);
			return NULL;
		}
	}

    return hModule;
}

void PluginManager::unloadPluginInternal(const tstring &fileName) {
    writeLog(_T("Unload plugin: ") + fileName);

    HMODULE hModule = m_hPluginModules[fileName];

    _pluginTerminate pluginTerminate = (_pluginTerminate)
        GetProcAddress(hModule, _T("pluginTerminate"));
	if(pluginTerminate!=NULL){
		pluginTerminate(m_hMainWindow);
	}

    FreeLibrary(hModule);
}
