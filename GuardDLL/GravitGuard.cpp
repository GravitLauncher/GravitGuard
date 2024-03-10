#include "pch.h"
#include "GravitGuard.h"
#include <unordered_map>
#include "xorstr.hpp"
#include <iostream>
#include "Logger.h"

void blackhole_none() {

}

std::wstring getWorkDirectory() {
	std::wstring appdata(1024, 'X');
	DWORD len = GetCurrentDirectoryW(appdata.size(), &appdata[0]);
	if (len <= 0) {
		return L"";
	}
	appdata.resize(len);
	return appdata;
}

std::wstring getWindowsDirectory() {
	std::wstring appdata(1024, 'X');
	DWORD len = GetWindowsDirectoryW(&appdata[0], appdata.size());
	if (len <= 0) {
		return L"";
	}
	appdata.resize(len);
	return appdata;
}

std::wstring GravitGuard::getModuleFileName(HMODULE module) {
	thread_local std::unordered_map<HMODULE, std::wstring> cache;
	if (cache.contains(module)) {
		return cache[module];
	}
	std::wstring appdata(1024, 'X');
	DWORD len = GetModuleFileNameW(module, &appdata[0], appdata.size());
	if (len <= 0) {
		return L"";
	}
	appdata.resize(len);
	return appdata;
}

GravitGuard::GravitGuard(std::wstring java_home) : java_home(java_home)
{
	work_dir = getWorkDirectory();
	windows_dir = getWindowsDirectory();
	getProcAddress = &GetProcAddress;
	module_ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
	module_kernel32 = GetModuleHandleA(xorstr_("kernel32.dll"));
	module_kernelbase = GetModuleHandleA(xorstr_("kernelbase.dll"));
	{
		auto success = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&blackhole_none, &self_module);
	}
	known_modules_mutex = new std::mutex();
	initHooks();
}
thread_local int trusted_context = 0;
inline bool GravitGuard::is_trusted_context()
{
	return trusted_context > 0;
}

void GravitGuard::enter_trusted_context()
{
	trusted_context++;
}

void GravitGuard::exit_trusted_context()
{
	trusted_context--;
}

std::optional<std::string> GravitGuard::get_env(std::string_view name)
{
	std::string tmp(name);
	std::string appdata(1024, 'X');
	DWORD len = GetEnvironmentVariableA(tmp.c_str(), &appdata[0], appdata.size());
	if (len <= 0) {
		return std::nullopt;
	}
	appdata.resize(len);
	return appdata;
}

#ifdef GUARD_DEBUG
std::wstring to_string(GravitGuard::DLLSource value) {
	switch (value) {
	case GravitGuard::DLLSource::APP_HOME: {
		return L"APP_HOME";
	}
	case GravitGuard::DLLSource::JAVA_HOME: {
		return L"JAVA_HOME";
	}
	case GravitGuard::DLLSource::OTHER: {
		return L"OTHER";
	}
	case GravitGuard::DLLSource::SELF: {
		return L"SELF";
	}
	case GravitGuard::DLLSource::SYSTEM: {
		return L"SYSTEM";
	}
	case GravitGuard::DLLSource::UNKNOWN_MEMORY: {
		return L"UNKNOWN_MEMORY";
	}
	}
}
#endif

GravitGuard::CheckResultAll GravitGuard::check(DLLSource source)
{
	thread_local void* buf[1024];
	unsigned long int hash;
	if (is_trusted_context()) {
		return CheckResultAll{ CheckResult::PASS };
	}
	TrustedContextLock _l();
	auto n = captureStackTrace(buf, 1024, &hash);
	if (n <= 0) {
		return CheckResultAll{ CheckResult::CANCEL };
	}
#ifdef GUARD_DEBUG
	logger.log(Logger::Level::TRACE, L"=== Check Stacktrace ===");
#endif
	bool isOk = false;
	bool found_unknown_module = false;
	bool suspect = false;
	for (int i = 0; i < n; i++) {
		StackTraceElement el;
		el.address = buf[i];
		HMODULE module;
		bool this_element_in_unknown_module = false;
		auto found = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)buf[i], &module);
		if (!found) { // JIT?
			el.source = DLLSource::UNKNOWN_MEMORY;
			if (i != n - 1) {
				isOk = false;
			}
		}
		else if (module == self_module) {
			el.source = DLLSource::SELF;
		}
		else {
			el.fileName = getModuleFileName(module);
			el.source = getSource(el.fileName);
			if (!is_known_module(el.fileName)) {
				std::wstring_view view = el.fileName;
				if (el.source == DLLSource::SYSTEM) {
					if (view.ends_with(L"\\ucrtbase.dll")) { // ucrtbase.dll injected by system without LdrLoadDll
						gg->add_known_module(el.fileName);
					}
					else {
						//suspect = true;
					}
				}
				else {
					found_unknown_module = true;
					this_element_in_unknown_module = true;
				}
			}
			if (!is_win_internal_module(module)) {
				isOk = true;
			}
		}
#ifdef GUARD_DEBUG
		logger.log(Logger::Level::TRACE, L">>> {}\t{}\t{}",el.address, to_string(el.source), el.fileName);
#endif
	}
#ifdef GUARD_DEBUG
	logger.log(Logger::Level::TRACE, L"=======================");
#endif
	if (found_unknown_module) {
#ifdef GUARD_DEBUG
		logger.log(Logger::Level::DEBUG, L"Terminate (unknown module)");
#endif
		return CheckResultAll{ CheckResult::TERMINATE_APP };
	}
	if (isOk) {
		if (suspect) {
			return CheckResultAll{ CheckResult::ONLY_SIGNED };
		}
		return CheckResultAll{ CheckResult::PASS };
	}
	else {
		return CheckResultAll{ CheckResult::CANCEL };
	}
}

size_t GravitGuard::captureStackTrace(void** buf, size_t size, unsigned long int* hash)
{
	size_t num;
	int counter = 0;
	do {
		num = CaptureStackBackTrace(2, size, buf, hash);
		counter++;
	} while (num <= 0 && counter < 100);
	return num;
}

GravitGuard::DLLSource GravitGuard::getSource(std::wstring_view view)
{
	if (view.starts_with(java_home)) {
		return DLLSource::JAVA_HOME;
	}
	if (view.starts_with(work_dir)) {
		return DLLSource::APP_HOME;
	}
	if (view.starts_with(windows_dir)) {
		return DLLSource::SYSTEM;
	}
	return DLLSource::OTHER;
}

void GravitGuard::set_jvm_module(HMODULE module)
{
	module_jvm = module;
}

thread_local std::list<std::wstring> local_known;
bool GravitGuard::is_known_module(std::wstring_view module)
{
	for (auto a : local_known) {
		if (a == module) {
			return true;
		}
	}
	known_modules_mutex->lock();
	for (auto a : known_modules) {
		if (a == module) {
			local_known.push_back(std::wstring(module));
			known_modules_mutex->unlock();
			return true;
		}
	}
	known_modules_mutex->unlock();
	return false;
}

void GravitGuard::add_known_module(std::wstring_view module)
{
	known_modules_mutex->lock();
	known_modules.push_back(std::wstring(module));
	known_modules_mutex->unlock();
}

inline TrustedContextLock::TrustedContextLock()
{
	trusted_context++;
}

inline TrustedContextLock::~TrustedContextLock()
{
	trusted_context--;
}
