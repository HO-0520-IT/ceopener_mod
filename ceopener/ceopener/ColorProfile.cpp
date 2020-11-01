#include "ColorProfile.h"

#include <knceutil.h>

using namespace std;

ColorProfile::ColorProfile() {
}

ColorProfile::~ColorProfile() {
}

void ColorProfile::setDefaultProfile(const map<tstring, COLORREF> &table) {
    m_defaultColorTable = table;

    if (m_colorTable.empty())
        m_colorTable = table;
}

void ColorProfile::loadProfile(const std::tstring &name) {
    if (name.empty()) {
        m_colorTable = m_defaultColorTable;
        return;
    }

    tstring curDir = KnceUtil::getCurrentDirectory();

    tstring profileDir = curDir + _T("\\colors");
    tstring profileFileName = profileDir + _T("\\") + name;

    map<tstring, tstring> props;
    readPropertyFile(props, profileFileName);

    m_colorTable.clear();

    map<tstring, tstring>::const_iterator iter = props.begin();
    for ( ; iter != props.end(); iter++) {
        COLORREF color = 0;
        tstring colorStr = iter->second;

        if (colorStr.find(_T('#')) == 0)
            color = _tcstol(colorStr.c_str() + 1, NULL, 16);
        else
            color = _tcstol(colorStr.c_str(), NULL, 10);

        m_colorTable[iter->first] = ((color & 0xff)) << 16 | (color & 0xff00) |
            ((color & 0x00ff0000) >> 16);
    }
}

COLORREF ColorProfile::getColor(const std::tstring &name) {
    if (m_colorTable.find(name) == m_colorTable.end())
        return 0;
    else
        return m_colorTable[name];
}

void ColorProfile::readPropertyFile(map<tstring, tstring> &props,
    const tstring &fileName) {

    FILE *file = _tfopen(fileName.c_str(), _T("r"));

    TCHAR lineBuf[1024];
    char mbLineBuf[sizeof(lineBuf) * 2];

    while (true) {
        if (fgets(mbLineBuf, sizeof(mbLineBuf), file) == NULL)
            break;

        char *cr = strchr(mbLineBuf, '\n');
        if (cr != NULL)
            *cr = '\0';

        MultiByteToWideChar(932, 0, mbLineBuf, -1, lineBuf, sizeof(lineBuf));

        tstring line = lineBuf;
        int pos = line.find(_T('='));

        props[line.substr(0, pos)] = line.substr(pos + 1);
    }

    fclose(file);
}
