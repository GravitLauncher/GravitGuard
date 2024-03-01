#pragma once
#include <string>
#define GUARD_EXPORT    __declspec(dllexport)
extern "C" {
extern GUARD_EXPORT void run_java(std::wstring javaPath, int argc, const char** argv);
}