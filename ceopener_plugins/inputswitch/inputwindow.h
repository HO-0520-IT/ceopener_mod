#ifndef INPUTWINDOW_H
#define INPUTWINDOW_H

#include <windows.h>
#include "CandidateController.h"
#include "EditController.h"

enum {
    INPUT_WINDOW_CONTROL_CANDIDATE = 1,
    INPUT_WINDOW_CONTROL_EDIT = 2,
};

class InputWindowParams {
public:
    InputWindowParams();
    virtual ~InputWindowParams();
    CandidateController *getCandidateController() const
        { return m_candidateController; }
    void setCandidateController(CandidateController *ctrler)
        { m_candidateController = ctrler; }
    EditController *getEditController() const { return m_editController; }
    void setEditController(EditController *ctrler)
        { m_editController = ctrler; }

private:
    CandidateController *m_candidateController;
    EditController *m_editController;
};

void registerInputWindowClass();
void unregisterInputWindowClass();
HWND createInputWindow(InputWindowParams &params);
void destroyInputWindow(HWND hWnd);
void showInputWindow(HWND hWnd);
void hideInputWindow(HWND hWnd);
int getInputWindowFontSize(HWND hWnd);
void setInputWindowFontSize(HWND hWnd, int size);
int getInputWindowControlType(HWND hWnd);
void setInputWindowControlType(HWND hWnd, int type);

#endif  // INPUTWINDOW_H

