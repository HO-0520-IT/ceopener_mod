#include "Dictionary.h"

#include <locale.h>

using namespace std;

Dictionary::Dictionary() {
    m_hDictionaryFile = NULL;
}

Dictionary::~Dictionary() {
    if (m_hDictionaryFile != NULL) {
        UnmapViewOfFile(m_fileBuffer);
        CloseHandle(m_hFileMapping);

        CloseHandle(m_hDictionaryFile);
        m_hDictionaryFile = NULL;
    }
}

bool Dictionary::setDictionaryFile(const tstring &fileName) {
    if (m_hDictionaryFile != NULL) {
        UnmapViewOfFile(m_fileBuffer);
        CloseHandle(m_hFileMapping);

        CloseHandle(m_hDictionaryFile);
        m_hDictionaryFile = NULL;
    }

    m_hDictionaryFile = CreateFile(fileName.c_str(), GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (m_hDictionaryFile == INVALID_HANDLE_VALUE)
        return false;

    unsigned long numRead = 0;

    SetFilePointer(m_hDictionaryFile, 8, NULL, FILE_CURRENT);   // sig & ver
    SetFilePointer(m_hDictionaryFile, 4, NULL, FILE_CURRENT);   // area size

    m_captionOffsetTable.clear();

    TCHAR caption;
    while (true) {
        ReadFile(m_hDictionaryFile, &caption, sizeof(caption), &numRead, NULL);
        if ((short)caption == -1)
            break;

        long offset = 0;
        ReadFile(m_hDictionaryFile, &offset, 4, &numRead, NULL);

        m_captionOffsetTable[caption] = offset;
    }

    long keyAreaOffset = SetFilePointer(m_hDictionaryFile, 0, NULL,
        FILE_CURRENT);
    long keyAreaSize = 0;
    ReadFile(m_hDictionaryFile, &keyAreaSize, 4, &numRead, NULL);

    m_hFileMapping = CreateFileMapping(m_hDictionaryFile, NULL, PAGE_READONLY, 0,
        0, NULL);
    if (m_hFileMapping == NULL) {
        CloseHandle(m_hDictionaryFile);
        m_hDictionaryFile = NULL;

        return false;
    }

    m_fileBuffer = MapViewOfFile(m_hFileMapping, FILE_MAP_READ, 0, 0,
        keyAreaOffset + keyAreaSize);
    if (m_fileBuffer == NULL) {
        CloseHandle(m_hFileMapping);

        CloseHandle(m_hDictionaryFile);
        m_hDictionaryFile = NULL;

        return false;
    }

    return true;
}

bool Dictionary::find(vector<tstring> &cands, const tstring &key) const {
    const int maxLineSize = 1024;

    int i;

    if (m_hDictionaryFile == NULL)
        return false;

    if (key.empty())
        return false;

    cands.clear();

    TCHAR caption = key[0];
    if (m_captionOffsetTable.find(caption) == m_captionOffsetTable.end())
        return false;

    long captionOffset = m_captionOffsetTable.find(caption)->second;
    unsigned char *filePtr = (unsigned char *)m_fileBuffer + captionOffset;

    const TCHAR *keyCStr = key.c_str();

    // 2-bytes alignment
    long keyOffset = -1;
    while (true) {
        short keySize = *(short *)filePtr;
        if (keySize == -1)
            break;
        filePtr += 2;

        if (*(TCHAR *)filePtr != caption)
            break;

        if (_tcscmp(keyCStr, (TCHAR *)filePtr) == 0) {
            filePtr += keySize;
            keyOffset = (*((unsigned short *)filePtr + 1) << 16) |
                *(unsigned short *)filePtr;

            break;
        }
        else
            filePtr += keySize + 4;
    }

    if (keyOffset == -1)
        return false;

    SetFilePointer(m_hDictionaryFile, keyOffset, NULL, FILE_BEGIN);

    unsigned long numRead = 0;
    short candSize = 0;
    ReadFile(m_hDictionaryFile, &candSize, 2, &numRead, NULL);

    char mbCandBuff[maxLineSize * 2];
    ReadFile(m_hDictionaryFile, mbCandBuff, candSize, &numRead, NULL);

    TCHAR candBuff[maxLineSize];
    MultiByteToWideChar(CP_UTF8, 0, mbCandBuff, -1, candBuff, maxLineSize);

    tstring cand = candBuff;

    if (cand.empty())
        return false;

    tstring curCand;
    int len = cand.length();

    for (i = 0 ; i < len; i++) {
        TCHAR ch = cand[i];
        if (ch == _T('/')) {
            if (!curCand.empty())
                cands.push_back(curCand);

            curCand = _T("");
        }
        else if (ch != _T(' '))
            curCand += ch;
    }

    return !cands.empty();
}
