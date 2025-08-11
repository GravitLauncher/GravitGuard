#include "pch.h"
#include <jni.h>
#include <jvmti.h>
#include "guard.h"
#include "GravitGuard.h"
#include "GuardDetour.h"
#include "WindowsInternalDefines.h"
#include "xorstr.hpp"
#include <string_view>
#include "Logger.h"
#include <iostream>
#include <psapi.h>

GuardDetour<GuardLibraryDefines::LdrLoadDll*>* ldrLoadDllHook = nullptr;
GuardDetour<GuardLibraryDefines::LdrpLoadDll*>* ldrpLoadDllHook = nullptr;
GuardDetour<decltype(&LoadLibraryW)>* loadLibraryWHook = nullptr;
GuardDetour<decltype(&LoadLibraryA)>* loadLibraryAHook = nullptr;
GuardDetour<decltype(JNIInvokeInterface_::GetEnv)>* getEnvHook = nullptr;
GuardDetour<decltype(JNIInvokeInterface_::AttachCurrentThread)>* attachCurrentThreadHook = nullptr;
GuardDetour<decltype(JNIInvokeInterface_::AttachCurrentThreadAsDaemon)>* attachCurrentThreadAsDaemonHook = nullptr;
GuardDetour<decltype(&GetProcAddress)>* getProcAddressHook = nullptr;
GuardDetour<decltype(&VirtualProtectEx)>* virtualProtectExHook = nullptr;
GuardDetour<decltype(&VirtualProtect)>* virtualProtectHook = nullptr;

volatile void* always_null = NULL;
inline void makesegfault() {
	always_null = (void*) 0xDEADDEAD;
}

void blackhole_function() {

}

inline void checkTerminateActions(GravitGuard::CheckResultAll result) {
	if (result.result == GravitGuard::CheckResult::TERMINATE_THREAD) {
		TerminateThread(GetCurrentThread(), -8591);
		makesegfault();
	}
	if (result.result == GravitGuard::CheckResult::TERMINATE_APP) {
		TerminateProcess(GetCurrentProcess(), -8592);
		makesegfault();
	}
	if (result.result == GravitGuard::CheckResult::INFINITE_SLEEP) {
		Sleep(60 * 60 * 24 * 356 * 1000);
		makesegfault();
	}
}

NTSTATUS NTAPI LdrLoadDllHook(UINT32 Flags, PUINT32 Reserved, PUNICODE_STRING ModuleFileName, PHANDLE ModuleHandle) {
	std::wstring_view fileName(ModuleFileName->Buffer, ModuleFileName->Length/2);
#ifdef ENABLE_DLL_THREAD_LOCAL_OPTIMIZATION
	thread_local std::vector<std::wstring> checked;
	for (auto l : checked) {
		if (l == fileName) {
			return ldrLoadDllHook->call_original(std::move(Flags), std::move(Reserved), std::move(ModuleFileName), std::move(ModuleHandle));
		}
	}
#endif
	logger.log(Logger::Level::DEBUG, L"LdrLoadDll {}", fileName);
	auto source = gg->getSource(fileName);
	auto result = gg->check(source);
	checkTerminateActions(result);
	if (result.result == GravitGuard::CheckResult::CANCEL) {
		logger.log(Logger::Level::CRITICAL, L"LdrLoadDll {} cancelled", fileName);
		return -1;
	}
	if (result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
		logger.log(Logger::Level::CRITICAL, L"LdrLoadDll {} only signed", fileName);
		Flags |= LOAD_LIBRARY_REQUIRE_SIGNED_TARGET;
	}
	if (!gg->is_known_module(fileName)) {
		gg->add_known_module(fileName);
	}
	auto retCode = ldrLoadDllHook->call_original(std::move(Flags), std::move(Reserved), std::move(ModuleFileName), std::move(ModuleHandle));
	if (NT_SUCCESS(retCode)) {
		auto realModuleFileName = gg->getModuleFileName(*((HMODULE*)ModuleHandle));
		if (fileName != realModuleFileName) {
			logger.log(Logger::Level::DEBUG, L"Found module {}: {}", fileName, realModuleFileName);
			if (!gg->is_known_module(realModuleFileName)) {
				gg->add_known_module(realModuleFileName);
			}
		}
#ifdef ENABLE_DLL_THREAD_LOCAL_OPTIMIZATION
		checked.push_back(std::wstring(fileName));
#endif
	}
	else {
		logger.log(Logger::Level::DEBUG, L"Fail to load module {}", fileName);
	}
	return retCode;
}

NTSTATUS NTAPI LdrpLoadDllHook(BOOLEAN Redirected, PWSTR DllPath, PULONG DllCharacteristics, PUNICODE_STRING DllName, PVOID* BaseAddress, BOOLEAN CallInit) {
	std::wstring_view fileName(DllPath);
	logger.log(Logger::Level::DEBUG, L"LdrpLoadDll {}", fileName);
	auto source = gg->getSource(fileName);
	auto result = gg->check(source);
	checkTerminateActions(result);
	if (result.result == GravitGuard::CheckResult::CANCEL) {
		return -1;
	}
	if (result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
		*DllCharacteristics |= LOAD_LIBRARY_REQUIRE_SIGNED_TARGET;
	}
	gg->add_known_module(fileName);
	return ldrpLoadDllHook->call_original(std::move(Redirected), std::move(DllPath), std::move(DllCharacteristics), std::move(DllName), std::move(BaseAddress), std::move(CallInit));
}

HMODULE WINAPI LoadLibraryWHook(LPCWSTR lpLibFileName) {
	std::wstring_view fileName(lpLibFileName);
	logger.log(Logger::Level::TRACE, L"loadLibraryW {}", fileName);
	std::wcout << L"loadLibraryW " << fileName << std::endl << std::flush;
	auto source = gg->getSource(fileName);
	auto result = gg->check(source);
	checkTerminateActions(result);
	if (result.result == GravitGuard::CheckResult::CANCEL || result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
		return NULL;
	}
	return loadLibraryWHook->call_original(std::move(lpLibFileName));
}

HMODULE WINAPI LoadLibraryAHook(LPCSTR lpLibFileName) {
	auto source = GravitGuard::DLLSource::UNKNOWN_MEMORY;
	auto result = gg->check(source);
	checkTerminateActions(result);
	if (result.result == GravitGuard::CheckResult::CANCEL || result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
		return NULL;
	}
	return loadLibraryAHook->call_original(std::move(lpLibFileName));
}

jint JNICALL GetEnvHook(JavaVM* vm, void** penv, jint version) {
	thread_local bool checked = false;
	if (checked) {
		return getEnvHook->call_original(std::move(vm), std::move(penv), std::move(version));
	}
	auto source = GravitGuard::DLLSource::UNKNOWN_MEMORY;
	auto result = gg->check(source);
	checkTerminateActions(result);
	if (result.result == GravitGuard::CheckResult::CANCEL || result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
		return JNI_ERR;
	}
	checked = true;
	return getEnvHook->call_original(std::move(vm), std::move(penv), std::move(version));
}

jint JNICALL AttachCurrentThreadHook(JavaVM* vm, void** penv, void* args) {
	auto source = GravitGuard::DLLSource::UNKNOWN_MEMORY;
	auto result = gg->check(source);
	checkTerminateActions(result);
	if (result.result == GravitGuard::CheckResult::CANCEL || result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
		return JNI_ERR;
	}
	return attachCurrentThreadHook->call_original(std::move(vm), std::move(penv), std::move(args));
}

jint JNICALL AttachCurrentThreadAsDaemonHook(JavaVM* vm, void** penv, void* args) {
	auto source = GravitGuard::DLLSource::UNKNOWN_MEMORY;
	auto result = gg->check(source);
	checkTerminateActions(result);
	if (result.result == GravitGuard::CheckResult::CANCEL || result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
		return JNI_ERR;
	}
	return attachCurrentThreadAsDaemonHook->call_original(std::move(vm), std::move(penv), std::move(args));
}

FARPROC
WINAPI
GetProcAddressHook(
	_In_ HMODULE hModule,
	_In_ LPCSTR lpProcName
) {
	if (!gg->is_curent_thread_verified() && !gg->is_trusted_context()) {
		auto source = GravitGuard::DLLSource::UNKNOWN_MEMORY;
		auto result = gg->check(source);
		checkTerminateActions(result);
		if (result.result == GravitGuard::CheckResult::CANCEL || result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
			return (FARPROC) &blackhole_function;
		}
		if (!gg->is_trusted_context()) {
			gg->mark_current_thread_verified(true);
		}
	}
	return getProcAddressHook->call_original(std::move(hModule), std::move(lpProcName));
}

BOOL
WINAPI
VirtualProtectExHook(
	_In_ HANDLE hProcess,
	_In_ LPVOID lpAddress,
	_In_ SIZE_T dwSize,
	_In_ DWORD flNewProtect,
	PDWORD lpflOldProtect
) {
	if (!gg->is_curent_thread_verified() && !gg->is_trusted_context() && ((flNewProtect & (PAGE_EXECUTE_READWRITE)) != 0) || ((flNewProtect & (PAGE_EXECUTE)) != 0) || ((flNewProtect & (PAGE_EXECUTE_READ)) != 0)) {
		auto source = GravitGuard::DLLSource::UNKNOWN_MEMORY;
		auto result = gg->check(source);
		checkTerminateActions(result);
		if (result.result == GravitGuard::CheckResult::CANCEL || result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
			return TRUE;
		}
		if (!gg->is_trusted_context()) {
			gg->mark_current_thread_verified(true);
		}
	}
	return virtualProtectExHook->call_original(std::move(hProcess), std::move(lpAddress), std::move(dwSize), std::move(flNewProtect), std::move(lpflOldProtect));
}

BOOL
WINAPI
VirtualProtectHook(
	_In_  LPVOID lpAddress,
	_In_  SIZE_T dwSize,
	_In_  DWORD flNewProtect,
	PDWORD lpflOldProtect
) {
	if (!gg->is_curent_thread_verified() && !gg->is_trusted_context() && ((flNewProtect & (PAGE_EXECUTE_READWRITE)) != 0) || ((flNewProtect & (PAGE_EXECUTE)) != 0) || ((flNewProtect & (PAGE_EXECUTE_READ)) != 0)) {
		auto source = GravitGuard::DLLSource::UNKNOWN_MEMORY;
		auto result = gg->check(source);
		checkTerminateActions(result);
		if (result.result == GravitGuard::CheckResult::CANCEL || result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
			return TRUE;
		}
		if (!gg->is_trusted_context()) {
			gg->mark_current_thread_verified(true);
		}
	}
	return virtualProtectHook->call_original(std::move(lpAddress), std::move(dwSize), std::move(flNewProtect), std::move(lpflOldProtect));
}

void GravitGuard::initHooks()
{
	TrustedContextLock _l;
	auto originalLdrLoadDll = getAddress<GuardLibraryDefines::LdrLoadDll*>(module_ntdll, xorstr_("LdrLoadDll"));
	ldrLoadDllHook = new GuardDetour<GuardLibraryDefines::LdrLoadDll*>(originalLdrLoadDll, &LdrLoadDllHook);
	ldrLoadDllHook->hook();
	getProcAddressHook = new GuardDetour<decltype(&GetProcAddress)>(&GetProcAddress, &GetProcAddressHook);
	getProcAddressHook->hook();
	virtualProtectHook = new GuardDetour<decltype(&VirtualProtect)>(&VirtualProtect, &VirtualProtectHook);
	virtualProtectHook->hook();
	virtualProtectExHook = new GuardDetour<decltype(&VirtualProtectEx)>(&VirtualProtectEx, &VirtualProtectExHook);
	virtualProtectExHook->hook();
	//auto originalLdrpLoadDll = getAddress<GuardLibraryDefines::LdrpLoadDll*>(module_kernelbase, xorstr_("LdrpLoadDll"));
	//ldrpLoadDllHook = new GuardDetour<GuardLibraryDefines::LdrpLoadDll*>(originalLdrpLoadDll, &LdrpLoadDllHook);
	//ldrpLoadDllHook->hook();
	{
		HMODULE modules[1024];
		DWORD cbNeeded;
		auto result = EnumProcessModules(GetCurrentProcess(), &modules[0], 1024, &cbNeeded);
		if (!result) {
			makesegfault();
		}
		for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
			std::wstring filePath = getModuleFileName(modules[i]);
			add_known_module(filePath);
#ifdef GUARD_DEBUG
			logger.log(Logger::Level::TRACE, L"Known module by startup {}", filePath);
#endif
		}
	}
	//loadLibraryAHook = new GuardDetour<decltype(&LoadLibraryA)>(&LoadLibraryA, &LoadLibraryAHook);
	//loadLibraryAHook->hook();
	//loadLibraryWHook = new GuardDetour<decltype(&LoadLibraryW)>(&LoadLibraryW, &LoadLibraryWHook);
	//loadLibraryWHook->hook();
}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM* jvm, char* options, void* reserved) {
	TrustedContextLock _l;
	getEnvHook = new GuardDetour<decltype(JNIInvokeInterface_::GetEnv)>(jvm->functions->GetEnv, &GetEnvHook);
	getEnvHook->hook();
	attachCurrentThreadHook = new GuardDetour<decltype(JNIInvokeInterface_::AttachCurrentThread)>(jvm->functions->AttachCurrentThread, &AttachCurrentThreadHook);
	attachCurrentThreadHook->hook();
	attachCurrentThreadAsDaemonHook = new GuardDetour<decltype(JNIInvokeInterface_::AttachCurrentThreadAsDaemon)>(jvm->functions->AttachCurrentThreadAsDaemon, &AttachCurrentThreadAsDaemonHook);
	attachCurrentThreadAsDaemonHook->hook();
	HMODULE jvm_module = GetModuleHandleA(xorstr_("jvm.dll"));
	gg->set_jvm_module(jvm_module);
	jvmtiError error;
	jint res;
	jvmtiEnv* jvmti = NULL;
	JNIEnv* jniEnv = NULL;
	res = jvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_8);

	if (res != JNI_OK || jniEnv == NULL)
	{
		return JNI_OK;
	}
	return JNI_OK;
}