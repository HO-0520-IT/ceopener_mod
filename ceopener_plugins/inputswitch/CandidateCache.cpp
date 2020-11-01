#pragma warning(disable: 4503)  // decorated name too long

#include "CandidateCache.h"

using namespace std;

enum {
    MAX_CACHED_ENTRIES = 100
};

CandidateCache::CandidateCache() {
}

CandidateCache::~CandidateCache() {
}

bool CandidateCache::getCandidates(vector<tstring> &cands, const tstring &key)
    const {

    map<tstring, vector<tstring> >::const_iterator iter =
        m_cacheTable.find(key);
    if (iter == m_cacheTable.end())
        return false;

    cands = iter->second;

    return true;
}

void CandidateCache::updateCandidates(const tstring &key,
    const vector<tstring> &cands) {

    m_cacheOrder.remove(key);

    if (m_cacheOrder.size() >= MAX_CACHED_ENTRIES) {
        tstring removedKey = m_cacheOrder.back();
        m_cacheOrder.pop_back();
        m_cacheTable.erase(removedKey);
    }

    m_cacheOrder.push_front(key);
    m_cacheTable[key] = cands;
}
