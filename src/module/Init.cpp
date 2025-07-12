#include "pch.h"

#define HOOKED_FUNCTION CreateMutexA
#define HOOKED_LIBRARY "KERNEL32.DLL"

#define SKIP_INITIALIZEASI
#include "HookInit.hpp"

#ifdef SKIP_INITIALIZEASI
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    HookInit::DLLMain(hinstDLL, fdwReason, lpvReserved);
    return TRUE;
}
#endif
