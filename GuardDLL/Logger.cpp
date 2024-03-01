#include "pch.h"
#include <format>
#include <iostream>
#include "Logger.h"

Logger logger;

void Logger::log(std::string_view str)
{
	std::cout << str << std::endl;
}

void Logger::log(std::wstring_view str)
{
	std::wcout << str << std::endl;
}
