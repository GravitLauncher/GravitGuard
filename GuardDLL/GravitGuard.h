#pragma once
#include "WindowsInternalDefines.h"
#include <string>
#include <string_view>
#include <optional>
#include <list>
#include <mutex>
#define GUARD_DEBUG
class GravitGuard
{
private:
	decltype(&GetProcAddress) getProcAddress;
	decltype(&GetModuleHandleExW) getModuleHandleEx;
	void initHooks();
	HMODULE module_ntdll;
	HMODULE module_kernel32;
	HMODULE module_kernelbase;
	HMODULE self_module;
	HMODULE parent_module;
	HMODULE module_jvm;
	const std::wstring java_home;
	std::wstring work_dir;
	std::wstring windows_dir;
	std::list<std::wstring> known_modules;
	std::mutex* known_modules_mutex;
public:
	GravitGuard(std::wstring java_home);
	template<typename T>
	T getAddress(HMODULE module, const char* lpProcName);
	template<typename T>
	T getAddress(HMODULE module, char* lpProcName);
	inline bool is_trusted_context();
	void enter_trusted_context();
	void exit_trusted_context();
	std::optional<std::string> get_env(std::string_view name);
	enum class CheckResult {
		PASS, ONLY_SIGNED, CANCEL, INFINITE_SLEEP, TERMINATE_THREAD, TERMINATE_APP
	};
	enum class DLLSource {
		SELF, SYSTEM, JAVA_HOME, APP_HOME, TEMP, OTHER, UNKNOWN_MEMORY
	};
	struct CheckResultAll {
		CheckResult result;
	};
	struct StackTraceElement {
		void* address;
		std::wstring fileName;
		DLLSource source;
		std::string functionName;
	};
	CheckResultAll check(DLLSource source);
	size_t captureStackTrace(void** buf, size_t size, unsigned long int* hash);
	DLLSource getSource(std::wstring_view view);
	void set_jvm_module(HMODULE module);
	bool is_known_module(std::wstring_view module);
	void add_known_module(std::wstring_view module);
	inline bool is_win_internal_module(HMODULE module) {
		if (module == module_ntdll) {
			return true;
		}
		if (module == module_kernel32) {
			return true;
		}
		if (module == module_kernelbase) {
			return true;
		}
		return false;
	}
	std::wstring getModuleFileName(HMODULE module);
};

class TrustedContextLock
{
	inline TrustedContextLock();
	inline ~TrustedContextLock();
};


extern GravitGuard* gg;

template<typename T>
inline T GravitGuard::getAddress(HMODULE module, char* lpProcName)
{
	return reinterpret_cast<T>(getProcAddress(module, lpProcName));
}

template<typename T>
inline T GravitGuard::getAddress(HMODULE module, const char* lpProcName)
{
	return reinterpret_cast<T>(getProcAddress(module, lpProcName));
}