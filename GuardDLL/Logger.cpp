#include "pch.h"
#include "guard.h"
#include <format>
#ifdef ENABLE_LOGGING_CONSOLE
#include <iostream>
#endif
#ifdef ENABLE_LOGGING_FILE
#include <fstream>
#endif
#include "Logger.h"

Logger logger;
#ifdef ENABLE_LOGGING_FILE
std::fstream* logfile;
#endif

Logger::Logger()
{
#ifdef ENABLE_LOGGING_FILE
	logfile = new std::fstream(L"GravitGuard2.log", std::ios_base::app | std::ios_base::out);
#endif
}

Logger::~Logger()
{
#ifdef ENABLE_LOGGING_FILE
	delete logfile;
#endif
}

std::string Logger::toUtf8(std::wstring_view wstr)
{
	size_t size = wstr.size() * 2;
	std::string appdata(size, ' ');
	size_t nCnt = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr.size(), appdata.data(), size, 0, 0);
	appdata.resize(nCnt);
	return appdata;
}

void Logger::log(std::string_view str)
{
#ifdef ENABLE_LOGGING_CONSOLE
	std::cout << str << std::endl;
#endif
#ifdef ENABLE_LOGGING_FILE
	*logfile << str << std::endl;
#endif
}

void Logger::log(std::wstring_view str)
{
#ifdef ENABLE_LOGGING_CONSOLE
	std::wcout << str << std::endl;
#endif
#ifdef ENABLE_LOGGING_FILE
	*logfile << toUtf8(str) << std::endl;
#endif
}
