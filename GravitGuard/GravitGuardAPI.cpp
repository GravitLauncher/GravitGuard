#include "pch.h"
#include "GravitGuardAPI.h"
#include <iostream>
#include "MinHook.h"
#include "GuardDetour.h"
#include "MagicFunctionTable.h"
#include "GravitGuard.h"
#include "Logger.h"
#define MAGIC_API 0x4D
MagicFunctionTable apiTable(10, MAGIC_API);
GravitGuardAPI GUARD_EXPORT getGuardAPI()
{
	return apiTable.callAndCorrupt<GravitGuardAPI>(1 ^ MAGIC_API);
}
GravitGuardAPI getGuardAPI0()
{
	GravitGuardAPI api;
	//api.test = apiTable.callAndCorrupt<decltype(api.test)>(0 ^ MAGIC_API);
	api.init = apiTable.callAndCorrupt<decltype(api.init)>(2 ^ MAGIC_API);
	api.log = apiTable.callAndCorrupt<decltype(api.log)>(3 ^ MAGIC_API);
	api.toUtf8 = apiTable.callAndCorrupt<decltype(api.toUtf8)>(4 ^ MAGIC_API);
	api.jvmCreatedCallback = apiTable.callAndCorrupt<decltype(api.jvmCreatedCallback)>(5 ^ MAGIC_API);
	api.printLicense = apiTable.callAndCorrupt<decltype(api.printLicense)>(6 ^ MAGIC_API);
	api.generate_license = apiTable.callAndCorrupt<decltype(api.generate_license)>(7 ^ MAGIC_API);
	api.licenseData = apiTable.callAndCorrupt<decltype(api.licenseData)>(8 ^ MAGIC_API);
	return api;
}
void init0()
{
	guard.init();
}
void log0(unsigned char type, std::string_view module_name, std::string_view data)
{
	logger.log(static_cast<Logger::logType>(type), module_name, data);
}
std::string toUtf0(std::wstring& wstr)
{
	return logger.toUtf8(wstr);
}
void callbackJVMCreated(void* jvm_ptr, void* jvm_env)
{
	guard.jvmCreated(jvm_ptr, jvm_env);
}
void printLicense()
{
	guard.license.printLicense(guard.license.currentLicense);
}
std::pair<char*, size_t> generate_license(std::string username, const char* pubkey_data, unsigned int pubkey_size, char* privatekey)
{
	return guard.license.generate_license(std::move(username), pubkey_data, pubkey_size, privatekey);
}
void licenseData(std::string key)
{
	guard.license.licenseKey = key;
}
decltype(GravitGuardAPI::init) getInit()
{
	return &init0;
}
decltype(GravitGuardAPI::log) getLog()
{
	return &log0;
}
decltype(GravitGuardAPI::toUtf8) getLogToUtf()
{
	return &toUtf0;
}
decltype(GravitGuardAPI::jvmCreatedCallback) getJvmCreatedCallback()
{
	return &callbackJVMCreated;
}
decltype(GravitGuardAPI::printLicense) getPrintLicenseCallback()
{
	return &printLicense;
}
decltype(GravitGuardAPI::generate_license) getGenerateLicense()
{
	return &generate_license;
}
decltype(GravitGuardAPI::licenseData) getLicenseData()
{
	return &licenseData;
}
void GUARD_EXPORT initGuard()
{
	//TODO: ����������
	apiTable.write(1, &getGuardAPI0);
	apiTable.write(2, &getInit);
	apiTable.write(3, &getLog);
	apiTable.write(4, &getLogToUtf);
	apiTable.write(5, &getJvmCreatedCallback);
	apiTable.write(6, &getPrintLicenseCallback);
	apiTable.write(7, &getGenerateLicense);
	apiTable.write(8, &getLicenseData);
}