// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "kbmucom.h"

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        hKbMucomModule = hModule;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
