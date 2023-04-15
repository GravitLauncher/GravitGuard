// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
#include "Config.h"
#include "Logger.h"
#include "xorstr.h"
// Force NVIDIA/AMD enable
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
#ifdef GUARD_DEBUG_MODE
    case DLL_PROCESS_ATTACH:
	{
		logger.trace(xorstr_("Process"), xorstr_("ProcessAttach"));
		break;
	}
    case DLL_THREAD_ATTACH:
	{
		logger.trace(xorstr_("Process"), xorstr_("ThreadAttach"));
		break;
	}
    case DLL_THREAD_DETACH:
	{
		logger.trace(xorstr_("Process"), xorstr_("ThreadDetach"));
		break;
	}
    case DLL_PROCESS_DETACH:
	{
		logger.trace(xorstr_("Process"), xorstr_("ThreadDetach"));
		break;
	}
#else
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
#endif
    }
    return TRUE;
}

