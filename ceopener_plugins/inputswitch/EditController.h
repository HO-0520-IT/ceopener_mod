#ifndef EDITCONTROLLER_H_
#define EDITCONTROLLER_H_

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

class EditController {
public:
    enum {
        CONVERT_MODE_NONE = 0,
        CONVERT_MODE_KANJI = 1,
        CONVERT_MODE_FULL_KATAKANA = 2,
        CONVERT_MODE_HALF_KATAKANA = 3,
        CONVERT_MODE_FULL_ALPHA = 4
    };

    class Proposed {
    public:
        Proposed(const std::tstring &propStr, const std::tstring &followStr);
        virtual ~Proposed();
        std::tstring getProposedString() const { return m_proposedString; }
        std::tstring getFollowString() const { return m_followString; }

    private:
        std::tstring m_proposedString;
        std::tstring m_followString;
    };

    EditController();
    virtual ~EditController();
    void setCharacterSize(int width, int height)
        { m_characterWidth = width, m_characterHeight = height; }
    int getConvertMode() { return m_convertMode; }
    void setConvertMode(int mode) { m_convertMode = mode; }
    bool isEmpty() const { return m_editBuffer.empty(); }
    std::tstring getText() const;
    void clear();
    void append(TCHAR ch);
    void chop();
    void setKanjiString(const std::tstring &str) { m_kanjiString = str; }
    std::tstring obtainConvertedString(int convMode = -1) const;
    std::tstring obtainKanaString() const;
    std::tstring obtainRomaString() const;
    void makeProposedList();
    const std::vector<Proposed *> &getProposedList() const
        { return m_proposedList; }
    void setFont(HFONT hFont) { m_hFont = hFont; }
    void draw(HDC hDC);

private:
    enum {
        INPUT_AREA_MARGIN = 2,
        MAX_NUM_CHARACTERS = 15
    };

    class Token {
    public:
        Token(const std::tstring &kanaStr,
            const std::tstring &romaStr = _T(""));
        virtual ~Token();
        std::tstring getKanaString() const { return m_kanaString; }
        void setKanaString(const std::tstring &kanaStr)
            { m_kanaString = kanaStr; }
        bool hasRomaString() const { return !m_romaString.empty(); }
        std::tstring getRomaString() const { return m_romaString; }
        void setRomaString(const std::tstring &romaStr)
            { m_romaString = romaStr; }

    private:
        std::tstring m_kanaString;
        std::tstring m_romaString;
    };

    void makeRomaToKanaTable();
    void makeKanaToHalfKatakanaTable();
    void releaseEditBuffer();
    void releaseProposedList();
    std::tstring obtainKanaString(std::list<Token *>::const_iterator first,
        std::list<Token *>::const_iterator last) const;
    TCHAR convertHalfToFullwidth(TCHAR ch) const;
    TCHAR convertFullToHalfwidth(TCHAR ch) const;
    std::tstring convertHalfToFullwidth(const std::tstring &str) const;

    int m_characterWidth;
    int m_characterHeight;
    std::map<std::tstring, std::tstring> m_romaToKanaTable;
    std::map<TCHAR, std::tstring> m_kanaToHalfKatakanaTable;
    int m_convertMode;
    std::list<Token *> m_editBuffer;
    std::vector<Proposed *> m_proposedList;
    std::tstring m_kanjiString;
    HFONT m_hFont;
};

#endif /* EDITCONTROLLER_H_ */
