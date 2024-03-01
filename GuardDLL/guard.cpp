#include "pch.h"
#include "guard.h"
#include "GravitGuard.h"
#include "jni.h"
#include "xorstr.hpp"
#include "jli_defines.h"
void protectMain(std::wstring java_home);

GUARD_EXPORT void run_java(std::wstring javaPath, int argc, const char** argv) {
	protectMain(javaPath);
	gg->enter_trusted_context();
	std::wstring jliDllPath = javaPath + xorstr_(L"\\bin\\jli.dll");
    HMODULE jliDll = LoadLibraryW(jliDllPath.c_str());
	auto jliLaunch = gg->getAddress<decltype(&JLI_Launch)>(jliDll, xorstr_("JLI_Launch"));
	auto jliCmdToArgs = gg->getAddress<decltype(&JLI_CmdToArgs)>(jliDll, xorstr_("JLI_CmdToArgs"));
	auto jliGetArgc = gg->getAddress<decltype(&JLI_GetStdArgc)>(jliDll, xorstr_("JLI_GetStdArgc"));
	auto jliGetArgs = gg->getAddress<decltype(&JLI_GetStdArgs)>(jliDll, xorstr_("JLI_GetStdArgs"));
	auto jlInitArgProcessing = gg->getAddress<decltype(&JLI_InitArgProcessing)>(jliDll, xorstr_("JLI_InitArgProcessing"));
	jlInitArgProcessing(JNI_FALSE, JNI_TRUE);
	int appargc;
	char** appargv;
	jliCmdToArgs(GetCommandLineA());
	appargc = jliGetArgc();
	appargv = (char**)malloc((appargc + 1) * (sizeof(char*)));
	{
		int i = 0;
		StdArg* stdargs = jliGetArgs();
		for (i = 0; i < appargc; i++) {
			appargv[i] = stdargs[i].arg;
		}
		appargv[i] = NULL;
	}
	gg->exit_trusted_context();
	jliLaunch(appargc, appargv, 0, NULL, 0, NULL, "GravitGuard", "GravitGuard", "GravitGuard", "GravitGuard", false, false, false, 0);
}

GravitGuard* gg;

void protectMain(std::wstring java_home) {
	gg = new GravitGuard(java_home);
}