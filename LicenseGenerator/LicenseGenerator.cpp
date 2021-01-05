// LicenseGenerator.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include "GravitGuardAPI.h"
#include <Windows.h>
#include <chrono>
#include "LoggerWrapper.h"
#include "Header.h"
#include <sstream>
//Include CPP file!
#include "base64.cpp"

std::string get_env(LPCSTR s)
{
	std::string appdata(1024, 'X');

	DWORD len = GetEnvironmentVariableA(s, &appdata[0], appdata.size());
	appdata.resize(len);
	return appdata;
}
char* ReadAllBytes(const char* filename, int* read)
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
LoggerWrapper logger;
int main()
{
	std::cout << "GravitGuard License Manager" << std::endl;
	std::cout << "This utility is freely distributed to all GravitGuard users." << std::endl;
	std::cout << "GravitGuard will now be initialized. If you do not have a license, the utility will not start" << std::endl;
	std::cout << "If you have a license - rename the utility EXE file to the same name that is specified in the license" << std::endl;
	initGuard();
	GravitGuardAPI api = getGuardAPI();
	api.licenseData(get_env("GUARD_SIGNATURE"));
	api.init();
	logger.init(api);
	std::cout << "Print License information" << std::endl;
	api.printLicense();
	std::cout << "1) Generate new License" << std::endl;
	std::cout << "2) Base64 to header" << std::endl;
	int num = 0;
	std::cin >> num;
	if (num == 1)
	{
		std::string username;
		std::cout << "Print your username: ";
		std::cin >> username;

		std::string pubkeyBase64;
		std::cout << "Print your publickey(base64): ";
		std::cin >> pubkeyBase64;

		int readed = 0;
		char* privatekey = ReadAllBytes("private.key", &readed);
		if (privatekey == nullptr)
		{
			std::string privKeyPath;
			std::cout << "Print vendor privatekey file path: ";
			std::cin >> privKeyPath;
			privatekey = ReadAllBytes(privKeyPath.c_str(), &readed);
			if (privatekey == nullptr)
			{
				std::cout << "File not found. Exit...";
				return -1;
			}
		}
		std::string rawPublicKey = base64_decode(pubkeyBase64);

		std::pair<char*, size_t> data = api.generate_license(username, rawPublicKey.data(), rawPublicKey.size(), privatekey);
		if (data.second != 0 && data.first != nullptr)
		{
			std::cout << "License generated successful" << std::endl;
			std::cout << "Base64 license: " << base64_encode((const unsigned char*) data.first, data.second) << std::endl;
			std::cout << "Save to file? (1 - yes, 2 - no)" << std::endl;
			std::cin >> num;
			if (num == 1)
			{
				std::fstream lic_file("license.dat", std::ios::binary | std::ios::out);
				lic_file.write(data.first, data.second);
				lic_file.close();
				std::cout << "Saved to license.dat" << std::endl;
			}
		}
	}
	else if (num == 2)
	{
		std::string base64;
		std::cout << "Print base64 data:" << std::endl;
		std::cin >> base64;
		std::string raw_data = base64_decode(base64);
		std::stringstream result;
		result << "{ ";
		bool t = false;
		for (unsigned char c : raw_data)
		{
			if (t)
			{
				result << ", ";
			}
			else t = true;
			result << "0x" << std::hex << (unsigned int) c;
		}
		result << " }";
		std::cout << "Output: " << result.str() << std::endl;
	}
	return 0;
	auto start = std::chrono::high_resolution_clock::now();
	short max_size;
	for (int i = 0; i < 1000; ++i)
	{
		void* ptrs[1024];
		short result = CaptureStackBackTrace(0, 1024, ptrs, NULL);
		max_size = result;
		for (int j = 0; j < result; ++j)
		{
			HMODULE mod;
			volatile BOOL result1 = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)ptrs[j], &mod);
		}
	}
	auto finish = std::chrono::high_resolution_clock::now();
	std::cout << max_size << " "<< std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count() << "ns\n";
	logger.debug("Benchmark", "stop");
	return 0;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
