#ifndef DRAWUTIL_H
#define DRAWUTIL_H

#include "OpenerGDI.h"
#include <windows.h>

extern HWND g_hDesktopWindow;



class DrawUtil {
public:
	enum {
		DRAW_TRIANGLE_NORTH = 0,
		DRAW_TRIANGLE_WEST = 1,
		DRAW_TRIANGLE_SOUTH = 2,
		DRAW_TRIANGLE_EAST = 3,
		DRAW_BUTTON_MAIN = 0,
		DRAW_BUTTON_FOLD = 1,
		DRAW_BUTTON_TASKITEM = 2,
		DRAW_RATIO_100 = 0,
		DRAW_RATIO_75 = 1,
		DRAW_RATIO_50 = 2,
		DRAW_RATIO_25 = 3,
		DRAW_FLAG_TEXTTRANS = 4,
		TASKBAR_HEIGHT = 24,
		MAX_TASK_ITEM_WIDTH = 125
	};
	
	static void AlphaBlend(OpenerDIBSection32 *dest, OpenerDIBSection32 *source, POINT bottomleft, UINT nRatio){
		LPDWORD lpBits = dest->getBits();
		LPDWORD lpBackBits = source->getBits();
		int SizeX = dest->xsize();
		int SizeY = dest->ysize();
		int BSizeX = source->xsize();
		
		switch (nRatio) {
			case DRAW_RATIO_50:
				for(int y = SizeY - 1; y >= 0; y--){
					for(int x = 0; x < SizeX; x++){
						lpBits[y * SizeX + x] = ((lpBits[y * SizeX + x] >> 1) & 0x7F7F7F7F) + 
							((lpBackBits[(y + bottomleft.y) * BSizeX + (x + bottomleft.x)] >> 1) & 0x7F7F7F7F);
					}
				}
				break;
			case DRAW_RATIO_75:
				for(int y = SizeY - 1; y >= 0; y--){
					for(int x = 0; x < SizeX; x++){
						lpBits[y * SizeX + x] = ((lpBits[y * SizeX + x] >> 2) & 0x3F3F3F3F) * 3 + 
							((lpBackBits[(y + bottomleft.y) * BSizeX + (x + bottomleft.x)] >> 2) & 0x3F3F3F3F);
					}
				}
				break;
			case DRAW_RATIO_25:
				for(int y = SizeY - 1; y >= 0; y--){
					for(int x = 0; x < SizeX; x++){
						lpBits[y * SizeX + x] = ((lpBits[y * SizeX + x] >> 2) & 0x3F3F3F3F) + 
							((lpBackBits[(y + bottomleft.y) * BSizeX + (x + bottomleft.x)] >> 2) & 0x3F3F3F3F) * 3;
					}
				}
				break;
		}
	}

	static void drawButton(HDC hDC, RECT rect, COLORREF color, bool pushed) {
		HBRUSH hBrush = CreateSolidBrush(color);
		HBRUSH hPrevBrush = (HBRUSH)SelectObject(hDC, hBrush);

		PatBlt(hDC, 0, 0, rect.right, rect.bottom, PATCOPY);

		SelectObject(hDC, hPrevBrush);
		DeleteObject(hBrush);

		HPEN hPen1 = CreatePen(PS_SOLID, 1, adjustLightness24(color,
			pushed ? -0.1 : 0.1));
		HPEN hPrevPen = (HPEN)SelectObject(hDC, hPen1);

		MoveToEx(hDC, rect.right - 1, 0, NULL);
		LineTo(hDC, 0, 0);
		LineTo(hDC, 0, rect.bottom - 1);

		HPEN hPen2 = CreatePen(PS_SOLID, 1, adjustLightness24(color,
			pushed ? 0.1 : -0.1));
		SelectObject(hDC, hPen2);

		LineTo(hDC, rect.right - 1, rect.bottom - 1);
		LineTo(hDC, rect.right - 1, 0);

		SelectObject(hDC, hPrevPen);
		DeleteObject(hPen1);
		DeleteObject(hPen2);
	}

	static void drawButton(HWND hWnd, HDC hDC, RECT rect, COLORREF color, bool pushed, OpenerDIBSection32 *hBack,
	LPCTSTR lpText, COLORREF txColor, UINT ItemType, UINT nRatioButton, UINT nRatioText) {
		OpenerDIBSection32 hButton(hDC, rect.right - rect.left, rect.bottom - rect.top);
		RECT brc;POINT pt2;
		GetWindowRect(hWnd, &brc);
		pt2.x = brc.left;
		pt2.y = hBack->ysize() - brc.bottom;
		COLORREF utxColor = txColor ^ 0x00FFFFFF;
		OpenerDIBSection32 textBuf(hDC, rect.right, rect.bottom);
		HBRUSH hUtxBrush = CreateSolidBrush(utxColor);
		FillRect(textBuf, &rect, hUtxBrush);
		DeleteObject(hUtxBrush);

		//Debug
		//HDC hDesktopDC = GetDC(g_hDesktopWindow);

		OpenerMonoDDB tMonoDDB(hDC, rect.right, rect.bottom);
		HBITMAP hMonoBit = tMonoDDB.getBitmap();
		HDC hMonoDC = tMonoDDB.getDeviceContext();

		HBRUSH hBrush = CreateSolidBrush(color);
		HBRUSH hPrevBrush = (HBRUSH)SelectObject(hButton, hBrush);

		PatBlt(hButton, 0, 0, rect.right, rect.bottom, PATCOPY);

		//Debug
		//BitBlt(hDesktopDC, 240, 10, MAX_TASK_ITEM_WIDTH, TASKBAR_HEIGHT, hButton, 0, 0, SRCCOPY);

		SelectObject(hButton, hPrevBrush);
		DeleteObject(hBrush);

		HPEN hPen1 = CreatePen(PS_SOLID, 1, adjustLightness24(color,
			pushed ? -0.1 : 0.1));
		HPEN hPrevPen = (HPEN)SelectObject(hButton, hPen1);

		MoveToEx(hButton, rect.right - 1, 0, NULL);
		LineTo(hButton, 0, 0);
		LineTo(hButton, 0, rect.bottom - 1);

		HPEN hPen2 = CreatePen(PS_SOLID, 1, adjustLightness24(color,
			pushed ? 0.1 : -0.1));
		SelectObject(hButton, hPen2);

		LineTo(hButton, rect.right - 1, rect.bottom - 1);
		LineTo(hButton, rect.right - 1, 0);

		SelectObject(hButton, hPrevPen);
		DeleteObject(hPen1);
		DeleteObject(hPen2);

		DrawUtil::AlphaBlend(&hButton, hBack, pt2, nRatioButton);

		//Debug
		//BitBlt(hDesktopDC, 240, 10 + TASKBAR_HEIGHT * 1, MAX_TASK_ITEM_WIDTH, TASKBAR_HEIGHT, hButton, 0, 0, SRCCOPY);
			
		if(ItemType == DrawUtil::DRAW_BUTTON_FOLD){
			UINT direction = (UINT)lpText;
			
			POINT center = {(rect.left + rect.right) / 2,
			(rect.top + rect.bottom) / 2};
			DrawUtil::drawTriangle(textBuf, center,
				direction, txColor);
		}
		else if(ItemType == DrawUtil::DRAW_BUTTON_MAIN){
			COLORREF prevTextColor = SetTextColor(textBuf, txColor);
			int prevBkMode = SetBkMode(textBuf, TRANSPARENT);

			RECT captionRect = rect;
			InflateRect(&captionRect, -5, -1);
			DrawText(textBuf, lpText, -1, &captionRect, DT_CENTER | DT_VCENTER |
				DT_SINGLELINE);

			OffsetRect(&captionRect, 1, 0);
			DrawText(textBuf, lpText, -1, &captionRect, DT_CENTER | DT_VCENTER |
				DT_SINGLELINE);

			SetTextColor(textBuf, prevTextColor);
			SetBkMode(textBuf, prevBkMode);
		}
		else{
			COLORREF prevTextColor = SetTextColor(textBuf, txColor);
			int prevBkMode = SetBkMode(textBuf, TRANSPARENT);

			RECT captionRect = rect;
			InflateRect(&captionRect, -4, 0);
			DrawText(textBuf, lpText, -1, &captionRect, DT_LEFT | DT_VCENTER |
				DT_SINGLELINE);

			SetTextColor(textBuf, prevTextColor);
			SetBkMode(textBuf, prevBkMode);
		}

		//Debug
		//BitBlt(hDesktopDC, 240, 10 + TASKBAR_HEIGHT * 2, MAX_TASK_ITEM_WIDTH, TASKBAR_HEIGHT, textBuf, 0, 0, SRCCOPY);

		SetBkColor(textBuf, utxColor);
		BitBlt(hMonoDC, 0, 0, rect.right, rect.bottom, textBuf, 0, 0, SRCCOPY);
		OpenerDDB hFullMask(hDC, rect.right, rect.bottom);
		BitBlt(hFullMask, 0, 0, rect.right, rect.bottom, hMonoDC, 0, 0, SRCCOPY);

		if(nRatioText){
			POINT ptText;
			ptText.x = 0;
			ptText.y = 0;
			DrawUtil::AlphaBlend(&textBuf, &hButton, ptText, nRatioText);
		}

		BitBlt(hButton, 0, 0, rect.right, rect.bottom, hFullMask, 0, 0, SRCAND);
		PatBlt(hFullMask, 0, 0, rect.right, rect.bottom, DSTINVERT);

		BitBlt(textBuf, 0, 0, rect.right, rect.bottom, hFullMask, 0, 0, SRCAND);
		
		BitBlt(hButton, 0, 0, rect.right, rect.bottom, textBuf, 0, 0, SRCPAINT);

		//Transfar Result to Button
		BitBlt(hDC, rect.left, rect.top, rect.right, rect.bottom, hButton, 0, 0, SRCCOPY);

		//Debug
		//ReleaseDC(g_hDesktopWindow, hDesktopDC);
	}

	static void drawPanel(HDC hDC, RECT &rect, COLORREF color) {
		HBRUSH hBrush = CreateSolidBrush(color);
		HBRUSH hPrevBrush = (HBRUSH)SelectObject(hDC, hBrush);

		PatBlt(hDC, 0, 0, rect.right, rect.bottom, PATCOPY);

		SelectObject(hDC, hPrevBrush);
		DeleteObject(hBrush);

		HPEN hPen1 = CreatePen(PS_SOLID, 1, adjustLightness24(color, 0.2));
		HPEN hPrevPen = (HPEN)SelectObject(hDC, hPen1);

		MoveToEx(hDC, rect.right - 1, 0, NULL);
		LineTo(hDC, 0, 0);
		LineTo(hDC, 0, rect.bottom - 1);

		HPEN hPen2 = CreatePen(PS_SOLID, 1, adjustLightness24(color, 0.1));
		SelectObject(hDC, hPen2);

		MoveToEx(hDC, rect.right - 2, 1, NULL);
		LineTo(hDC, 1, 1);
		LineTo(hDC, 1, rect.bottom - 2);

		HPEN hPen3 = CreatePen(PS_SOLID, 1, adjustLightness24(color, -0.2));
		SelectObject(hDC, hPen3);

		MoveToEx(hDC, 0, rect.bottom - 1, NULL);
		LineTo(hDC, rect.right - 1, rect.bottom - 1);
		LineTo(hDC, rect.right - 1, 0);

		HPEN hPen4 = CreatePen(PS_SOLID, 1, adjustLightness24(color, -0.1));
		SelectObject(hDC, hPen4);

		MoveToEx(hDC, 1, rect.bottom - 2, NULL);
		LineTo(hDC, rect.right - 2, rect.bottom - 2);
		LineTo(hDC, rect.right - 2, 1);

		SelectObject(hDC, hPrevPen);
		DeleteObject(hPen1);
		DeleteObject(hPen2);
		DeleteObject(hPen3);
		DeleteObject(hPen4);
	}

	static void drawTriangle(HDC hDC, POINT center, int dir, COLORREF color) {
		POINT pts[3];
		switch (dir) {
		case DRAW_TRIANGLE_NORTH:
			pts[0].x = center.x - 4, pts[0].y = center.y + 2;
			pts[1].x = center.x + 4, pts[1].y = center.y + 2;
			pts[2].x = center.x, pts[2].y = center.y - 2;
			break;
		case DRAW_TRIANGLE_WEST:
			pts[0].x = center.x + 2, pts[0].y = center.y - 4;
			pts[1].x = center.x - 2, pts[1].y = center.y;
			pts[2].x = center.x + 2, pts[2].y = center.y + 4;
			break;
		case DRAW_TRIANGLE_SOUTH:
			pts[0].x = center.x + 4, pts[0].y = center.y - 2;
			pts[1].x = center.x - 4, pts[1].y = center.y - 2;
			pts[2].x = center.x, pts[2].y = center.y + 2;
			break;
		case DRAW_TRIANGLE_EAST:
			pts[0].x = center.x - 2, pts[0].y = center.y - 4;
			pts[1].x = center.x - 2, pts[1].y = center.y + 4;
			pts[2].x = center.x + 2, pts[2].y = center.y;
			break;
		}

		HPEN hPen = CreatePen(PS_SOLID, 1, color);
		HPEN hPrevPen = (HPEN)SelectObject(hDC, hPen);

		HBRUSH hBrush = CreateSolidBrush(color);
		HBRUSH hPrevBrush = (HBRUSH)SelectObject(hDC, hBrush);

		Polygon(hDC, pts, 3);

		SelectObject(hDC, hPrevPen);
		DeleteObject(hPen);

		SelectObject(hDC, hPrevBrush);
		DeleteObject(hBrush);
	}

	static COLORREF adjustLightness24(COLORREF rgb, double adjust) {
		double r = ((rgb & 0xff0000) >> 16) / 255.0;
		double g = ((rgb & 0xff00) >> 8) / 255.0;
		double b = (rgb & 0xff) / 255.0;

		double h = 0.0, s = 0.0, l = 0.0;
		rgbToHsl(h, s, l, r, g, b);

		l += adjust;
		l = l < 0.0 ? 0.0 : (l > 1.0 ? 1.0 : l);

		hslToRgb(r, g, b, h, s, l);

		return ((int)(r * 255.0) << 16) | ((int)(g * 255.0) << 8) |
			(int)(b * 255.0);
	}

	// from farbtastic jQuery plugin (GPL'd)
	// http://acko.net/dev/farbtastic
	static void rgbToHsl(double &h, double &s, double &l, double r, double g,
		double b) {

		double min = r < (g < b ? g : b) ? r : (g < b ? g : b);
		double max = r > (g > b ? g : b) ? r : (g > b ? g : b);
		double delta = max - min;

		l = (min + max) / 2.0;

		s = 0.0;
		if (l > 0.0 && l < 1.0)
			s = delta / (l < 0.5 ? (2.0 * l) : (2.0 - 2.0 * l));

		h = 0.0;
		if (delta > 0.0) {
			if (max == r && max != g)
				h += (g - b) / delta;
			if (max == g && max != b)
				h += (2.0 + (b - r) / delta);
			if (max == b && max != r)
				h += (4.0 + (r - g) / delta);
			h /= 6.0;
		}
	}

	static void hslToRgb(double &r, double &g, double &b, double h, double s,
		double l) {

		double m2 = (l <= 0.5) ? l * (s + 1.0) : l + s - l * s;
		double m1 = l * 2.0 - m2;

		r = hueToRgb(m1, m2, h + 0.33333);
		g = hueToRgb(m1, m2, h);
		b = hueToRgb(m1, m2, h - 0.33333);
	}

private:
	static double hueToRgb(double m1, double m2, double h) {
		h = (h < 0.0) ? h + 1.0 : ((h > 1.0) ? h - 1.0 : h);
		if (h * 6.0 < 1.0)
			return m1 + (m2 - m1) * h * 6.0;
		else if (h * 2.0 < 1.0)
			return m2;
		else if (h * 3.0 < 2.0)
			return m1 + (m2 - m1) * (0.66666 - h) * 6.0;
		else
			return m1;
	}
};

#endif /* DRAWUTIL_H */
