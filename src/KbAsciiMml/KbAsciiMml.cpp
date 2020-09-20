#include "pch.h"
#include "musiccom.h"
#include <kmp_pi.h>

#pragma comment(lib, "Shlwapi.lib")

using namespace std;
using namespace boost;

HMODULE hDllModule;

#define KMPPLUGIN_VERSION 1
#define KMPPLUGIN_COPYRIGHT "Copyright (C) asmichi, cisc"
#define KMPPLUGIN_DESCRIPTION "ASCII music.com MML player (powered by cisc's fmgen library)"

class KbAsciiMml
{
public:
    KbAsciiMml();
    BOOL Open(const char* cszFileName, SOUNDINFO* pInfo);
    DWORD Render(BYTE* pBuffer, DWORD dwSize);
    DWORD SetPosition(DWORD dwPos);
    ~KbAsciiMml();

private:
    MusicCom::MusicCom musicCom;
    uint bytespersample;
    SOUNDINFO info;
};

KbAsciiMml::KbAsciiMml()
{
    int fmvol = 0, psgvol = 0;
    try
    {
        wchar_t iniName[MAX_PATH];
        GetModuleFileNameW(hDllModule, iniName, sizeof(iniName));
        PathRemoveExtensionW(iniName);
        PathAddExtensionW(iniName, L".ini");

        wchar_t buf[256];
        GetPrivateProfileStringW(L"KbAsciiMml", L"FMVolume", L"0", buf, sizeof(buf), iniName);
        fmvol = boost::lexical_cast<int>(buf);
        GetPrivateProfileStringW(L"KbAsciiMml", L"PSGVolume", L"0", buf, sizeof(buf), iniName);
        psgvol = boost::lexical_cast<int>(buf);
    }
    catch (boost::bad_lexical_cast)
    {
    }
    musicCom.SetFMVolume(fmvol);
    musicCom.SetPSGVolume(psgvol);
}

KbAsciiMml::~KbAsciiMml()
{
}

BOOL KbAsciiMml::Open(const char* cszFileName, SOUNDINFO* pInfo)
{
    if (pInfo == NULL)
    {
        return FALSE;
    }

    if (pInfo->dwSamplesPerSec == 0)
    {
        pInfo->dwSamplesPerSec = 44100;
    }
    pInfo->dwBitsPerSample = 16;
    pInfo->dwLength = 0xFFFFFFFF;
    pInfo->dwChannels = 2;
    pInfo->dwSeekable = 0;
    pInfo->dwUnitRender = 0;
    pInfo->dwReserved1 = 0xFFFFFFFF;
    pInfo->dwReserved2 = 0;

    if (!musicCom.Load(cszFileName))
        return FALSE;
    if (!musicCom.PrepareMix(pInfo->dwSamplesPerSec))
        return FALSE;

    bytespersample = pInfo->dwChannels * pInfo->dwBitsPerSample / 8;
    info = *pInfo;
    return TRUE;
}

DWORD KbAsciiMml::Render(BYTE* Buffer, DWORD dwSize)
{
    musicCom.Mix((int16*)Buffer, dwSize / bytespersample);
    return dwSize;
}

DWORD KbAsciiMml::SetPosition(DWORD dwPos)
{
    musicCom.PrepareMix(info.dwSamplesPerSec);
    return 0;
}

static HKMP WINAPI kmp_Open(const char* cszFileName, SOUNDINFO* pInfo)
{
    OutputDebugStringA(cszFileName);

    KbAsciiMml* pKbAsciiMml = new KbAsciiMml;
    if (pKbAsciiMml->Open(cszFileName, pInfo))
    {
        return (HKMP)pKbAsciiMml;
    }
    else
    {
        delete pKbAsciiMml;
        return NULL;
    }
}

static void WINAPI kmp_Close(HKMP hKMP)
{
    KbAsciiMml* pKbAsciiMml = (KbAsciiMml*)hKMP;
    delete pKbAsciiMml;
}

static DWORD WINAPI kmp_Render(HKMP hKMP, BYTE* Buffer, DWORD dwSize)
{
    KbAsciiMml* pKbAsciiMml = (KbAsciiMml*)hKMP;
    if (pKbAsciiMml)
    {
        return pKbAsciiMml->Render(Buffer, dwSize);
    }
    else
    {
        return 0;
    }
}

static DWORD WINAPI kmp_SetPosition(HKMP hKMP, DWORD dwPos)
{
    KbAsciiMml* pKbAsciiMml = (KbAsciiMml*)hKMP;
    if (pKbAsciiMml)
    {
        return pKbAsciiMml->SetPosition(dwPos);
    }
    else
    {
        return 0;
    }
}

KMPMODULE* WINAPI kmp_GetTestModule()
{
    static const char* ppszSupportExts[] = {".mml", NULL};

    static KMPMODULE Mod =
        {
            KMPMODULE_VERSION, // dwVersion
            KMPPLUGIN_VERSION, // dwPluginVersion
            KMPPLUGIN_COPYRIGHT, // pszCopyright
            KMPPLUGIN_DESCRIPTION, // pszDescription
            ppszSupportExts, // ppszSupportExts
            1, // dwReentrant
            NULL, // Init
            NULL, // Deinit
            kmp_Open, // Open
            NULL, // OpenFromBuffer
            kmp_Close, // Close
            kmp_Render, // Render
            kmp_SetPosition // SetPosition
        };

    return &Mod;
}

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        hDllModule = hModule;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
