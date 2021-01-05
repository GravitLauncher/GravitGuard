#include "pch.h"
#include "LicenseChecker.h"
#include "Config.h"
//extern "C" {
//#define SODIUM_STATIC
//#include <sodium.h>
//}
#define crypto_sign_BYTES 64
#include <string>
#include <cstring>
#include "utils.h"
#include "Logger.h"
#include "xorstr.h"
//static unsigned char pk[crypto_sign_PUBLICKEYBYTES] = { 0xfb, 0x97, 0xfa, 0x5a, 0x00, 0x50, 0x21, 0x53, 0xb2, 0x41, 0x25, 0x05, 0xc0, 0xca, 0x85, 0x4a, 0x33, 0xb7, 0x62, 0x2e, 0x19, 0x3f, 0x9f, 0x5b, 0x3c, 0x41, 0xe8, 0x3b, 0xb4, 0x76, 0xad, 0x71};
static unsigned char license_key[16] = { 0x4b, 0x9d, 0xd7, 0x90, 0xf2, 0x82, 0xf5, 0xce, 0x72, 0xb0, 0x13, 0x38, 0x7d, 0xb8, 0x83, 0x6d };
License LicenseChecker::parse_license(const char* data, unsigned int data_size)
{
	License license;
	license.header = *((const LicenseHeader*)data);
	license.username = std::string(&data[sizeof(LicenseHeader)], license.header.username_len);
	license.pubKey = std::string(&data[sizeof(LicenseHeader) + license.header.username_len], license.header.pubkey_len);
	unsigned int sigOffset = sizeof(LicenseHeader) + license.header.username_len + license.header.pubkey_len;
	//int ret = crypto_sign_verify_detached((unsigned char*)&data[sigOffset], (unsigned char*)&data[0], sigOffset, (unsigned char*)&pk);
	int ret = 0;
	license.valid_sign = ret == 0;
#ifdef GUARD_DEBUG_MODE
	logger.debug(xorstr_("LicCheckSign"), license.valid_sign ? xorstr_("true") : xorstr_("false"));
#endif
	return license;
}
char* ReadAllBytes(std::wstring filename, int* read)
{
	std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
	if (!ifs.good()) return nullptr;
	std::ifstream::pos_type pos = ifs.tellg();

	// What happens if the OS supports really big files.
	// It may be larger than 32 bits?
	// This will silently truncate the value/
	int length = pos;

	// Manuall memory management.
	// Not a good idea use a container/.
	char* pChars = new char[length];
	ifs.seekg(0, std::ios::beg);
	ifs.read(pChars, length);

	// No need to manually close.
	// When the stream goes out of scope it will close the file
	// automatically. Unless you are checking the close for errors
	// let the destructor do it.
	ifs.close();
	*read = length;
	return pChars;
}
void LicenseChecker::readCurrentLicense(std::wstring path)
{
	int length = 0;
	char* licBytes = ReadAllBytes(path, &length);
	if (licBytes == nullptr) return;
	currentLicense = parse_license(licBytes, length);
}
LicenseChecker::LicenseChecker()
{
	//if (sodium_init() == -1) {
	//	throw std::exception("7201");
	//}
}
void LicenseChecker::printLicense(License& license)
{
#ifdef GUARD_DEBUG_MODE
	logger.info(xorstr_("License"), xorstr_("Name:") + license.username);
	logger.info(xorstr_("License"), xorstr_("Product:") + std::to_string(license.header.product_id));
	logger.info(xorstr_("License"), xorstr_("Vendor:") + std::to_string(license.header.vendor_id));
	logger.info(xorstr_("License"), std::string(xorstr_("Signed:")) + ( license.valid_sign ? xorstr_("true") : xorstr_("false") ));
#endif
}

std::pair<char*, size_t> LicenseChecker::generate_license(std::string username, const char* pubkey_data, unsigned int pubkey_size, char* privatekey)
{
	LicenseHeader header;
	header.username_len = username.size();
	header.pubkey_len = pubkey_size;
	header.product_id = 6;
	size_t buf_size = sizeof(header) + header.username_len + pubkey_size + crypto_sign_BYTES;
	char* buf = new char[buf_size];
	memcpy(buf, (char*)&header, sizeof(header));
	memcpy(&buf[sizeof(header)], username.data(), username.size());
	memcpy(&buf[sizeof(header) + username.size()], pubkey_data, pubkey_size);
	int sigOffset = buf_size - crypto_sign_BYTES;
	//char ppk[crypto_sign_PUBLICKEYBYTES];
	//crypto_sign_keypair((unsigned char*)&ppk[0], (unsigned char*)&privatekey[0]);
	//int ret = crypto_sign_detached((unsigned char*)&buf[sigOffset], NULL, (unsigned char*)&buf[0], sigOffset, (unsigned char*)&privatekey[0]);
	int ret = 0;
	//ret = crypto_sign_verify_detached((unsigned char*)&buf[sigOffset], (unsigned char*)&buf[0], sigOffset, (unsigned char*)&pk[0]);
	return std::make_pair(buf, buf_size);
}

boolean LicenseChecker::checkLicenseKey()
{
	std::string validLicenseKey = base64_decode(licenseKey);
#ifdef GUARD_DEBUG_MODE
	logger.debug(xorstr_("LicenseKey"), licenseKey);
#endif
	//return crypto_sign_verify_detached((unsigned char*)&validLicenseKey.data()[0], (unsigned char*)&license_key[0], 16, (unsigned char*)&currentLicense.pubKey.data()[0]) == 0;
	return true;
}
