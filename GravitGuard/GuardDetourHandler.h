#pragma once
#ifdef GUARD_DEBUG_MODE
class MinHookException
{
public:
	MH_STATUS status;
	MinHookException(MH_STATUS status) : status(status) {};
};
#endif
template<typename OriginalFunctionType, typename HookFunctionType>
class GuardDetourBase
{
protected:
	OriginalFunctionType originalFunction;
	OriginalFunctionType original;
	HookFunctionType hookFunction;
public:
	using typeOriginal = OriginalFunctionType;
	using typeHook = HookFunctionType;
	GuardDetourBase() {}
	void hook()
	{
		MH_STATUS status = MH_EnableHook(reinterpret_cast<LPVOID>(original));
#ifdef GUARD_DEBUG_MODE
		if (status != MH_OK)
			throw MinHookException(status);
#endif
	}
	void disable()
	{
		MH_STATUS status = MH_DisableHook(reinterpret_cast<LPVOID>(original));
#ifdef GUARD_DEBUG_MODE
		if (status != MH_OK)
			throw MinHookException(status);
#endif
	}
	void* _original()
	{
		return reinterpret_cast<LPVOID>(original);
	}
	~GuardDetourBase()
	{
		MH_STATUS status = MH_DisableHook(reinterpret_cast<LPVOID>(original));
		status = MH_RemoveHook(reinterpret_cast<LPVOID>(original));
	}
};
template<typename Fn>
struct GuardDetourHandle;
template<typename R, typename... Args>
class GuardDetourHandle<R (*)(Args...)> : public GuardDetourBase< R(__cdecl *)(Args...) , R(__cdecl*)(Args...)>
{
public:
	using typeOriginal = typename GuardDetourBase<R(__cdecl*)(Args...), R(__cdecl*)(Args...)>::typeOriginal;
	using typeHook = typename GuardDetourBase<R(__cdecl*)(Args...), R(__cdecl *)(Args...)>::typeHook;
	inline R call_original(Args&&... args)
	{
		return this->originalFunction(args...);
	}
};
#ifndef _WIN64
template<typename R, typename... Args>
class GuardDetourHandle<R(__stdcall *)(Args...)> : public GuardDetourBase< R(__stdcall *)(Args...), R(__stdcall*)(Args...)>
{
public:
	using typeOriginal = typename GuardDetourBase<R(__stdcall*)(Args...), R(__stdcall*)(Args...)>::typeOriginal;
	using typeHook = typename GuardDetourBase<R(__stdcall*)(Args...), R(__stdcall*)(Args...)>::typeHook;
	inline R call_original(Args&&... args)
	{
		return this->originalFunction(args...);
	}
};
template<typename R, typename... Args>
class GuardDetourHandle<R(__thiscall *)(Args...)> : public GuardDetourBase< R(__thiscall*)(Args...), R(__thiscall*)(Args...)>
{
public:
	using typeOriginal = typename GuardDetourBase<R(__thiscall*)(Args...), R(__thiscall*)(Args...)>::typeOriginal;
	using typeHook = typename GuardDetourBase<R(__thiscall*)(Args...), R(__thiscall*)(Args...)>::typeHook;
	inline R call_original(Args&&... args)
	{
		return this->originalFunction(args...);
	}
};
#endif