#pragma once
#define ENABLE_LOGGING_FILE
#define ENABLE_LOGGING_CONSOLE
#define ENABLE_DLL_THREAD_LOCAL_OPTIMIZATION
#include <string>
#define GUARD_EXPORT    __declspec(dllexport)
extern "C" {
extern GUARD_EXPORT void run_java(std::wstring javaPath, int argc, const char** argv);
}