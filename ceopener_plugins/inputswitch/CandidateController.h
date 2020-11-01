#ifndef CANDINATECONTROLLER_H_
#define CANDINATECONTROLLER_H_

#include <vector>
#include <windows.h>

#ifdef UNICODE
namespace std { typedef wstring tstring; }
#else
namespace std { typedef string tstring; }
#endif

class CandidateController {
public:
	CandidateController();
	virtual ~CandidateController();
	void setCharacterSize(int width, int height)
		{ m_characterWidth = width, m_characterHeight = height; }
	void setCandidateList(const std::vector<std::tstring> *list,
		bool isSeparated);
	int getSelectedIndex() const;
	void firstCandidate();
	void nextCandidate(bool isLarge = false);
	void prevCandidate(bool isLarge = false);
	void setFont(HFONT hFont) { m_hFont = hFont; }
	void draw(HDC hDC);

private:
	enum {
		CANDIDATE_AREA_MARGIN = 2,
		CANDIDATE_UNITS = 15,
	};

	class PageSpec {
	public:
		PageSpec();
		virtual ~PageSpec();
		int getStartIndex() const { return m_startIndex; }
		void setStartIndex(int index) { m_startIndex = index; }
		int getSize() const { return m_size; }
		void setSize(int size) { m_size = size; }

	private:
		int m_startIndex;//Page index
		int m_size;	//Number of Candidates
	};

	void releasePageSpecs();

	int m_characterWidth;
	int m_characterHeight;
	bool m_isSeparated;
	const std::vector<std::tstring> *m_candidateList;
	std::vector<PageSpec *> m_pageSpecs;
	int m_pageIndex;
	int m_pageOffset;
	HFONT m_hFont;
};

#endif /* CANDINATECONTROLLER_H_ */
