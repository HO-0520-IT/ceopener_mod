#include "brainapi.h"

_Res_Open_win Res_Open_win = NULL;
_Res_Close_win Res_Close_win = NULL;
_Font_Open_win Font_Open_win = NULL;
_Font_Close_win Font_Close_win = NULL;
_SHDicToolsInit SHDicToolsInit = NULL;
_Win2DicKey_NewPF_win Win2DicKey_NewPF_win = NULL;
_TPanel_Initialize_win TPanel_Initialize_win = NULL;
_TPanel_Terminate_win TPanel_Terminate_win = NULL;
_SLCD_on_win SLCD_on_win = NULL;
_SLCD_off_win SLCD_off_win = NULL;
_SLCD_disp_rectangle_win SLCD_disp_rectangle_win = NULL;
_PadEditor_win PadEditor_win = NULL;
_PadEditorButton_Num PadEditorButton_Num = NULL;
_SHDicGetMessageID SHDicGetMessageID = NULL;
_PadPenProc PadPenProc = NULL;
_PadPenProc_after PadPenProc_after = NULL;
_TPanel_PowerHandler TPanel_PowerHandler = NULL;
_EdSetSubBacklightState EdSetSubBacklightState = NULL;
_SSHOT_WriteSubLcdBMP2File_win SSHOT_WriteSubLcdBMP2File_win = NULL;
_EdGetDisablePowerOff EdGetDisablePowerOff = NULL;
_EdSetDisablePowerOff EdSetDisablePowerOff = NULL;
_EdUpdateBacklightState EdUpdateBacklightState = NULL;
_EdKeepBacklightState EdKeepBacklightState = NULL;
_EdFreeBacklightState EdFreeBacklightState = NULL;
_EdSetBacklightBright EdSetBacklightBright = NULL;
_EdKeepBacklightBright EdKeepBacklightBright = NULL;
_EdMrSensorGetState EdMrSensorGetState = NULL;
_EdMrSensorDisable EdMrSensorDisable = NULL;
_EdMrSensorEnable EdMrSensorEnable = NULL;
_EdMmMapIoSpace EdMmMapIoSpace = NULL;
_EdMmUnmapIoSpace EdMmUnmapIoSpace = NULL;

static HMODULE g_hBrainLib = NULL;

int brainApisEnabled() {
    return g_hBrainLib == NULL ? 0 : 1;
}

int loadBrainApis() {
    g_hBrainLib = LoadLibrary(_T("SharpLib.dll"));
    if (g_hBrainLib == NULL)
        return 0;

    Res_Open_win = (_Res_Open_win)
        GetProcAddress(g_hBrainLib, _T("Res_Open_win"));
    Res_Close_win = (_Res_Close_win)
        GetProcAddress(g_hBrainLib, _T("Res_Close_win"));
    Font_Open_win = (_Font_Open_win)
        GetProcAddress(g_hBrainLib, _T("Font_Open_win"));
    Font_Close_win = (_Font_Close_win)
        GetProcAddress(g_hBrainLib, _T("Font_Close_win"));
    SHDicToolsInit = (_SHDicToolsInit)
        GetProcAddress(g_hBrainLib, _T("SHDicToolsInit"));
    Win2DicKey_NewPF_win = (_Win2DicKey_NewPF_win)
        GetProcAddress(g_hBrainLib, _T("Win2DicKey_NewPF_win"));
    TPanel_Initialize_win = (_TPanel_Initialize_win)
        GetProcAddress(g_hBrainLib, _T("TPanel_Initialize_win"));
    TPanel_Terminate_win = (_TPanel_Terminate_win)
        GetProcAddress(g_hBrainLib, _T("TPanel_Terminate_win"));
    SLCD_on_win = (_SLCD_on_win)
        GetProcAddress(g_hBrainLib, _T("SLCD_on_win"));
    SLCD_off_win = (_SLCD_off_win)
        GetProcAddress(g_hBrainLib, _T("SLCD_off_win"));
    SLCD_disp_rectangle_win = (_SLCD_disp_rectangle_win)
        GetProcAddress(g_hBrainLib, _T("SLCD_disp_rectangle_win"));
    PadEditor_win = (_PadEditor_win)
        GetProcAddress(g_hBrainLib, _T("PadEditor_win"));
    PadEditorButton_Num = (_PadEditorButton_Num)
        GetProcAddress(g_hBrainLib, _T("PadEditorButton_Num"));
    SHDicGetMessageID = (_SHDicGetMessageID)
        GetProcAddress(g_hBrainLib, _T("SHDicGetMessageID"));
    PadPenProc = (_PadPenProc)
        GetProcAddress(g_hBrainLib, _T("PadPenProc"));
    PadPenProc_after = (_PadPenProc_after)
        GetProcAddress(g_hBrainLib, _T("PadPenProc_after"));
    TPanel_PowerHandler = (_TPanel_PowerHandler)
        GetProcAddress(g_hBrainLib, _T("TPanel_PowerHandler"));
    EdSetSubBacklightState = (_EdSetSubBacklightState)
        GetProcAddress(g_hBrainLib, _T("EdSetSubBacklightState"));
    SSHOT_WriteSubLcdBMP2File_win = (_SSHOT_WriteSubLcdBMP2File_win)
        GetProcAddress(g_hBrainLib, _T("SSHOT_WriteSubLcdBMP2File_win"));
    EdGetDisablePowerOff = (_EdGetDisablePowerOff)
        GetProcAddress(g_hBrainLib, _T("EdGetDisablePowerOff"));
    EdSetDisablePowerOff = (_EdSetDisablePowerOff)
        GetProcAddress(g_hBrainLib, _T("EdSetDisablePowerOff"));
    EdUpdateBacklightState = (_EdUpdateBacklightState)
        GetProcAddress(g_hBrainLib, _T("EdUpdateBacklightState"));
    EdKeepBacklightState = (_EdKeepBacklightState)
        GetProcAddress(g_hBrainLib, _T("EdKeepBacklightState"));
    EdFreeBacklightState = (_EdFreeBacklightState)
        GetProcAddress(g_hBrainLib, _T("EdFreeBacklightState"));
    EdSetBacklightBright = (_EdSetBacklightBright)
        GetProcAddress(g_hBrainLib, _T("EdSetBacklightBright"));
    EdKeepBacklightBright = (_EdKeepBacklightBright)
        GetProcAddress(g_hBrainLib, _T("EdKeepBacklightBright"));
    EdMrSensorGetState = (_EdMrSensorGetState)
        GetProcAddress(g_hBrainLib, _T("EdMrSensorGetState"));
    EdMrSensorDisable = (_EdMrSensorDisable)
        GetProcAddress(g_hBrainLib, _T("EdMrSensorDisable"));
    EdMrSensorEnable = (_EdMrSensorEnable)
        GetProcAddress(g_hBrainLib, _T("EdMrSensorEnable"));
    EdMmMapIoSpace = (_EdMmMapIoSpace)
        GetProcAddress(g_hBrainLib, _T("EdMmMapIoSpace"));
    EdMmUnmapIoSpace = (_EdMmUnmapIoSpace)
        GetProcAddress(g_hBrainLib, _T("EdMmUnmapIoSpace"));

    return 1;
}

void freeBrainApis() {
    FreeLibrary(g_hBrainLib);
}
