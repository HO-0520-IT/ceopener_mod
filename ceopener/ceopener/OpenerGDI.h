#ifndef OPENERGDI_H_
#define OPENERGDI_H_

#include <windows.h>

class OpenerDIBSection32{
	public:
		OpenerDIBSection32(HDC hdc, int iSizeX, int iSizeY);
		virtual ~OpenerDIBSection32();
		HDC getDeviceContext() const { return m_hCont; }
		HBITMAP getBitmap() const { return m_hBmp; }
		LPDWORD getBits() const{ return m_lpBits; }
		int xsize() const { return m_x; }
		int ysize() const { return m_y; }
		operator HDC(){
			return m_hCont;
		}
		operator HBITMAP(){
			return m_hBmp;
		}
		operator HGDIOBJ(){
			return (HGDIOBJ)m_hBmp;
		}
	private:
		HDC m_hCont;
		HBITMAP m_hBmp;
		LPDWORD m_lpBits;
		int m_x, m_y;
};

class OpenerDDB{
	public:
		OpenerDDB(HDC hdc, int iSizeX, int iSizeY);
		virtual ~OpenerDDB();
		HDC getDeviceContext() const { return m_hCont; }
		HBITMAP getBitmap() const { return m_hBmp; }
		int xsize() const { return m_x; }
		int ysize() const { return m_y; }
		operator HDC(){
			return m_hCont;
		}
		operator HBITMAP(){
			return m_hBmp;
		}
		operator HGDIOBJ(){
			return (HGDIOBJ)m_hBmp;
		}
	private:
		HDC m_hCont;
		HBITMAP m_hBmp;
		int m_x, m_y;
};

class OpenerMonoDDB{
	public:
		OpenerMonoDDB(HDC hdc, int iSizeX, int iSizeY);
		virtual ~OpenerMonoDDB();
		HDC getDeviceContext() const { return m_hCont; }
		HBITMAP getBitmap() const { return m_hBmp; }
		int xsize() const { return m_x; }
		int ysize() const { return m_y; }
		operator HDC(){
			return m_hCont;
		}
		operator HBITMAP(){
			return m_hBmp;
		}
		operator HGDIOBJ(){
			return (HGDIOBJ)m_hBmp;
		}
	private:
		HDC m_hCont;
		HBITMAP m_hBmp;
		int m_x, m_y;
};
#endif //OPENERGDI_H_