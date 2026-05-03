
#include "base.h"
#include"engine.h"
#include"DX11.h"
#include"hook.h"



BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {

        AllocConsole();
        freopen("CONOUT$", "w+", stdout);

        
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)DX11Hook, NULL, NULL, NULL);
        

    }

    return true;
}
