#include "PadInput.h"

#include <brainapi.h>

PadInput::PadInput() {
    m_hNoticeWindow = NULL;
    m_isEnabled = false;
    m_currentPadType = 0;
    m_currentPad = NULL;
    m_halfPadPageIndex = 0;
    m_fullPadPageIndex = 0;
}

PadInput::~PadInput() {
}

void PadInput::init(HWND hWnd, bool isLargePad) {
    m_halfSymbolPads.clear();
    m_fullSymbolPads.clear();

	m_isLarge = isLargePad;

	createModePad();

	if (isLargePad) {
		createModeAndHalfSymbolPadLarge();
		createModeAndFullSymbolPadLarge();
		switchPad(PAD_HALF_SYMBOL);
	}
	else {
		createHalfSymbolPads();
		createFullSymbolPads();
		switchPad(PAD_MODE);
	}

    m_hNoticeWindow = hWnd;
}

void PadInput::terminate() {
    int i;

    disable();

    releasePadEditorButtons(m_modePad);

    int numPads = m_halfSymbolPads.size();
    for (i = 0; i < numPads; i++)
        releasePadEditorButtons(m_halfSymbolPads[i]);

    numPads = m_fullSymbolPads.size();
    for (i = 0; i < numPads; i++)
        releasePadEditorButtons(m_fullSymbolPads[i]);
}

void PadInput::enable() {
    if (m_isEnabled)
        return;

    m_isEnabled = true;

    redraw();
}

void PadInput::disable() {
    if (!m_isEnabled)
        return;

    m_isEnabled = false;

    PadEditor_win(m_hNoticeWindow, 0, 0, NULL);
}

void PadInput::redraw() {
    if (!m_isEnabled)
        return;

    PadEditor_win(m_hNoticeWindow, 0, 0, NULL);
    PadEditor_win(m_hNoticeWindow, 3, 0, m_currentPad);
}

unsigned short PadInput::processButtonDown(unsigned short id) {
    PadPenProc(1, id);
    PadPenProc_after();

    if (id == 135)
        return 0;
    else if (id == BUTTON_SYMBOL_PAD_PREV) {
        if (m_currentPadType == PAD_HALF_SYMBOL) {
            m_halfPadPageIndex--;
            if (m_halfPadPageIndex < 0)
                m_halfPadPageIndex = m_halfSymbolPads.size() - 1;
        }
        else {
            m_fullPadPageIndex--;
            if (m_fullPadPageIndex < 0)
                m_fullPadPageIndex = m_fullSymbolPads.size() - 1;
        }

        switchPad(m_currentPadType);

        return 0;
    }
    else if (id == BUTTON_SYMBOL_PAD_NEXT) {
        if (m_currentPadType == PAD_HALF_SYMBOL) {
            m_halfPadPageIndex++;
            if (m_halfPadPageIndex >= m_halfSymbolPads.size())
                m_halfPadPageIndex = 0;
        }
        else {
            m_fullPadPageIndex++;
            if (m_fullPadPageIndex >= m_fullSymbolPads.size())
                m_fullPadPageIndex = 0;
        }

        switchPad(m_currentPadType);

        return 0;
    }

    return id;
}

void PadInput::switchPad(int type) {
    m_currentPadType = type;

	if (m_isLarge) {
		switch (type) {
			break;
		case PAD_HALF_SYMBOL:
			m_currentPad = m_halfSymbolPads[0];
			break;
		case PAD_FULL_SYMBOL:
			m_currentPad = m_fullSymbolPads[0];
			break;
		}
	}
	else {
		switch (type) {
		case PAD_MODE:
			m_currentPad = m_modePad;
			break;
		case PAD_HALF_SYMBOL:
			m_currentPad = m_halfSymbolPads[m_halfPadPageIndex];
			break;
		case PAD_FULL_SYMBOL:
			m_currentPad = m_fullSymbolPads[m_fullPadPageIndex];
			break;
		}
	}

    redraw();
}

void PadInput::createModePad() {
    m_modePad = new PadEditorButton[6 + 1];

    int index = 0;
    unsigned short text1[] = {0x2121, 0x3221, 0x4123, 0};   // _A
    setPadEditorButton(&m_modePad[index++], 30, 10, 30, 20,
        BUTTON_ALPHA_HALF, text1, sizeof(text1) / 2);

    unsigned short text2[] = {0x2121, 0x3221, 0x3523, 0};   // _5
    setPadEditorButton(&m_modePad[index++], 30, 40, 30, 20,
        BUTTON_NUMBER_HALF, text2, sizeof(text2) / 2);

    unsigned short text3[] = {0x2121, 0x3221, 0x7721, 0};   // _@
    setPadEditorButton(&m_modePad[index++], 30, 70, 30, 20,
        BUTTON_SYMBOL_HALF, text3, sizeof(text3) / 2);

    unsigned short text4[] = {0x2121, 0x2277, 0};   // Ç†
    setPadEditorButton(&m_modePad[index++], 70, 10, 30, 20,
        BUTTON_HIRA, text4, sizeof(text4) / 2);

    unsigned short text5[] = {0x2121, 0x358f, 0};   // ÇT
    setPadEditorButton(&m_modePad[index++], 70, 40, 30, 20,
        BUTTON_NUMBER_FULL, text5, sizeof(text5) / 2);

    unsigned short text6[] = {0x2121, 0x7791, 0};   // Åó
    setPadEditorButton(&m_modePad[index++], 70, 70, 30, 20,
        BUTTON_SYMBOL_FULL, text6, sizeof(text6) / 2);

    setPadEditorEnd(&m_modePad[index++]);
}

void PadInput::createHalfSymbolPads() {
    unsigned short syms1[] = {
        0x2a21, 0x4921, 0x7421, 0x7021, 0x7321, 0x7521,
        0x4721, 0x4a21, 0x4b21, 0x7621, 0x5c21, 0x2421,
        0x5d21, 0x2521, 0x3f21, 0x2721, 0x2821, 0x6321
    };
    unsigned short syms2[] = {
        0x6121, 0x6421, 0x2921, 0x7721, 0x4e21, 0x6f21,
        0x4f21, 0x3021, 0x3221, 0x2e21, 0x5021, 0x4321,
        0x5121, 0x4121
    };

    int id = BUTTON_SYMBOL_BASE;
    PadEditorButton *pad = createSymbolPad(syms1, id, sizeof(syms1) / 2, true);
    m_halfSymbolPads.push_back(pad);

    id += sizeof(syms1) / 2;
    pad = createSymbolPad(syms2, id, sizeof(syms2) / 2, true);
    m_halfSymbolPads.push_back(pad);
}

void PadInput::createFullSymbolPads() {
    unsigned short syms1[] = {
        0x2a91, 0x4991, 0x7491, 0x7091, 0x7391, 0x7591,
        0x4791, 0x4a91, 0x4b91, 0x7691, 0x5c91, 0x2291,
        0x3c91, 0x2391, 0x2691, 0x2791, 0x2891, 0x6391
    };
    unsigned short syms2[] = {
        0x6191, 0x6491, 0x2991, 0x7791, 0x5691, 0x6f91,
        0x5791, 0x3091, 0x3291, 0x4691, 0x5091, 0x4391,
        0x5191, 0x4191
    };

    int id = BUTTON_SYMBOL_BASE;
    PadEditorButton *pad = createSymbolPad(syms1, id, sizeof(syms1) / 2,
        false);
    m_fullSymbolPads.push_back(pad);

    id += sizeof(syms1) / 2;
    pad = createSymbolPad(syms2, id, sizeof(syms2) / 2, false);
    m_fullSymbolPads.push_back(pad);
}

void PadInput::createModeAndHalfSymbolPadLarge() {
	unsigned short syms[] = {
		0x2a21, 0x4921, 0x7421, 0x7021, 0x7321, 0x7521, 0x4721,
		0x4a21, 0x4b21, 0x7621, 0x5c21, 0x2421, 0x5d21, 0x2521,
		0x3f21, 0x2721, 0x2821, 0x6321, 0x6121, 0x6421, 0x2921,
		0x7721, 0x4e21, 0x6f21, 0x4f21, 0x3021, 0x3221, 0x2e21,
		0x5021, 0x4321, 0x5121, 0x4121
	};

    int id = BUTTON_SYMBOL_BASE;
    PadEditorButton *pad = createModeAndSymbolPadLarge(syms, id, sizeof(syms) / 2, true);
    m_halfSymbolPads.push_back(pad);
}

void PadInput::createModeAndFullSymbolPadLarge() {
	unsigned short syms[] = {
        0x2a91, 0x4991, 0x7491, 0x7091, 0x7391, 0x7591, 0x4791,
		0x4a91, 0x4b91, 0x7691, 0x5c91, 0x2291, 0x3c91, 0x2391,
		0x2691, 0x2791, 0x2891, 0x6391, 0x6191, 0x6491, 0x2991,
		0x7791, 0x5691, 0x6f91, 0x5791, 0x3091, 0x3291, 0x4691,
		0x5091, 0x4391, 0x5191, 0x4191
    };

    int id = BUTTON_SYMBOL_BASE;
    PadEditorButton *pad = createModeAndSymbolPadLarge(syms, id, sizeof(syms) / 2, false);
    m_fullSymbolPads.push_back(pad);
}

PadInput::PadEditorButton *PadInput::createSymbolPad(
    const unsigned short *syms, int startId, int numSyms, bool padding) {

    const int numSymsHoriz = 6;

    int i;

    PadEditorButton *pad = new PadEditorButton[numSyms + 3 + 1];

    int index = 0;
    for (i = 0; i < numSyms; i++) {
        int left = SYMBOL_BUTTON_WIDTH * (i % numSymsHoriz) + 4;
        int top = SYMBOL_BUTTON_HEIGHT * (i / numSymsHoriz) + 4;

        if (padding) {
            unsigned short text[] = {0x2121, syms[i], 0};
            setPadEditorButton(&pad[index++], left, top, SYMBOL_BUTTON_WIDTH,
                SYMBOL_BUTTON_HEIGHT, startId + i, text, sizeof(text) / 2);
        }
        else {
            unsigned short text[] = {syms[i], 0};
            setPadEditorButton(&pad[index++], left, top, SYMBOL_BUTTON_WIDTH,
                SYMBOL_BUTTON_HEIGHT, startId + i, text, sizeof(text) / 2);
        }
    }

    unsigned short prevText[] = {0x2121, 0x2293, 0};   // <
    setPadEditorButton(&pad[index++], 5, 70, 30, 20,
        BUTTON_SYMBOL_PAD_PREV, prevText, sizeof(prevText) / 2);

    unsigned short nextText[] = {0x2121, 0x2193, 0};   // >
    setPadEditorButton(&pad[index++], 40, 70, 30, 20,
        BUTTON_SYMBOL_PAD_NEXT, nextText, sizeof(nextText) / 2);

    unsigned short closeText[] = {0x5f91, 0};   // Å~
    setPadEditorButton(&pad[index++], 100, 70, 20, 20,
        BUTTON_SYMBOL_PAD_CLOSE, closeText, sizeof(closeText) / 2);

    setPadEditorEnd(&pad[index++]);

    return pad;
}

PadInput::PadEditorButton *PadInput::createModeAndSymbolPadLarge(
    const unsigned short *syms, int startId, int numSyms, bool padding) {

    const int numSymsHoriz = 7;

    int i;

    PadEditorButton *pad = new PadEditorButton[6 + numSyms + 1];

    int index = 0;
    unsigned short text1[] = {0x2121, 0x3221, 0x4123, 0};   // _A
    setPadEditorButton(&pad[index++], 8, 16, 30, 20,
        BUTTON_ALPHA_HALF, text1, sizeof(text1) / 2);

    unsigned short text2[] = {0x2121, 0x3221, 0x3523, 0};   // _5
    setPadEditorButton(&pad[index++], 8, 44, 30, 20,
        BUTTON_NUMBER_HALF, text2, sizeof(text2) / 2);

    unsigned short text3[] = {0x2121, 0x3221, 0x7721, 0};   // _@
    setPadEditorButton(&pad[index++], 8, 72, 30, 20,
        BUTTON_SYMBOL_HALF, text3, sizeof(text3) / 2);

    unsigned short text4[] = {0x2121, 0x2277, 0};   // Ç†
    setPadEditorButton(&pad[index++], 42, 16, 30, 20,
        BUTTON_HIRA, text4, sizeof(text4) / 2);

    unsigned short text5[] = {0x2121, 0x358f, 0};   // ÇT
    setPadEditorButton(&pad[index++], 42, 44, 30, 20,
        BUTTON_NUMBER_FULL, text5, sizeof(text5) / 2);

    unsigned short text6[] = {0x2121, 0x7791, 0};   // Åó
    setPadEditorButton(&pad[index++], 42, 72, 30, 20,
        BUTTON_SYMBOL_FULL, text6, sizeof(text6) / 2);

    for (i = 0; i < numSyms; i++) {
        int left = SYMBOL_BUTTON_WIDTH * (i % numSymsHoriz) + 86;
        int top = SYMBOL_BUTTON_HEIGHT * (i / numSymsHoriz) + 4;

        if (padding) {
            unsigned short text[] = {0x2121, syms[i], 0};
            setPadEditorButton(&pad[index++], left, top, SYMBOL_BUTTON_WIDTH,
                SYMBOL_BUTTON_HEIGHT, startId + i, text, sizeof(text) / 2);
        }
        else {
            unsigned short text[] = {syms[i], 0};
            setPadEditorButton(&pad[index++], left, top, SYMBOL_BUTTON_WIDTH,
                SYMBOL_BUTTON_HEIGHT, startId + i, text, sizeof(text) / 2);
        }
    }

    setPadEditorEnd(&pad[index++]);

    return pad;
}

void PadInput::setPadEditorButton(PadEditorButton *button, short left,
    short top, short width, short height, unsigned short id,
    const unsigned short *text, int textLen) {

    unsigned short *textBuf = new unsigned short[textLen];
    memcpy(textBuf, text, textLen * 2);

    PadEditorCaption *capt = new PadEditorCaption();
    memset(capt, 0, sizeof(PadEditorCaption));

    capt->text = textBuf;
    capt->fontSize = 16;

    memset(button, 0, sizeof(PadEditorButton));

    button->left = left;
    button->top = top;
    button->width = width;
    button->height = height;
    button->id = id;
    button->caption = capt;
}

void PadInput::setPadEditorEnd(PadEditorButton *button) {
    memset(button, 0, sizeof(PadEditorButton));

    button->left = -1;
    button->top = -1;
    button->width = -1;
    button->height = -1;
    button->disabled = -1;
    button->id = -1;
}

void PadInput::releasePadEditorButtons(PadEditorButton *buttons) {
    for ( ; buttons->left != -1; buttons++) {
        PadEditorCaption *capt = buttons->caption;
        delete [] capt->text;
        delete capt;
    }

    delete [] buttons;
}

void PadInput::setIsLargePad(bool isLargePad) {
	if (m_isLarge == isLargePad) 
		return;
	m_isLarge = isLargePad;

	m_halfSymbolPads.clear();
	m_fullSymbolPads.clear();

	if (!isLargePad) {
		createHalfSymbolPads();
		createFullSymbolPads();
		switchPad(PAD_MODE);
	}
	else {
		createModeAndHalfSymbolPadLarge();
		createModeAndFullSymbolPadLarge();
		switchPad(PAD_HALF_SYMBOL);
	}
}
