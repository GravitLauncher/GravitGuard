#pragma once
#include "GuardDetour.h"
#include "LdrHeader.h"
#include "Config.h"
#include <vector>
#include <string>
#include <thread>
#include <optional>
#include "jni.h"
#include "jvmti.h"
#include "psapi.h"
#include"LicenseChecker.h"
/*
class ModuleException : public std::exception
{
public:
	enum class ErrorType
	{
		MODULE_NOT_FOUND
	};
	ErrorType type;
	std::string_view info;
	ModuleException(ErrorType e, std::string_view info) : type(e), info(info) {}

};*/
#ifdef GUARD_DEBUG_MODE
class GuardException : public std::exception
{
};
#endif


class GuardHooks
{
public:
	void init();
	void postJVMCreated(JavaVM* jvmPtr, JNIEnv* env, jvmtiEnv* env_ti);
	GuardDetour<GuardLibraryDefines::LdrLoadDll>* ldrLoadDll = nullptr;
	GuardDetour<jclass  (JNICALL*)(JNIEnv*, const char*)>* findClass = nullptr;
	GuardDetour<jclass (JNICALL*)(JNIEnv*, const char*, jobject, const jbyte*, jsize)>* defineClass = nullptr;
	GuardDetour<jclass (JNICALL *)(JNIEnv*, const char*, jobject, const jbyte*, jsize, jobject, const char*)>* magicDefineClass = nullptr;
	GuardDetour<decltype(&GetProcAddress)>* getProcAddress = nullptr;
	~GuardHooks();
};
class GravitGuard
{
private:
	JavaVM* jvmPtr = nullptr;
	JNIEnv* jniEnv = nullptr;
	jvmtiEnv* envTi = nullptr;
public:
	GravitGuard();
	enum class CheckResult
	{
		OK, WARN, CRASH
	};
	typedef HMODULE module_t;
	typedef FARPROC ptr_t;
	typedef MODULEINFO module_info_t;
	typedef HANDLE handle_t;
	handle_t currentProcess;
	void* currentLpBasePoint;
	std::vector<void*> systemModules;
	std::vector<std::wstring> safeModules;
	GuardHooks hooks;
	LicenseChecker license;
	void init();
	void jvmCreated(void* jvm_ptr, void* jvm_env);
	module_t (__stdcall *getModuleByName)(const char* name);
	unsigned short (_stdcall *getStackTrace)(unsigned long skip_frames, unsigned long capture_frames,void** ptrs, unsigned long* hash);
	BOOL (_stdcall *getModuleEx)(unsigned long flags, LPCSTR name, module_t* out_mod);
	BOOL(_stdcall* getModuleInfoEx)(handle_t process, module_t mod, module_info_t* out_info, unsigned long size);
	inline module_info_t getModuleInfo(module_t mod)
	{
		module_info_t result;
		if (!getModuleInfoEx(currentProcess, mod, &result, sizeof(module_info_t)))
		{
#ifdef GUARD_DEBUG_MODE
			throw GuardException();
#endif
		}
		return result;
	}
	inline module_t getModuleByAddress(void* addr)
	{
		module_t mod;
		if (!getModuleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(addr), &mod))
		{
			return nullptr;
		}
		return mod;
	}
	void* getBaseModuleAddress(void* ptr)
	{
		module_t mod = getModuleByAddress(ptr);
		if (mod != nullptr)
		{
			module_info_t info = getModuleInfo(mod);
			return info.lpBaseOfDll;
		}
		return nullptr;
	}
	ptr_t (__stdcall *getFunctionPtr)(module_t mod, const char* name);
	template<typename T>
	inline auto getFunction(module_t mod, const char* name) noexcept
	{
		return reinterpret_cast<T>(getFunctionPtr(mod, name));
	}
	template<typename T>
	inline T getFunction(const char* module_name, const char* name) noexcept
	{
		module_t mod = getModuleByName(module_name);
		if (mod == nullptr) return reinterpret_cast<T>(nullptr);
		return reinterpret_cast<T>(getFunctionPtr(mod, name));
	}
	static constexpr unsigned int CHECK_STACKTRACE_FAST = 1;
	static constexpr unsigned int CHECK_STACKTRACE_DEBUG = 2;
	CheckResult checkStacktrace(unsigned int flags);
};
extern GravitGuard guard;