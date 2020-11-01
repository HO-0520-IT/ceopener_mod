#include "EditController.h"

using namespace std;

EditController::Proposed::Proposed(const tstring &propStr,
    const tstring &followStr)
{
    m_proposedString = propStr;
    m_followString = followStr;
}

EditController::Proposed::~Proposed() {
}

EditController::Token::Token(const tstring &kanaStr, const tstring &romaStr) {
    m_kanaString = kanaStr;
    m_romaString = romaStr;
}

EditController::Token::~Token() {
}

EditController::EditController() {
    makeRomaToKanaTable();
    makeKanaToHalfKatakanaTable();

    m_characterWidth = 0;
    m_characterHeight = 0;
    m_hFont = NULL;

    clear();
}

EditController::~EditController() {
    releaseEditBuffer();
    releaseProposedList();
}

tstring EditController::getText() const {
    return obtainConvertedString();
}

void EditController::clear() {
    m_convertMode = CONVERT_MODE_NONE;

    list<Token *>::const_iterator iter = m_editBuffer.begin();
    for ( ; iter != m_editBuffer.end(); iter++)
        delete *iter;

    releaseEditBuffer();
    m_editBuffer.clear();

    releaseProposedList();
    m_proposedList.clear();
}

void EditController::append(TCHAR ch) {
    if (m_convertMode != CONVERT_MODE_NONE)
        return;

    int textLen = getText().length();
    if (textLen >= MAX_NUM_CHARACTERS)
        return;

	TCHAR fullCh;
	//Not a symbol
	if(ch >= _T('a') && ch <= _T('z') ||
		ch >= _T('A') && ch <= _T('Z') ||
		ch >= _T('\0') && ch <= _T(' ') ||
		ch == 0x007f)
		fullCh = convertHalfToFullwidth(ch);
	else
		fullCh = ch;
    m_editBuffer.push_back(new Token(tstring(1, fullCh)));

    list<Token *>::reverse_iterator riter = m_editBuffer.rbegin();
    for ( ; riter != m_editBuffer.rend(); riter++) {
        Token *token = *riter;
        if (token->hasRomaString())
            break;
    }

    TCHAR cons = _T('\0');
    list<Token *>::iterator kanaConvIter = riter.base();

    for ( ; kanaConvIter != m_editBuffer.end(); kanaConvIter++) {
        tstring romaStr = obtainKanaString(kanaConvIter, m_editBuffer.end());
        TCHAR romaFirst = romaStr[0];
        map<tstring, tstring>::const_iterator iter =
            m_romaToKanaTable.find(romaStr);

        if (iter == m_romaToKanaTable.end()) {
            if (romaFirst == 0xff4e && romaStr.size() > 1 &&
                romaStr[1] != 0xff59) {

                *kanaConvIter = new Token(tstring(1, 0x3093),
                    tstring(1, romaFirst));
            }

            cons = romaFirst;
        }
        else {
            if (romaFirst == cons) {
                kanaConvIter--;
                m_editBuffer.erase(kanaConvIter, m_editBuffer.end());
                m_editBuffer.push_back(
                    new Token(tstring(1, 0x3063), tstring(1, romaFirst)));
            }
            else
                m_editBuffer.erase(kanaConvIter, m_editBuffer.end());

            m_editBuffer.push_back(new Token(iter->second, romaStr));

            break;
        }
    }
}

void EditController::chop() {
    if (m_convertMode != CONVERT_MODE_NONE)
        return;

    if (m_editBuffer.empty())
        return;

    delete m_editBuffer.back();
    m_editBuffer.pop_back();
}

tstring EditController::obtainConvertedString(int convMode) const {
    int i;

    if (convMode == -1)
        convMode = m_convertMode;

    tstring newStr;
    if (convMode == CONVERT_MODE_NONE)
        newStr = obtainKanaString();
    else if (convMode == CONVERT_MODE_KANJI)
        newStr = m_kanjiString;
    else if (convMode == CONVERT_MODE_FULL_KATAKANA) {
        tstring str = obtainKanaString();
        int len = str.length();

        for (i = 0; i < len; i++) {
            TCHAR ch = str[i];
            if (ch >= 0x3041 && ch <= 0x3094)
                newStr += ch + 0x0060;
            else
                newStr += ch;
        }
    }
    else if (convMode == CONVERT_MODE_HALF_KATAKANA) {
        tstring str = obtainKanaString();
        int len = str.length();

        for (i = 0; i < len; i++) {
            TCHAR ch = str[i];
            map<TCHAR, tstring>::const_iterator iter =
                m_kanaToHalfKatakanaTable.find(ch);
            if (iter == m_kanaToHalfKatakanaTable.end())
                newStr += ch;
            else
                newStr += iter->second;
        }
    }
    else if (convMode == CONVERT_MODE_FULL_ALPHA)
        newStr = obtainRomaString();

    return newStr;
}

tstring EditController::obtainKanaString() const {
    tstring str;
    list<Token *>::const_iterator iter = m_editBuffer.begin();

    for ( ; iter != m_editBuffer.end(); iter++)
        str += (*iter)->getKanaString();

    return str;
}

tstring EditController::obtainRomaString() const {
    tstring str;
    list<Token *>::const_iterator iter = m_editBuffer.begin();

    for ( ; iter != m_editBuffer.end(); iter++) {
        Token *token = *iter;
        tstring romaStr = token->getRomaString();

        if (romaStr.empty())
            str += token->getKanaString();
        else
            str += romaStr;
    }

    return str;
}

void EditController::makeProposedList() {
    releaseProposedList();
    m_proposedList.clear();

    list<Token *>::const_reverse_iterator riter(m_editBuffer.end());
    list<Token *>::const_reverse_iterator rend(m_editBuffer.begin());

    while (true) {
        tstring propStr = obtainKanaString(m_editBuffer.begin(), riter.base());
        tstring followStr = obtainKanaString(riter.base(), m_editBuffer.end());
        m_proposedList.push_back(new Proposed(propStr, followStr));

        Token *token = *riter;
        bool hasRomaStr = token->hasRomaString();

        TCHAR tail = _T('\0');
        if (hasRomaStr)
            tail = convertFullToHalfwidth(token->getRomaString()[0]);

        riter++;
        if (riter == rend)
            break;

        if (hasRomaStr) {
            propStr = obtainKanaString(m_editBuffer.begin(), riter.base());
            propStr += tail;
            followStr = obtainKanaString(riter.base(), m_editBuffer.end());
            m_proposedList.push_back(new Proposed(propStr, followStr));
        }
    }
}

void EditController::draw(HDC hDC) {
    int i;

    int areaWidth = m_characterWidth * MAX_NUM_CHARACTERS;

	PatBlt(hDC, INPUT_AREA_MARGIN, INPUT_AREA_MARGIN, areaWidth,
        m_characterHeight, PATCOPY);

    HFONT hPrevFont = NULL;
    if (m_hFont != NULL)
        hPrevFont = (HFONT)SelectObject(hDC, m_hFont);
    
    tstring text = obtainConvertedString();
    int len = text.length();

    if (m_convertMode == CONVERT_MODE_NONE) {
        for (i = 0; i < len; i++) {
            ExtTextOut(hDC, i * m_characterWidth + INPUT_AREA_MARGIN,
                INPUT_AREA_MARGIN, 0, NULL, &text[i], 1, NULL);
        }
    }
    else {
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 255));
        HBRUSH hPrevBrush = (HBRUSH)SelectObject(hDC, hBrush);

        PatBlt(hDC, INPUT_AREA_MARGIN, INPUT_AREA_MARGIN,
            len * m_characterWidth, m_characterHeight, PATCOPY);

        SelectObject(hDC, hPrevBrush);
        DeleteObject(hBrush);

        int bkPrevMode = SetBkMode(hDC, TRANSPARENT);

        for (i = 0; i < len; i++) {
            ExtTextOut(hDC, i * m_characterWidth + INPUT_AREA_MARGIN,
                INPUT_AREA_MARGIN, 0, NULL, &text[i], 1, NULL);
        }

        SetBkMode(hDC, bkPrevMode);
    }

    if (m_hFont != NULL)
        SelectObject(hDC, hPrevFont);
}

void EditController::makeRomaToKanaTable() {
    int i;

    // http://www.dpri.kyoto-u.ac.jp/~dptech/oboe/romakana.html

    static TCHAR *romas[] = {
        _T("a"), _T("i"), _T("u"), _T("e"), _T("o"),
        _T("yi"), _T("wu"),
        _T("la"), _T("li"), _T("lu"), _T("le"), _T("lo"),
        _T("xa"), _T("xi"), _T("xu"), _T("xe"), _T("xo"),
        _T("ye"),
        _T("wi"), _T("we"),
        _T("ka"), _T("ki"), _T("ku"), _T("ke"), _T("ko"),
        _T("lka"), _T("lke"),
        _T("xka"), _T("xke"),
        _T("sa"), _T("si"), _T("su"), _T("se"), _T("so"),
        _T("shi"),
        _T("ta"), _T("ti"), _T("tu"), _T("te"), _T("to"),
        _T("chi"), _T("tsu"),
        _T("xtu"),
        _T("ltu"),
        _T("na"), _T("ni"), _T("nu"), _T("ne"), _T("no"),
        _T("ha"), _T("hi"), _T("hu"), _T("he"), _T("ho"),
        _T("fu"),
        _T("ma"), _T("mi"), _T("mu"), _T("me"), _T("mo"),
        _T("ya"), _T("yu"), _T("yo"),
        _T("lya"), _T("lyu"), _T("lyo"),
        _T("xya"), _T("xyu"), _T("xyo"),
        _T("ra"), _T("ri"), _T("ru"), _T("re"), _T("ro"),
        _T("wa"), _T("wo"), _T("nn"),
        _T("lwa"),
        _T("xwa"),

        _T("ga"), _T("gi"), _T("gu"), _T("ge"), _T("go"),
        _T("za"), _T("zi"), _T("zu"), _T("ze"), _T("zo"),
        _T("ji"),
        _T("da"), _T("di"), _T("du"), _T("de"), _T("do"),
        _T("ba"), _T("bi"), _T("bu"), _T("be"), _T("bo"),
        _T("pa"), _T("pi"), _T("pu"), _T("pe"), _T("po"),

        _T("kya"), _T("kyi"), _T("kyu"), _T("kye"), _T("kyo"),
        _T("kwa"),
        _T("sya"), _T("syi"), _T("syu"), _T("sye"),_T("syo"),
        _T("sha"), _T("shu"), _T("she"), _T("sho"),
        _T("tya"), _T("tyi"), _T("tyu"), _T("tye"),_T("tyo"),
        _T("cha"), _T("chu"), _T("che"), _T("cho"),
        _T("cya"), _T("cyi"), _T("cyu"), _T("cye"), _T("cyo"),
        _T("tsa"), _T("tsi"), _T("tse"), _T("tso"),
        _T("tha"), _T("thi"), _T("thu"), _T("the"), _T("tho"),
        _T("twu"),
        _T("nya"), _T("nyi"), _T("nyu"), _T("nye"), _T("nyo"),
        _T("hya"), _T("hyi"), _T("hyu"), _T("hye"), _T("hyo"),
        _T("fa"), _T("fi"), _T("fe"), _T("fo"),
        _T("fyi"), _T("fye"),
        _T("fya"), _T("fyu"), _T("fyo"),
        _T("mya"), _T("myi"), _T("myu"), _T("mye"), _T("myo"),
        _T("rya"), _T("ryi"), _T("ryu"), _T("rye"), _T("ryo"),

        _T("gya"), _T("gyu"), _T("gyo"),
        _T("gyi"), _T("gye"),
        _T("gwa"),
        _T("zya"), _T("zyi"), _T("zyu"), _T("zye"), _T("zyo"),
        _T("ja"), _T("ju"), _T("je"), _T("jo"),
        _T("jya"), _T("jyi"), _T("jyu"), _T("jye"), _T("jyo"),
        _T("dya"), _T("dyi"), _T("dyu"), _T("dye"), _T("dyo"),
        _T("dha"), _T("dhi"), _T("dhu"), _T("dhe"), _T("dho"),
        _T("dwu"),
        _T("bya"), _T("byi"), _T("byu"), _T("bye"), _T("byo"),
        _T("va"), _T("vi"), _T("vu"), _T("ve"), _T("vo"),
        _T("pya"), _T("pyi"), _T("pyu"), _T("pye"), _T("pyo"),

        _T("-")
    };

    static TCHAR kanas[][3] = {
        // ‚ , ‚¢, ‚¤, ‚¦, ‚¨
        {0x3042, 0}, {0x3044, 0}, {0x3046, 0}, {0x3048, 0}, {0x304a, 0},
        // (‚¢, ‚¤)
        {0x3044, 0}, {0x3046, 0},
        // ‚Ÿ, ‚¡, ‚£, ‚¥, ‚§
        {0x3041, 0}, {0x3043, 0}, {0x3045, 0}, {0x3047, 0}, {0x3049, 0},
        // (‚Ÿ, ‚¡, ‚£, ‚¥, ‚§)
        {0x3041, 0}, {0x3043, 0}, {0x3045, 0}, {0x3047, 0}, {0x3049, 0},
        // ‚¢‚¥
        {0x3044, 0x3047, 0},
        // ‚¤‚¡, ‚¤‚¥
        {0x3046, 0x3043, 0}, {0x3046, 0x3047, 0},
        // ‚©, ‚«, ‚­, ‚¯, ‚±
        {0x304b, 0}, {0x304d, 0}, {0x304f, 0}, {0x3051, 0}, {0x3053, 0},
        // ƒ•, ƒ–
        {0x30f5, 0}, {0x30f6, 0},
        // (ƒ•, ƒ–)
        {0x30f5, 0}, {0x30f6, 0},
        // ‚³, ‚µ, ‚·, ‚¹, ‚»
        {0x3055, 0}, {0x3057, 0}, {0x3059, 0}, {0x305b, 0}, {0x305d, 0},
        // (‚µ)
        {0x3057, 0},
        // ‚½, ‚¿, ‚Â, ‚Ä, ‚Æ
        {0x305f, 0}, {0x3061, 0}, {0x3064, 0}, {0x3066, 0}, {0x3068, 0},
        // (‚¿, ‚Â)
        {0x3061, 0}, {0x3064, 0},
        // ‚Á
        {0x3063, 0},
        // (‚Á)
        {0x3063, 0},
        // ‚È, ‚É, ‚Ê, ‚Ë, ‚Ì
        {0x306a, 0}, {0x306b, 0}, {0x306c, 0}, {0x306d, 0}, {0x306e, 0},
        // ‚Í, ‚Ð, ‚Ó, ‚Ö, ‚Ù
        {0x306f, 0}, {0x3072, 0}, {0x3075, 0}, {0x3078, 0}, {0x307b, 0},
        // (‚Ó)
        {0x3075, 0},
        // ‚Ü, ‚Ý, ‚Þ, ‚ß, ‚à
        {0x307e, 0}, {0x307f, 0}, {0x3080, 0}, {0x3081, 0}, {0x3082, 0},
        // ‚â, ‚ä, ‚æ
        {0x3084, 0}, {0x3086, 0}, {0x3088, 0},
        // ‚á, ‚ã, ‚å
        {0x3083, 0}, {0x3085, 0}, {0x3087, 0},
        // (‚á, ‚ã, ‚å)
        {0x3083, 0}, {0x3085, 0}, {0x3087, 0},
        // ‚ç, ‚è, ‚é, ‚ê, ‚ë
        {0x3089, 0}, {0x308a, 0}, {0x308b, 0}, {0x308c, 0}, {0x308d, 0},
        // ‚í, ‚ð, ‚ñ
        {0x308f, 0}, {0x3092, 0}, {0x3093, 0},
        // ‚ì
        {0x308e, 0},
        // (‚ì)
        {0x308e, 0},

        // ‚ª, ‚¬, ‚®, ‚°, ‚²
        {0x304c, 0}, {0x304e, 0}, {0x3050, 0}, {0x3052, 0}, {0x3054, 0},
        // ‚´, ‚¶, ‚¸, ‚º, ‚¼
        {0x3056, 0}, {0x3058, 0}, {0x305a, 0}, {0x305c, 0}, {0x305e, 0},
        // (‚¶)
        {0x3058, 0},
        // ‚¾, ‚À, ‚Ã, ‚Å, ‚Ç
        {0x3060, 0}, {0x3062, 0}, {0x3065, 0}, {0x3067, 0}, {0x3069, 0},
        // ‚Î, ‚Ñ, ‚Ô, ‚×, ‚Ú
        {0x3070, 0}, {0x3073, 0}, {0x3076, 0}, {0x3079, 0}, {0x307c, 0},
        // ‚Ï, ‚Ò, ‚Õ, ‚Ø, ‚Û
        {0x3071, 0}, {0x3074, 0}, {0x3077, 0}, {0x307a, 0}, {0x307d, 0},

        // ‚«‚á, ‚«‚¡, ‚«‚ã, ‚«‚¥, ‚«‚å
        {0x304d, 0x3083, 0}, {0x304d, 0x3043, 0}, {0x304d, 0x3085, 0},
            {0x304d, 0x3047, 0}, {0x304d, 0x3087, 0},
        // ‚­‚Ÿ
        {0x304f, 0x3041, 0},
        // ‚µ‚á, ‚µ‚¡, ‚µ‚ã, ‚µ‚¥, ‚µ‚å
        {0x3057, 0x3083, 0}, {0x3057, 0x3043, 0}, {0x3057, 0x3085, 0},
            {0x3057, 0x3047, 0}, {0x3057, 0x3087, 0},
        // (‚µ‚á, ‚µ‚ã, ‚µ‚å)
        {0x3057, 0x3083, 0}, {0x3057, 0x3085, 0}, {0x3057, 0x3047, 0},
            {0x3057, 0x3087, 0},
        // ‚¿‚á, ‚¿‚¡, ‚¿‚ã, ‚¿‚¥, ‚¿‚å
        {0x3061, 0x3083, 0}, {0x3061, 0x3043, 0}, {0x3061, 0x3085, 0},
            {0x3061, 0x3047, 0}, {0x3061, 0x3087, 0},
        // (‚¿‚á, ‚¿‚ã, ‚¿‚¥, ‚¿‚å)
        {0x3061, 0x3083, 0}, {0x3061, 0x3085, 0}, {0x3061, 0x3047, 0},
            {0x3061, 0x3087, 0},
        // (‚¿‚á, ‚¿‚¡, ‚¿‚ã, ‚¿‚¥, ‚¿‚å)
        {0x3061, 0x3083, 0}, {0x3061, 0x3043, 0}, {0x3061, 0x3085, 0},
            {0x3061, 0x3047, 0}, {0x3061, 0x3087, 0},
        // ‚Â‚Ÿ, ‚Â‚¡, ‚Â‚¥, ‚Â‚§
        {0x3064, 0x3041, 0}, {0x3064, 0x3043, 0}, {0x3064, 0x3047, 0},
            {0x3064, 0x3049, 0},
        // ‚Ä‚á, ‚Ä‚¡, ‚Ä‚ã, ‚Ä‚¥, ‚Ä‚å
        {0x3066, 0x3083, 0}, {0x3066, 0x3043, 0}, {0x3066, 0x3085, 0},
            {0x3066, 0x3047, 0}, {0x3066, 0x3087, 0},
        // ‚Æ‚£
        {0x3068, 0x3045, 0},
        // ‚É‚á, ‚É‚¡, ‚É‚ã, ‚É‚¥, ‚É‚å
        {0x306b, 0x3083, 0}, {0x306b, 0x3043, 0}, {0x306b, 0x3085, 0},
            {0x306b, 0x3047, 0}, {0x306b, 0x3087, 0},
        // ‚Ð‚á, ‚Ð‚¡, ‚Ð‚ã, ‚Ð‚¥, ‚Ð‚å
        {0x3072, 0x3083, 0}, {0x3072, 0x3043, 0}, {0x3072, 0x3085, 0},
            {0x3072, 0x3047, 0}, {0x3072, 0x3087, 0},
        // ‚Ó‚Ÿ, ‚Ó‚¡, ‚Ó‚¥, ‚Ó‚§
        {0x3075, 0x3041, 0}, {0x3075, 0x3043, 0}, {0x3075, 0x3047, 0},
            {0x3075, 0x3049, 0},
        // (‚Ó‚¡, ‚Ó‚¥)
        {0x3075, 0x3043, 0}, {0x3075, 0x3047, 0},
        // ‚Ó‚á, ‚Ó‚ã, ‚Ó‚å
        {0x3075, 0x3083, 0}, {0x3075, 0x3085, 0}, {0x3075, 0x3087, 0},
        // ‚Ý‚á, ‚Ý‚¡, ‚Ý‚ã, ‚Ý‚¥, ‚Ý‚å
        {0x307f, 0x3083, 0}, {0x307f, 0x3043, 0}, {0x307f, 0x3085, 0},
            {0x307f, 0x3047, 0}, {0x307f, 0x3087, 0},
        // ‚è‚á, ‚è‚¡, ‚è‚ã, ‚è‚¥, ‚è‚å
        {0x308a, 0x3083, 0}, {0x308a, 0x3043, 0}, {0x308a, 0x3085, 0},
            {0x308a, 0x3047, 0}, {0x308a, 0x3087, 0},

        // ‚¬‚á, ‚¬‚ã, ‚¬‚å
        {0x304e, 0x3083, 0}, {0x304e, 0x3085, 0}, {0x304e, 0x3087, 0},
        // ‚¬‚¡, ‚¬‚¥
        {0x304e, 0x3043, 0}, {0x304e, 0x3047, 0},
        // ‚®‚Ÿ
        {0x3050, 0x3041, 0},
        // ‚¶‚á, ‚¶‚¡, ‚¶‚ã, ‚¶‚¥, ‚¶‚å
        {0x3058, 0x3083, 0}, {0x3058, 0x3043, 0}, {0x3058, 0x3085, 0},
            {0x3058, 0x3047, 0}, {0x3058, 0x3087, 0},
        // (‚¶‚á, ‚¶‚ã, ‚¶‚¥, ‚¶‚å)
        {0x3058, 0x3083, 0}, {0x3058, 0x3085, 0}, {0x3058, 0x3047, 0},
            {0x3058, 0x3087, 0},
        // (‚¶‚á, ‚¶‚¡, ‚¶‚ã, ‚¶‚¥, ‚¶‚å)
        {0x3058, 0x3083, 0}, {0x3058, 0x3043, 0}, {0x3058, 0x3085, 0},
            {0x3058, 0x3047, 0}, {0x3058, 0x3087, 0},
        // ‚À‚á, ‚À‚¡, ‚À‚ã, ‚À‚¥, ‚À‚å
        {0x3062, 0x3083, 0}, {0x3062, 0x3043, 0}, {0x3062, 0x3085, 0},
            {0x3062, 0x3047, 0}, {0x3062, 0x3087, 0},
        // ‚Å‚á, ‚Å‚¡, ‚Å‚ã, ‚Å‚¥, ‚Å‚å
        {0x3067, 0x3083, 0}, {0x3067, 0x3043, 0}, {0x3067, 0x3085, 0},
            {0x3067, 0x3047, 0}, {0x3067, 0x3087, 0},
        // ‚Ç‚£
        {0x3069, 0x3045, 0},
        // ‚Ñ‚á, ‚Ñ‚¡, ‚Ñ‚ã, ‚Ñ‚¥, ‚Ñ‚å
        {0x3073, 0x3083, 0}, {0x3073, 0x3043, 0}, {0x3073, 0x3085, 0},
            {0x3073, 0x3047, 0}, {0x3073, 0x3087, 0},
        // ƒ”‚Ÿ, ƒ”‚¡, ƒ”, ƒ”‚¥, ƒ”‚§
        {0x30f4, 0x3041, 0}, {0x30f4, 0x3043, 0}, {0x30f4, 0},
            {0x30f4, 0x3047, 0}, {0x30f4, 0x3049, 0},
        // ‚Ò‚á, ‚Ò‚¡, ‚Ò‚ã, ‚Ò‚¥, ‚Ò‚å
        {0x3074, 0x3083, 0}, {0x3074, 0x3043, 0}, {0x3074, 0x3085, 0},
            {0x3074, 0x3047, 0}, {0x3074, 0x3087, 0},

        // "["
        {0x30fc, 0}
    };

    int numKanas = sizeof(romas) / sizeof(TCHAR *);
    for (i = 0; i < numKanas; i++)
        m_romaToKanaTable[convertHalfToFullwidth(romas[i])] = kanas[i];
}

void EditController::makeKanaToHalfKatakanaTable() {
    int i;

    static TCHAR kanas[] = {
        0x3042, 0x3044, 0x3046, 0x3048, 0x304a,     // ‚ 
        0x304b, 0x304d, 0x304f, 0x3051, 0x3053,     // ‚©
        0x3055, 0x3057, 0x3059, 0x305b, 0x305d,     // ‚³
        0x305f, 0x3061, 0x3064, 0x3066, 0x3068,     // ‚½
        0x306a, 0x306b, 0x306c, 0x306d, 0x306e,     // ‚È
        0x306f, 0x3072, 0x3075, 0x3078, 0x307b,     // ‚Í
        0x307e, 0x307f, 0x3080, 0x3081, 0x3082,     // ‚Ü
        0x3084, 0x3086, 0x3088,                     // ‚â
        0x3089, 0x308a, 0x308b, 0x308c, 0x308d,     // ‚ç
        0x308f, 0x3092, 0x3093,                     // ‚í
        0x304c, 0x304e, 0x3050, 0x3052, 0x3054,     // ‚ª
        0x3056, 0x3058, 0x305a, 0x305c, 0x305e,     // ‚´
        0x3060, 0x3062, 0x3065, 0x3067, 0x3069,     // ‚¾
        0x3070, 0x3073, 0x3076, 0x3079, 0x307c,     // ‚Î
        0x3071, 0x3074, 0x3077, 0x307a, 0x307d,     // ‚Ï
        0x3041, 0x3043, 0x3045, 0x3047, 0x3049,     // ‚Ÿ
        0x3063,     // ‚Á
        0x3083, 0x3085, 0x3087,                     // ‚á
        0x30fc      // "["
    };

    static TCHAR katakanas[][3] = {
        {0x71, 0}, {0x72, 0}, {0x73, 0}, {0x74, 0}, {0x75, 0},      // ±
        {0x76, 0}, {0x77, 0}, {0x78, 0}, {0x79, 0}, {0x7a, 0},      // ¶
        {0x7b, 0}, {0x7c, 0}, {0x7d, 0}, {0x7e, 0}, {0x7f, 0},      // »
        {0x80, 0}, {0x81, 0}, {0x82, 0}, {0x83, 0}, {0x84, 0},      // À
        {0x85, 0}, {0x86, 0}, {0x87, 0}, {0x88, 0}, {0x89, 0},      // Å
        {0x8a, 0}, {0x8b, 0}, {0x8c, 0}, {0x8d, 0}, {0x8e, 0},      // Ê
        {0x8f, 0}, {0x90, 0}, {0x91, 0}, {0x92, 0}, {0x93, 0},      // Ï
        {0x94, 0}, {0x95, 0}, {0x96, 0},                            // Ô
        {0x97, 0}, {0x98, 0}, {0x99, 0}, {0x9a, 0}, {0x9b, 0},      // ×
        {0x9c, 0}, {0x66, 0}, {0x9d, 0},                            // Ü
        {0x76, 0x9e, 0}, {0x77, 0x9e, 0}, {0x78, 0x9e, 0}, {0x79, 0x9e, 0},
            {0x7a, 0x9e, 0},      // ¶Þ
        {0x7b, 0x9e, 0}, {0x7c, 0x9e, 0}, {0x7d, 0x9e, 0}, {0x7e, 0x9e, 0},
            {0x7f, 0x9e, 0},      // »Þ
        {0x80, 0x9e, 0}, {0x81, 0x9e, 0}, {0x82, 0x9e, 0}, {0x83, 0x9e, 0},
            {0x84, 0x9e, 0},      // ÀÞ
        {0x8a, 0x9e, 0}, {0x8b, 0x9e, 0}, {0x8c, 0x9e, 0}, {0x8d, 0x9e, 0},
            {0x8e, 0x9e, 0},      // ÊÞ
        {0x8a, 0x9f, 0}, {0x8b, 0x9f, 0}, {0x8c, 0x9f, 0}, {0x8d, 0x9f, 0},
            {0x8e, 0x9f, 0},      // Êß
        {0x67, 0}, {0x68, 0}, {0x69, 0}, {0x6a, 0}, {0x6b, 0},      // §
        {0x6f, 0},      // ¯
        {0x6c, 0}, {0x6d, 0}, {0x6e, 0},                            // ¬
        {0x70, 0}       // "°"
    };

    int numKanas = sizeof(kanas) / sizeof(TCHAR);
    for (i = 0; i < numKanas; i++) {
        TCHAR *ptr = katakanas[i];
        for ( ; *ptr != 0; ptr++)
            *ptr |= 0xff00;

        m_kanaToHalfKatakanaTable[kanas[i]] = katakanas[i];
    }
}

void EditController::releaseEditBuffer() {
    list<Token *>::const_iterator iter = m_editBuffer.begin();
    for ( ; iter != m_editBuffer.end(); iter++)
        delete *iter;
}

void EditController::releaseProposedList() {
    int i;

    int propListSize = m_proposedList.size();
    for (i = 0; i < propListSize; i++)
        delete m_proposedList[i];
}

tstring EditController::obtainKanaString(
    list<Token *>::const_iterator first,
    list<Token *>::const_iterator last) const
{
    tstring str;
    for ( ; first != last; first++)
        str += (*first)->getKanaString();

    return str;
}

TCHAR EditController::convertHalfToFullwidth(TCHAR ch) const {
    return ch - _T('!') + 0xff01;
}

TCHAR EditController::convertFullToHalfwidth(TCHAR ch) const {
    return ch - 0xff01 + _T('!');
}

tstring EditController::convertHalfToFullwidth(const tstring &str) const {
    int i;

    tstring newStr;
    int len = str.length();
    for (i = 0; i < len; i++)
        newStr += convertHalfToFullwidth(str[i]);

    return newStr;
}
