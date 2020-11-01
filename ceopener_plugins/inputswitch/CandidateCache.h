#ifndef CANDINATECACHE_H_
#define CANDINATECACHE_H_

#include <vector>
#include <list>
#include <map>
#include <string>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class CandidateCache {
public:
    CandidateCache();
    virtual ~CandidateCache();
    bool getCandidates(std::vector<std::tstring> &cands, const std::tstring &key)
        const;
    void updateCandidates(const std::tstring &key,
        const std::vector<std::tstring> &cands);

private:
    std::map<std::tstring, std::vector<std::tstring> > m_cacheTable;
    std::list<std::tstring> m_cacheOrder;
};

#endif /* CANDINATECACHE_H_ */
