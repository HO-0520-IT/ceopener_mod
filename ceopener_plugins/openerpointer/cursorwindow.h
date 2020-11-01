#ifndef CURSORWINDOW_H
#define CURSORWINDOW_H

#include <windows.h>

void registerCursorWindowClass();
void unregisterCursorWindowClass();
HWND createCursorWindow();
void destroyCursorWindow(HWND hWnd);
void showCursorWindow(HWND hWnd);
void hideCursorWindow(HWND hWnd);
void moveCursorWindow(HWND hWnd, int x, int y);

#endif  // CURSORWINDOW_H
