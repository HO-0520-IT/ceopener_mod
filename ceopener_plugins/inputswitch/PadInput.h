#ifndef PADINPUT_H
#define PADINPUT_H

#include <vector>
#include <windows.h>

class PadInput {
public:
    enum {
        PAD_MODE = 1,
        PAD_HALF_SYMBOL = 2,
        PAD_FULL_SYMBOL = 3,
        BUTTON_ALPHA_HALF = 0x8000 | 1,
        BUTTON_NUMBER_HALF = 0x8000 | 2,
        BUTTON_SYMBOL_HALF = 0x8000 | 3,
        BUTTON_HIRA = 0x8000 | 4,
        BUTTON_NUMBER_FULL = 0x8000 | 5,
        BUTTON_SYMBOL_FULL = 0x8000 | 6,
        BUTTON_SYMBOL_PAD_PREV = 0x8000 | 11,
        BUTTON_SYMBOL_PAD_NEXT = 0x8000 | 12,
        BUTTON_SYMBOL_PAD_CLOSE = 0x8000 | 13,
        BUTTON_SYMBOL_BASE = 0x8000 | 101
    };

    PadInput();
    virtual ~PadInput();
    bool isEnabled() const { return m_isEnabled; }
    void init(HWND hWnd, bool isLargePad);
    void terminate();
    void enable();
    void disable();
    void redraw();
    unsigned short processButtonDown(unsigned short id);
    void switchPad(int type);
	void setIsLargePad(bool isLargePad);

private:
    enum {
        SYMBOL_BUTTON_WIDTH = 20,
        SYMBOL_BUTTON_HEIGHT = 20
    };

    struct PadEditorCaption {
        unsigned short *text;
        short fontSize;
        short unknown1[11];
    };

    struct PadEditorButton {
        short left;
        short top;
        short width;
        short height;
        short disabled;
        unsigned short id;
        short unknown1[2];
        PadEditorCaption *caption;
    };

    void createModePad();
    void createHalfSymbolPads();
    void createFullSymbolPads();
	void createModeAndHalfSymbolPadLarge();
	void createModeAndFullSymbolPadLarge();
    PadEditorButton *createSymbolPad(const unsigned short *syms, int startId,
        int numSyms, bool padding);
	PadEditorButton *createModeAndSymbolPadLarge(const unsigned short *syms, int startId,
        int numSyms, bool padding);
    void setPadEditorButton(PadEditorButton *button, short left,
        short top, short width, short height, unsigned short id,
        const unsigned short *text, int textLen);
    void setPadEditorEnd(PadEditorButton *button);
    void releasePadEditorButtons(PadEditorButton *buttons);

    PadEditorButton *m_modePad;
    std::vector<PadEditorButton *> m_halfSymbolPads;
    std::vector<PadEditorButton *> m_fullSymbolPads;
    HWND m_hNoticeWindow;
    bool m_isEnabled;
	bool m_isLarge;
    int m_currentPadType;
    PadEditorButton *m_currentPad;
    int m_halfPadPageIndex;
    int m_fullPadPageIndex;
};

#endif  // PADINPUT_H

