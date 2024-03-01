#pragma once
#include "MinHook.h"
#include "GuardDetourHandler.h"
template<typename Fn>
class GuardDetour : public GuardDetourHandle<Fn>
{
public:
	using typeOriginal = typename GuardDetourHandle<Fn>::typeOriginal;
	using typeHook = typename GuardDetourHandle<Fn>::typeHook;
private:
	unsigned char memory_region[5];
public:
	GuardDetour(typeOriginal original, typeHook hook)
		: GuardDetourHandle<Fn>()
	{
		static bool isMHInitilizated = false;
		if (!isMHInitilizated)
		{
			MH_Initialize();
			MH_SetThreadFreezeMethod(MH_FREEZE_METHOD_FAST_UNDOCUMENTED);
			isMHInitilizated = true;
		}
		MH_STATUS status = MH_CreateHook(reinterpret_cast<LPVOID>(original), reinterpret_cast<LPVOID>(hook), reinterpret_cast<LPVOID*>(&this->originalFunction));
#ifdef GUARD_DEBUG_MODE
		if (status != MH_OK)
			throw MinHookException(status);
#endif
		memcpy_s(&memory_region, 5, reinterpret_cast<LPVOID>(original), 5);
	}
	bool isHooked()
	{
		return memcmp(reinterpret_cast<LPVOID>(this->original), &memory_region, 5) == 0;
	}
	unsigned char* region()
	{
		return &memory_region;
	}
	void* _original()
	{
		return reinterpret_cast<LPVOID>(this->original);
	}
};

