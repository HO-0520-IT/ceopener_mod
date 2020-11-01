#include "OpenerGDI.h"

extern HFONT g_hMainFont;

OpenerDIBSection32::OpenerDIBSection32(HDC hdc, int iSizeX, int iSizeY) {
	BITMAPINFO bminfo = {0};

	bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bminfo.bmiHeader.biWidth = iSizeX;
	bminfo.bmiHeader.biHeight = iSizeY;
	bminfo.bmiHeader.biPlanes = 1;
	bminfo.bmiHeader.biBitCount = 32;
	bminfo.bmiHeader.biCompression = BI_RGB;

	m_hCont = CreateCompatibleDC(hdc);
	m_hBmp = CreateDIBSection(hdc, &bminfo, DIB_RGB_COLORS, (void**)&m_lpBits, NULL, 0);
	DeleteObject(SelectObject(m_hCont, m_hBmp));

	m_x = iSizeX;
	m_y = iSizeY;

	SelectObject(m_hCont, g_hMainFont);
}

OpenerDIBSection32::~OpenerDIBSection32(){
	DeleteDC(m_hCont);
	DeleteObject(m_hBmp);
}

OpenerDDB::OpenerDDB(HDC hdc, int iSizeX, int iSizeY){
	m_hCont = CreateCompatibleDC(hdc);
	m_hBmp = CreateCompatibleBitmap(hdc, iSizeX, iSizeY);
	DeleteObject(SelectObject(m_hCont, m_hBmp));

	SelectObject(m_hCont, g_hMainFont);
}

OpenerDDB::~OpenerDDB() {
	DeleteDC(m_hCont);
	DeleteObject(m_hBmp);
}

OpenerMonoDDB::OpenerMonoDDB(HDC hdc, int iSizeX, int iSizeY) {
	m_hCont = CreateCompatibleDC(hdc);
	m_hBmp = CreateBitmap(iSizeX, iSizeY, 1, 1, NULL);
	DeleteObject(SelectObject(m_hCont, m_hBmp));

	SelectObject(m_hCont, g_hMainFont);
}

OpenerMonoDDB::~OpenerMonoDDB() {
	DeleteDC(m_hCont);
	DeleteObject(m_hBmp);
}
