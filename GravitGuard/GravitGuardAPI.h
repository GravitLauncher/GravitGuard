#pragma once
#include "Config.h"
#include <string>
class GravitGuardAPI
{
public:
	void (* volatile test)();
	void (* volatile init)();
	std::string(* volatile toUtf8)(std::wstring& wstr);
	void (* volatile log)(unsigned char type, std::string_view module_name, std::string_view data);
	void (* volatile jvmCreatedCallback)(void* jvm_ptr, void* jvm_env);
	// License data
	void (* volatile printLicense)();
	std::pair<char*, size_t>(* volatile generate_license)(std::string username, const char* pubkey_data, unsigned int pubkey_size, char* privatekey);
	void (* volatile licenseData)(std::string key);
};

extern "C"  GravitGuardAPI GUARD_EXPORT getGuardAPI();
extern "C"  void GUARD_EXPORT initGuard();