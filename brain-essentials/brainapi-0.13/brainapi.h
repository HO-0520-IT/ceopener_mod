#ifndef BRAINAPI_H
#define BRAINAPI_H

#include "windows.h"

typedef int (WINAPI *_Res_Open_win)(const wchar_t *);
typedef int (WINAPI *_Res_Close_win)();
typedef int (WINAPI *_Font_Open_win)(const wchar_t *);
typedef int (WINAPI *_Font_Close_win)();
typedef int (WINAPI *_SHDicToolsInit)(HWND);
typedef int (WINAPI *_Win2DicKey_NewPF_win)(int, unsigned long);
typedef void (WINAPI *_TPanel_Initialize_win)(HWND);
typedef void (WINAPI *_TPanel_Terminate_win)();
typedef int (WINAPI *_SLCD_on_win)();
typedef int (WINAPI *_SLCD_off_win)();
typedef int (WINAPI *_SLCD_disp_rectangle_win)(int, int, int, int);
typedef int (WINAPI *_PadEditor_win)(HWND, int, int, const void *); 
typedef int (WINAPI *_PadEditorButton_Num)(HWND); 
typedef unsigned int (WINAPI *_SHDicGetMessageID)(int);
typedef int (WINAPI *_PadPenProc)(int, int);
typedef int (WINAPI *_PadPenProc_after)();
typedef int (WINAPI *_TPanel_PowerHandler)(int);
typedef int (WINAPI *_EdSetSubBacklightState)(int);
typedef int (WINAPI *_SSHOT_WriteSubLcdBMP2File_win)();
typedef int (WINAPI *_EdGetDisablePowerOff)();
typedef int (WINAPI *_EdSetDisablePowerOff)(int);
typedef int (WINAPI *_EdUpdateBacklightState)();
typedef HANDLE (WINAPI *_EdKeepBacklightState)(int, int);
typedef int (WINAPI *_EdFreeBacklightState)(HANDLE);
typedef void (WINAPI *_EdSetBacklightBright)(int);
typedef int (WINAPI *_EdKeepBacklightBright)(int, int);
typedef int (WINAPI *_EdMrSensorGetState)();
typedef int (WINAPI *_EdMrSensorDisable)();
typedef int (WINAPI *_EdMrSensorEnable)();
typedef void * (WINAPI *_EdMmMapIoSpace)(unsigned long, unsigned long, int);
typedef void (WINAPI *_EdMmUnmapIoSpace)(void *, unsigned long);

extern _Res_Open_win Res_Open_win;
extern _Res_Close_win Res_Close_win;
extern _Font_Open_win Font_Open_win;
extern _Font_Close_win Font_Close_win;
extern _SHDicToolsInit SHDicToolsInit;
extern _Win2DicKey_NewPF_win Win2DicKey_NewPF_win;
extern _TPanel_Initialize_win TPanel_Initialize_win;
extern _TPanel_Terminate_win TPanel_Terminate_win;
extern _SLCD_on_win SLCD_on_win;
extern _SLCD_off_win SLCD_off_win;
extern _SLCD_disp_rectangle_win SLCD_disp_rectangle_win;
extern _PadEditor_win PadEditor_win;
extern _PadEditorButton_Num PadEditorButton_Num;
extern _SHDicGetMessageID SHDicGetMessageID;
extern _PadPenProc PadPenProc;
extern _PadPenProc_after PadPenProc_after;
extern _TPanel_PowerHandler TPanel_PowerHandler;
extern _EdSetSubBacklightState EdSetSubBacklightState;
extern _SSHOT_WriteSubLcdBMP2File_win SSHOT_WriteSubLcdBMP2File_win;
extern _EdGetDisablePowerOff EdGetDisablePowerOff;
extern _EdSetDisablePowerOff EdSetDisablePowerOff;
extern _EdUpdateBacklightState EdUpdateBacklightState;
extern _EdKeepBacklightState EdKeepBacklightState;
extern _EdFreeBacklightState EdFreeBacklightState;
extern _EdSetBacklightBright EdSetBacklightBright;
extern _EdKeepBacklightBright EdKeepBacklightBright;
extern _EdMrSensorGetState EdMrSensorGetState;
extern _EdMrSensorDisable EdMrSensorDisable;
extern _EdMrSensorEnable EdMrSensorEnable;
extern _EdMmMapIoSpace EdMmMapIoSpace;
extern _EdMmUnmapIoSpace EdMmUnmapIoSpace;

extern "C" {

int brainApisEnabled();
int loadBrainApis();
void freeBrainApis();

}

#endif  // BRAINAPI_H
