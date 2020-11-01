#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#include <vector>
#include <map>
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class Dictionary {
public:
    Dictionary();
    virtual ~Dictionary();
    bool setDictionaryFile(const std::tstring &fileName);
    bool find(std::vector<std::tstring> &cands, const std::tstring &key) const;

private:
    HANDLE m_hDictionaryFile;
    HANDLE m_hFileMapping;
    void *m_fileBuffer;
    std::map<TCHAR, long> m_captionOffsetTable;
};

#endif /* DICTIONARY_H_ */
