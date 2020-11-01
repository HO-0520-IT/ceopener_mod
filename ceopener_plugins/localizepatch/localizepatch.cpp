#include <windows.h>
#include <winioctl.h>
#include <ceopener_plugin.h>

#ifdef LOCALIZEPATCH_EXPORTS
#define LOCALIZEPATCH_API __declspec(dllexport)
#else
#define LOCALIZEPATCH_API __declspec(dllimport)
#endif

#define IOCTL_LOCALIZEPATCH_BASE 0x8000
#define _LOCALIZEPATCH_ACCESS_CTL_CODE(_Function) \
    CTL_CODE(IOCTL_LOCALIZEPATCH_BASE, _Function, METHOD_NEITHER, \
    FILE_ANY_ACCESS)
#define IOCTL_LOCALIZEPATCH_APPLY_PATCH _LOCALIZEPATCH_ACCESS_CTL_CODE(0x801)
#define IOCTL_LOCALIZEPATCH_RESTORE_PATCH _LOCALIZEPATCH_ACCESS_CTL_CODE(0x802)

typedef long PTRDIFF;
typedef short FWORD;

struct IFIMETRICS {
    ULONG cjThis;
    ULONG cjIfiExtra;
    PTRDIFF dpwszFamilyName;
    PTRDIFF dpwszStyleName;
    PTRDIFF dpwszFaceName;
    PTRDIFF dpwszUniqueName;
    PTRDIFF dpFontSim;
    LONG lEmbedId;
    LONG lItalicAngle;
    LONG lCharBias;
    PTRDIFF dpCharSets;
    BYTE jWinCharSet;
    BYTE jWinPitchAndFamily;
    USHORT usWinWeight;
    ULONG flInfo;
    USHORT fsSelection;
    USHORT fsType;
    // omit...
};

extern "C" {

HANDLE LoadKernelLibrary(LPCWSTR lpszFileName);
BOOL KernelLibIoControl(HANDLE hModule, DWORD dwIoControlCode, LPVOID lpInBuf,
    DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize,
    LPDWORD lpBytesReturned);
BOOL WINAPI SetUserDefaultLCID(LCID Locale);

}

HINSTANCE g_hInstance = NULL;
HANDLE  g_hDriver = NULL;

void applyPatch(bool restore);

BOOL APIENTRY DllMain(HANDLE hInst, DWORD fdwReason, LPVOID pvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH)
        g_hInstance = (HINSTANCE)hInst;

    return true;
}

extern "C" LOCALIZEPATCH_API int pluginInit(HWND hCont) {
    TCHAR pathCStr[MAX_PATH];
	WIN32_FIND_DATA fdata;
    GetModuleFileName(g_hInstance, pathCStr, MAX_PATH);

    g_hDriver = LoadKernelLibrary(pathCStr);
    if (g_hDriver == NULL) {
		MessageBox(hCont, _T("Fontpatch failed.\nError:Cannot load as driver."), _T("エラー"),
            MB_ICONEXCLAMATION);
        return false;
    }

    DWORD numBytes = 0;
	*(_tcsrchr(pathCStr,_T('\\')))=_T('\0');
	*(_tcsrchr(pathCStr,_T('\\')))=_T('\0');
	_tcscat(pathCStr,_T("\\fonts\\jptahoma.ttc"));
	if(!AddFontResource(pathCStr)){
		pathCStr[lstrlen(pathCStr)-1]=_T('f');
		if(!AddFontResource(pathCStr)){
			MessageBox(hCont, _T("Fontpatch failed.\nError:File 'fonts\\jptahoma.ttc/ttf' not found."), _T("エラー"),
            MB_ICONEXCLAMATION);
			return false;
		}
	}
    KernelLibIoControl(g_hDriver, IOCTL_LOCALIZEPATCH_APPLY_PATCH, NULL, 0,
        NULL, 0, &numBytes);

    // jp localization
    SetUserDefaultLCID(1041);
    PostMessage(HWND_BROADCAST, WM_WININICHANGE, 0, INI_INTL);

    return true;
}

extern "C" LOCALIZEPATCH_API void pluginTerminate(HWND hCont) {
    DWORD numBytes = 0;
    KernelLibIoControl(g_hDriver, IOCTL_LOCALIZEPATCH_RESTORE_PATCH, NULL, 0,
        NULL, 0, &numBytes);

    SetUserDefaultLCID(1033);
    PostMessage(HWND_BROADCAST, WM_WININICHANGE, 0, INI_INTL);
}

extern "C" LOCALIZEPATCH_API BOOL IOControl(DWORD dwInst,
    DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf,
    DWORD nOutBufSize, LPDWORD lpBytesReturned) {
    
    switch (dwIoControlCode) {
    case IOCTL_LOCALIZEPATCH_APPLY_PATCH:
        applyPatch(false);
        return true;
    case IOCTL_LOCALIZEPATCH_RESTORE_PATCH:
        applyPatch(true);
        return true;
    }

    return true;
}


void applyPatch(bool restore) {
    int i;
    int trying=0,entr=0;

    unsigned char *p = (unsigned char *)0x80000000;
    unsigned char *endPtr = p + 0x04000000;

    do {
        // modify gdi font entries
        // | kernel ptr | type | handle | flags | object depended data...
        if ((long *)p + 2 < (long *)endPtr &&
            *(long *)p == OBJ_FONT && *((long *)p + 1) == 0x000a &&
            *((long *)p + 2) == 0xffff) {
            entr++;

            unsigned char *entry = p - 0x04;

            // for first 6 entries
            for (i = 0; i < 6; i++) {
                /*LOGFONT *logFont = (LOGFONT *)(entry + 0x10);

				logFont->lfHeight = -12;
				logFont->lfWidth = 0;
				logFont->lfEscapement = 0;
				logFont->lfOrientation = 0;
				logFont->lfWeight = FW_NORMAL;
				logFont->lfItalic = FALSE;
				logFont->lfUnderline = FALSE;
				logFont->lfStrikeOut = FALSE;
				logFont->lfCharSet = SHIFTJIS_CHARSET;
				logFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
				logFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
				logFont->lfQuality = DEFAULT_QUALITY;
				//logFont->lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;*/

                //short *strIndexPtr = (short *)(entry + 0x2c);
                void **fontObjPtr = (void **)(entry + 0x30);

                // some font info (size, style etc.) is changeable
                //logFont->lfHeight = -12;

                *fontObjPtr = NULL;     // force re-quering

                entry += 0x40;
            }
        }

        // modify loaded font info
        //  check chBreakChar, wcBreakChar, and ptlBaseline
        else if (p + 16 < endPtr &&
            *(p + 3) == 0x20 && *(wchar_t *)(p + 10) == 0x0020 &&
            *(long *)(p + 12) == 1 && *(long *)(p + 16) == 0) {
            IFIMETRICS *metrics = (IFIMETRICS *)(p - 108);

            const wchar_t *faceName =
                (wchar_t *)((char *)metrics + metrics->dpwszFamilyName);

            // for Tahoma and oriented font
            if ((unsigned char *)faceName < endPtr) {
                if (wcscmp(faceName, L"Tahoma") == 0 || faceName[0] == L'@') {
                    if (restore) {
                        // restore some metrics
                        metrics->jWinCharSet = faceName[0] == L'@' ?
                            SHIFTJIS_CHARSET : ANSI_CHARSET;
                        metrics->usWinWeight = FW_NORMAL;
                    }
                    else {
                        // invalidate some metrics
                        metrics->jWinCharSet = 0xff;
                        metrics->usWinWeight = 0xffff;
                    }
                }
            }
        }

        // modify string table
        // | ref count | str ptr | index | ...
        // 14: \Windows\tahoma.ttf
        // 15: Regular
        // 16: Tahoma <= target
        else if ((long *)p + 6 < (long *)endPtr &&
            *(long *)p == 14 && *((long *)p + 3) == 15 &&
            ((*((unsigned long *)p + 5) & 0xf0000000) == 0xd0000000) &&
            *((long *)p + 6) == -1) {

            wchar_t *str = (wchar_t *)*((unsigned long *)p + 5);

            if (restore) {
                if (wcscmp(str, L"System") == 0)
                    wcscpy(str, L"Tahoma!");
            }
            else {
                if (wcscmp(str, L"Tahoma!") == 0)
                    wcscpy(str, L"System");
			}
        }

        p += 4;
        trying++;
    } while (p < endPtr);
}

