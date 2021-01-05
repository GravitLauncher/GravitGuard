#pragma once
#include "Config.h"
#include <string>
class GravitGuardAPI
{
public:
	void (*test)();
	void (*init)();
	std::string(*toUtf8)(std::wstring& wstr);
	void (*log)(unsigned char type, std::string_view module_name, std::string_view data);
	void (*jvmCreatedCallback)(void* jvm_ptr, void* jvm_env);
	// License data
	void (*printLicense)();
	std::pair<char*, size_t>(*generate_license)(std::string username, const char* pubkey_data, unsigned int pubkey_size, char* privatekey);
	void (*licenseData)(std::string key);
};

extern "C"  GravitGuardAPI GUARD_EXPORT getGuardAPI();
extern "C"  void GUARD_EXPORT initGuard();