#include "main.h"
#include "script.h"
#include "keyboard.h"
#include "Logger.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID /*lpReserved*/) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        SetModuleHandle(hModule);
        scriptRegister(hModule, ScriptMain);
        keyboardHandlerRegister(OnKeyboardMessage);
        break;
    case DLL_PROCESS_DETACH:
        scriptUnregister(hModule);
        keyboardHandlerUnregister(OnKeyboardMessage);
        break;
    }
    return TRUE;
}