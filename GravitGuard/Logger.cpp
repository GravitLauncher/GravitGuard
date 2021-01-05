#include "pch.h"
#include "Logger.h"
#include "windows.h"
#include <iostream>
#include <sstream>
#include "xorstr.h"
Logger::Logger()
{
	
}
void Logger::init()
{
#ifndef LOGGER_DISABLE_FILE_OUTPUT
	file.open(xorstr_("GravitGuard.log"), std::ios_base::out | std::ios_base::app);
#endif
	isInit = true;
}
Logger::~Logger()
{
	std::cout << std::flush;
#ifndef LOGGER_DISABLE_FILE_OUTPUT
	file << std::flush;
	file.close();
#endif
}
std::string Logger::toUtf8(std::wstring& wstr)
{
	size_t size = wstr.size()*2;
	std::string appdata(size, ' ');
	size_t nCnt = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, appdata.data(), size, 0, 0);
	appdata.resize(nCnt-1);
	return appdata;
}
void Logger::log(logType type, std::string_view module_name, std::string_view data)
{
	if (!isInit) return;
	const char* s_type;
	switch (type)
	{
	case logType::TRACE:
		s_type = xorstr_("TRACE");
		break;
	case logType::DEBUG:
		s_type = xorstr_("DEBUG");
		break;
	case logType::INFO:
		s_type = xorstr_("INFO");
		break;
	case logType::WARN:
		s_type = xorstr_("WARN");
		break;
	case logType::CRASH:
		s_type = xorstr_("CRASH");
		break;
	default:
		s_type = xorstr_("UNKNO");
		break;
	}
	SYSTEMTIME systime;
	GetLocalTime(&systime);
	std::stringstream ss;
	ss << "[" << systime.wDay << "." << systime.wMonth << "." << systime.wYear << " "
		<< systime.wHour << ":" << systime.wMinute << ":" << systime.wSecond << ":" << systime.wMilliseconds
		<< "]\t[" << s_type << "]\t[" << module_name << "]\t" << data << std::endl;
	std::string s = std::move(ss).str();
#ifndef LOGGER_DISABLE_STD_OUTPUT
	std::cout << s;
#endif
#ifndef LOGGER_DISABLE_FILE_OUTPUT
	file << std::move(s);
#endif
}

void Logger::flush()
{
#ifndef LOGGER_DISABLE_STD_OUTPUT
	std::cout << std::flush;
#endif
#ifndef LOGGER_DISABLE_FILE_OUTPUT
	file << std::flush;
#endif
}
