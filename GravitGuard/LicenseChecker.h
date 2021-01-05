#pragma once
#include <string>
#pragma pack(push, 1)
struct LicenseHeader
{
	unsigned long long int license_magic = 0xBFFEBFFE00112233;
	unsigned short int version = 1;
	unsigned short int username_len = 0;
	unsigned int pubkey_len = 0;
	unsigned short product_id = 0;
	unsigned short vendor_id = 0;
};
#pragma pack(pop)
struct License
{
	LicenseHeader header;
	std::string username;
	std::string pubKey;
	bool valid_sign;
};
class LicenseChecker
{
public:
	LicenseChecker();
	License currentLicense;
	std::string licenseKey;
	void printLicense(License& license);
	License parse_license(const char* data, unsigned int data_size);
	void readCurrentLicense(std::wstring path);
	std::pair<char*, size_t> generate_license(std::string username, const char* pubkey_data, unsigned int pubkey_size, char* privatekey);
	boolean checkLicenseKey();
	inline boolean checkCurrentLicense()
	{

		return currentLicense.valid_sign && checkLicenseKey();
	}
};

