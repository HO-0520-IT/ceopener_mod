#include "CandidateController.h"

using namespace std;

CandidateController::PageSpec::PageSpec() {
	m_startIndex = 0;
	m_size = 0;
}

CandidateController::PageSpec::~PageSpec() {
}

CandidateController::CandidateController() {
	m_characterWidth = 0;
	m_characterHeight = 0;
	m_isSeparated = false;
	m_candidateList = NULL;
	m_pageIndex = 0;
	m_pageOffset = 0;
	m_hFont = NULL;
}

CandidateController::~CandidateController() {
	releasePageSpecs();
}

void CandidateController::setCandidateList(const vector<tstring> *list,
	bool isSeparated)
{
	int i;

	m_candidateList = list;
	m_isSeparated = isSeparated;

	releasePageSpecs();
	m_pageSpecs.clear();

	PageSpec *curPage = new PageSpec();
	int pageSize = 0, pos = 0;
	int numCands = m_candidateList->size();

	for (i = 0; i < numCands; i++) {
		tstring cand = (*m_candidateList)[i];

		int candLen = cand.length();
		candLen = candLen > CANDIDATE_UNITS ? CANDIDATE_UNITS : candLen;

		if (pos + candLen - 1 >= CANDIDATE_UNITS) {
			curPage->setSize(pageSize);
			m_pageSpecs.push_back(curPage);

			curPage = new PageSpec();
			curPage->setStartIndex(i);

			pageSize = 0;
			pos = 0;
			i--;

			continue;
		}

		pageSize++;
		pos += candLen;

		if (m_isSeparated)
			pos++;

		if (pos >= CANDIDATE_UNITS) {
			curPage->setSize(pageSize);
			m_pageSpecs.push_back(curPage);

			if (i >= numCands - 1)
				break;

			curPage = new PageSpec();
			curPage->setStartIndex(i + 1);

			pageSize = 0;
			pos = 0;

			continue;
		}
	}

	if (pos < CANDIDATE_UNITS) {
		curPage->setSize(pageSize);
		m_pageSpecs.push_back(curPage);
	}

	m_pageIndex = 0;
	m_pageOffset = 0;
}

int CandidateController::getSelectedIndex() const {
	PageSpec *pageSpec = m_pageSpecs[m_pageIndex];
	int startIndex = pageSpec->getStartIndex();
	int pageSize = pageSpec->getSize();
	int pageOffset = m_pageOffset >= pageSize ? pageSize - 1 : m_pageOffset;

	return startIndex + pageOffset;
}

void CandidateController::firstCandidate() {
	m_pageIndex = 0;
	m_pageOffset = 0;
}

void CandidateController::nextCandidate(bool isLarge) {
	if (isLarge)
		m_pageIndex++;
	else {
		PageSpec *pageSpec = m_pageSpecs[m_pageIndex];
		int pageSize = pageSpec->getSize();
		int pageOffset = m_pageOffset >= pageSize ? pageSize - 1 :
			m_pageOffset;

		pageOffset++;

		if (pageOffset >= pageSize) {
			m_pageIndex++;
			m_pageOffset = 0;
		}
		else
			m_pageOffset = pageOffset;
	}

	if (m_pageIndex >= m_pageSpecs.size())
		m_pageIndex = 0;
}

void CandidateController::prevCandidate(bool isLarge) {
	int numPages = m_pageSpecs.size();

	if (isLarge) {
		m_pageIndex--;
		if (m_pageIndex < 0)
			m_pageIndex = numPages - 1;
	}
	else {
		PageSpec *pageSpec = m_pageSpecs[m_pageIndex];
		int pageSize = pageSpec->getSize();
		int pageOffset = m_pageOffset >= pageSize ? pageSize - 1 :
			m_pageOffset;

		pageOffset--;

		if (pageOffset < 0) {
			m_pageIndex--;
			if (m_pageIndex < 0)
				m_pageIndex = numPages - 1;
			pageSpec = m_pageSpecs[m_pageIndex];
			pageSize = pageSpec->getSize();

			m_pageOffset = pageSize - 1;
		}
		else
			m_pageOffset = pageOffset;
	}

	if (m_pageIndex >= m_pageSpecs.size())
		m_pageIndex = 0;
}

void CandidateController::draw(HDC hDC) {
	int i, j;

	int areaWidth = m_characterWidth * CANDIDATE_UNITS;

	PatBlt(hDC, CANDIDATE_AREA_MARGIN, CANDIDATE_AREA_MARGIN,
		areaWidth, m_characterHeight, PATCOPY);

	HFONT hPrevFont = NULL;
	if (m_hFont != NULL)
		hPrevFont = (HFONT)SelectObject(hDC, m_hFont);

	int pos = 0;

	PageSpec *pageSpec = m_pageSpecs[m_pageIndex];
	int startIndex = pageSpec->getStartIndex();
	int pageSize = pageSpec->getSize();
	int pageOffset = m_pageOffset >= pageSize ? pageSize - 1 : m_pageOffset;

	for (i = 0; i < pageSize; i++) {
		tstring cand = (*m_candidateList)[startIndex + i];

		int candLen = cand.length();
		candLen = candLen > CANDIDATE_UNITS ? CANDIDATE_UNITS : candLen;

		int bkPrevMode = 0;
		bool isSelected = i == pageOffset;

		if (isSelected) {
			HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 255));
			HBRUSH hPrevBrush = (HBRUSH)SelectObject(hDC, hBrush);

			PatBlt(hDC, pos * m_characterWidth + CANDIDATE_AREA_MARGIN,
				CANDIDATE_AREA_MARGIN, candLen * m_characterWidth,
				m_characterHeight, PATCOPY);

			SelectObject(hDC, hPrevBrush);
			DeleteObject(hBrush);

			bkPrevMode = SetBkMode(hDC, TRANSPARENT);
		}

		for (j = 0 ; j < candLen; j++) {
			ExtTextOut(hDC, (j + pos) * m_characterWidth +
				CANDIDATE_AREA_MARGIN, CANDIDATE_AREA_MARGIN, 0, NULL,
				&cand[j], 1, NULL);
		}

		pos += candLen;

		if (isSelected)
			SetBkMode(hDC, bkPrevMode);

		if (m_isSeparated)
			pos++;
	}

	if (m_hFont != NULL)
		SelectObject(hDC, hPrevFont);
}

void CandidateController::releasePageSpecs() {
	int i;

	int numPages = m_pageSpecs.size();
	for (i = 0; i < numPages; i++)
		delete m_pageSpecs[i];
}
