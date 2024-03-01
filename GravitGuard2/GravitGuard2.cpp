// GravitGuard2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "guard.h"
#include <windows.h>

// Force NVIDIA/AMD enable
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

std::wstring get_env(LPCWSTR s)
{
	std::wstring appdata(1024, 'X');

	DWORD len = GetEnvironmentVariableW(s, &appdata[0], appdata.size());
	appdata.resize(len);
	return appdata;
}

int main(int argc, const char** argv)
{
	auto java_home = get_env(L"JAVA_HOME");
    run_java(java_home, argc, argv);
}