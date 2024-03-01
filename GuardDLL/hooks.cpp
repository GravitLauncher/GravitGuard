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

volatile void* always_null = NULL;
inline void makesegfault() {
	always_null = (void*) 0xDEADDEAD;
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
	logger.log(Logger::Level::DEBUG, L"LdrLoadDll {}", fileName);
	auto source = gg->getSource(fileName);
	auto result = gg->check(source);
	checkTerminateActions(result);
	if (result.result == GravitGuard::CheckResult::CANCEL) {
		return -1;
	}
	if (result.result == GravitGuard::CheckResult::ONLY_SIGNED) {
		Flags |= LOAD_LIBRARY_REQUIRE_SIGNED_TARGET;
	}
	gg->add_known_module(fileName);
	return ldrLoadDllHook->call_original(std::move(Flags), std::move(Reserved), std::move(ModuleFileName), std::move(ModuleHandle));
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

void GravitGuard::initHooks()
{
	auto originalLdrLoadDll = getAddress<GuardLibraryDefines::LdrLoadDll*>(module_ntdll, xorstr_("LdrLoadDll"));
	ldrLoadDllHook = new GuardDetour<GuardLibraryDefines::LdrLoadDll*>(originalLdrLoadDll, &LdrLoadDllHook);
	ldrLoadDllHook->hook();
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