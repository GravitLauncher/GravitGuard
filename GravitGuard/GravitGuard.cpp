#include "pch.h"
#include "GravitGuard.h"
#include <iostream>
#include "Logger.h"
#include <sstream>
#include "jni.h"
#include "jvmti.h"
#include "jni_md.h"
#include "jvm_private_export.h"
#include "psapi.h"
#include <string>
#include "xorstr.h"
void voidFunc()
{

}
void blackhole_void()
{

}
std::wstring getexepath()
{
	wchar_t result[MAX_PATH];
	return std::wstring(result, GetModuleFileName(NULL, result, MAX_PATH));
}
GravitGuard::GravitGuard()
{
	getFunctionPtr = &GetProcAddress;
	getModuleByName = &GetModuleHandleA;
	getStackTrace = &CaptureStackBackTrace;
	getModuleEx = &GetModuleHandleExA;
	getModuleInfoEx = &GetModuleInformation;
	currentProcess = GetCurrentProcess();
	currentLpBasePoint = getBaseModuleAddress((void*) &voidFunc);
}
void GravitGuard::init()
{
	module_t nextModule = getModuleByName(xorstr_("ntdll.dll"));
	if (nextModule != nullptr)
		systemModules.push_back(getModuleInfo(nextModule).lpBaseOfDll);
	nextModule = getModuleByName(xorstr_("kernel32.dll"));
	if (nextModule != nullptr)
		systemModules.push_back(getModuleInfo(nextModule).lpBaseOfDll);
	nextModule = getModuleByName(xorstr_("kernelbase.dll"));
	if (nextModule != nullptr)
		systemModules.push_back(getModuleInfo(nextModule).lpBaseOfDll);
	safeModules.push_back(xorstr_(L"kernel32"));
	logger.init();
#ifdef GUARD_DEBUG_MODE
	logger.info(xorstr_("GravitGuard"), xorstr_("GravitGuard Init Phase"));
#endif
#ifndef  GUARD_NO_LICENSE_CHECK
	std::wstring exe_path = getexepath();
	int pOffset = exe_path.rfind('\\');
	std::wstring exe_dir = exe_path.substr(0, pOffset);
	std::wstring exe_name = exe_path.substr(pOffset+1, exe_path.length() - pOffset - 5);
	if (exe_name.ends_with(xorstr_(L"64")) || exe_name.ends_with(xorstr_(L"32")))
	{
		exe_name = exe_name.substr(0, exe_name.size() - 2);
	}
	std::wstring license_file = (exe_dir + xorstr_(L"\\license.lic"));
	std::wstring license_file2 = xorstr_(L"license.lic");
#ifdef GUARD_DEBUG_MODE
	logger.debug(xorstr_("LicensePath"), license_file);
	logger.debug(xorstr_("LicenseEXE"), logger.toUtf8(exe_name));
#endif
	license.readCurrentLicense(license_file.c_str());
#ifdef GUARD_DEBUG_MODE
	logger.debug(xorstr_("LicenseUsername"), license.currentLicense.username);
#endif
	if (!license.checkCurrentLicense() || logger.toUtf8(exe_name) != license.currentLicense.username)
	{
#ifdef GUARD_DEBUG_MODE
		logger.crash(xorstr_("GravitGuard"), xorstr_("Invalid license"));
		exit(-72);
#else
		exit(-72);
#endif
	}
#endif // ! GUARD_NO_LICENSE
	hooks.init();
}

void GravitGuard::jvmCreated(void* jvm_ptr, void* jvm_env)
{
#ifdef GUARD_DEBUG_MODE
	logger.debug(xorstr_("GravitGuard"), xorstr_("JVM Created"));
#endif
	JavaVM* vm = reinterpret_cast<JavaVM*>(jvm_ptr);
	JNIEnv* env = reinterpret_cast<JNIEnv*>(jvm_env);
	vm->GetEnv(reinterpret_cast<void**>(&envTi), JVMTI_VERSION_1_0);
	jvmPtr = vm;
	jniEnv = env;
	hooks.postJVMCreated(vm, env, envTi);
}

GravitGuard::CheckResult GravitGuard::checkStacktrace(unsigned int flags)
{
	void* ptrs[1024];
	bool anyModuleFound = false;
	bool unknownMemoryRegion = false;
	bool lastUnknownMemoryRegion = true;
	unsigned short captured;
	int max = 128;
	while(true) {
		captured = getStackTrace(1, 1024, ptrs, NULL);
		max--;
		if (captured > 0 || max <= 0) {
			break;
		} else {
			Sleep(0);
		}
	}
	for (int i = 0; i < captured; ++i)
	{
		module_t mod = getModuleByAddress(ptrs[i]);

		if (mod != nullptr)
		{
			module_info_t mod_info = getModuleInfo(mod);
			if (mod_info.lpBaseOfDll == currentLpBasePoint) continue;
			bool isSystemModule = false;
			for (auto w : systemModules)
			{
				if (w == mod_info.lpBaseOfDll) { isSystemModule = true;  break; }
			}
			WCHAR filename[1024];
#ifdef GUARD_DEBUG_MODE
			DWORD writted = GetModuleFileNameW(mod, filename, 1024);
			logger.trace(xorstr_("CheckStacktrace"), std::wstring(filename, writted));
#endif
			if (unknownMemoryRegion) lastUnknownMemoryRegion = false;
			if (isSystemModule) continue;
			anyModuleFound = true;
		}
		else {
#ifdef GUARD_DEBUG_MODE
			logger.trace(xorstr_("CheckStacktrace"), xorstr_("Unknown memory region"));
#endif
			if(lastUnknownMemoryRegion) unknownMemoryRegion = true;
		}
	}
	if (anyModuleFound) return (unknownMemoryRegion && !lastUnknownMemoryRegion) ? CheckResult::CRASH : CheckResult::OK;
	return CheckResult::CRASH;
}

NTSTATUS NTAPI LdrLoadDllHook(UINT32 Flags, PUINT32 Reserved, PUNICODE_STRING ModuleFileName, PHANDLE ModuleHandle)
{
	thread_local std::vector<std::wstring> verifedDlls;
	thread_local bool verifedThread = false;
	std::wstring_view moduleName(ModuleFileName->Buffer, ModuleFileName->Length / 2);
	bool needAdd = true;
	GravitGuard::CheckResult check_result = GravitGuard::CheckResult::CRASH;
	for (const std::wstring w : verifedDlls) if (moduleName == w) { needAdd = false; goto ldrLoadDllOriginal; }
	for (const std::wstring w : guard.safeModules) if (moduleName == w) { needAdd = false; goto ldrLoadDllOriginal; }
	//std::wcout << L"LdrLoadDll Hook F:" << Flags << L" R:" << Reserved << L" N:" << moduleName << L" H:" << ModuleHandle << std::endl;
#ifdef GUARD_DEBUG_MODE
	logger.debug(xorstr_("LdrLoadDll"), (std::wstringstream() << xorstr_(L"F:") << Flags << xorstr_(L" R:") << Reserved << xorstr_(L" N:") << moduleName << xorstr_(L" H:") << ModuleHandle).str());
#endif
	check_result = guard.checkStacktrace(0);
	if (check_result == GravitGuard::CheckResult::OK)
	{
		goto ldrLoadDllOriginal;
	}
	else if (check_result == GravitGuard::CheckResult::WARN)
	{
		Flags |= LOAD_LIBRARY_REQUIRE_SIGNED_TARGET;
		logger.warn(xorstr_("LdrLoadDll"), xorstr_("Stacktrace check failed. Using magic flag"));
	}
	else
	{
		logger.crash(xorstr_("LdrLoadDll"), xorstr_("Stacktrace check failed. Interrupt"));
		logger.flush();
#ifdef GUARD_WARN_MODE
		goto ldrLoadDllOriginal;
#else
		return 0;
#endif
	}
	ldrLoadDllOriginal:
	NTSTATUS status = guard.hooks.ldrLoadDll->call_original(std::move(Flags), std::move(Reserved), std::move(ModuleFileName), std::move(ModuleHandle));
	if (needAdd && status == 0)
	{
		verifedDlls.push_back(std::wstring(moduleName));
	}
	return status;
}
jclass JNICALL FindClassHook(JNIEnv* env, const char* name)
{
#ifdef GUARD_DEBUG_MODE
	logger.debug(xorstr_("FindClass"), name);
#endif
	return guard.hooks.findClass->call_original(std::move(env), std::move(name));
}
jclass JNICALL DefineClassHook(JNIEnv* env, const char* name, jobject loader, const jbyte* buf, jsize len)
{
	thread_local bool isCorrectThread = false;
	if (!isCorrectThread)
	{
#ifdef GUARD_DEBUG_MODE
		logger.debug(xorstr_("DefineClass"), (std::stringstream() << xorstr_("Class Name: ") << name << xorstr_(" size ") << len).str());
#endif
		isCorrectThread = true;
	}
	return guard.hooks.defineClass->call_original(std::move(env), std::move(name), std::move(loader), std::move(buf), std::move(len));
}
jclass JNICALL JVM_DefineClassWithSourceHook(JNIEnv* env, const char* name, jobject loader,const jbyte* buf, jsize len, jobject pd,const char* source)
{
	thread_local bool isCorrectThread = false;
	if (!isCorrectThread)
	{
#ifdef GUARD_DEBUG_MODE
		logger.debug(xorstr_("MagicDefineClass"), (std::stringstream() << xorstr_("Class Name: ") << name << xorstr_(" size ") << len).str());
#endif
		guard.checkStacktrace(0);
		isCorrectThread = true;
	}
	return guard.hooks.magicDefineClass->call_original(std::move(env), std::move(name), std::move(loader), std::move(buf), std::move(len), std::move(pd), std::move(source));
}
FARPROC NTAPI getFunctionPtrHook(HMODULE mod, const char* name)
{
	thread_local bool checked = false;
	if (!checked)
	{
#ifdef GUARD_DEBUG_MODE
		logger.trace(xorstr_("getProcAddress"), name);
#endif
		GravitGuard::CheckResult result = guard.checkStacktrace(0);
		if (result != GravitGuard::CheckResult::OK)
		{
			logger.crash(xorstr_("getProcAddress"), name);
			logger.flush();
#ifdef GUARD_WARN_MODE
			return guard.hooks.getProcAddress->call_original(std::move(mod), std::move(name));
#else
			TerminateThread(GetCurrentThread(), 0);
#endif
		}
		checked = true;
	}
	return guard.hooks.getProcAddress->call_original(std::move(mod), std::move(name));
}
void GuardHooks::init()
{
	thread_local bool initializeThreadLocal = false;
	initializeThreadLocal = true;
	GuardLibraryDefines::LdrLoadDll ldrLoadDllPtr = guard.getFunction< GuardLibraryDefines::LdrLoadDll>(xorstr_("ntdll.dll"), xorstr_("LdrLoadDll"));
	ldrLoadDll = new GuardDetour<GuardLibraryDefines::LdrLoadDll>(ldrLoadDllPtr, &LdrLoadDllHook);
	ldrLoadDll->hook();
}

void GuardHooks::postJVMCreated(JavaVM* jvmPtr, JNIEnv* env, jvmtiEnv* env_ti)
{
	
	findClass = new GuardDetour<jclass(JNICALL *)(JNIEnv*, const char*)>(env->functions->FindClass, &FindClassHook);
	findClass->hook();
	defineClass = new GuardDetour<jclass(JNICALL*)(JNIEnv*, const char*, jobject, const jbyte*, jsize)>(env->functions->DefineClass, &DefineClassHook);
	defineClass->hook();
	GuardLibraryDefines::JVM_DefineClassWithSource jvm_defineClassPtr = guard.getFunction<GuardLibraryDefines::JVM_DefineClassWithSource>(xorstr_("jvm.dll"), xorstr_("JVM_DefineClassWithSource"));
	if (jvm_defineClassPtr == nullptr)
	{
#ifdef GUARD_DEBUG_MODE
		logger.warn(xorstr_("JVMHooks"), xorstr_("JVM_DefineClassWithSource not found"));
#endif
	}
	else
	{
		magicDefineClass = new GuardDetour<jclass(JNICALL *)(JNIEnv*, const char*, jobject, const jbyte*, jsize, jobject, const char*)>(jvm_defineClassPtr, &JVM_DefineClassWithSourceHook);
		magicDefineClass->hook();
	}
	getProcAddress = new GuardDetour<decltype(&GetProcAddress)>(guard.getFunctionPtr, &getFunctionPtrHook);
	getProcAddress->hook();
}

GuardHooks::~GuardHooks()
{
	if (ldrLoadDll != nullptr) delete ldrLoadDll;
	if (findClass != nullptr) delete findClass;
	if (defineClass != nullptr) delete defineClass;
	if (magicDefineClass != nullptr) delete magicDefineClass;
	if (getProcAddress != nullptr) delete getProcAddress;
}
