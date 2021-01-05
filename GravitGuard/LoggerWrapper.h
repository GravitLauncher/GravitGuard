#pragma once
#include <fstream>
#include "GravitGuardAPI.h"
class LoggerWrapper
{
public:
	enum class logType : unsigned char
	{
		TRACE = 0, DEBUG = 1, INFO = 2, WARN = 3, CRASH = 4
	};
	inline void init(GravitGuardAPI& api)
	{
		toUtf8 = api.toUtf8;
		log = reinterpret_cast<decltype(log)>(api.log);
	}
	std::string (*toUtf8)(std::wstring& wstr);
	void (*log)(logType type, std::string_view module_name, std::string_view data);
	inline void logW(logType type, std::string_view module_name, std::wstring data)
	{
		return log(std::move(type), std::move(module_name), toUtf8(data));
	}
	template<typename... Args>
	inline void trace(Args... args)
	{
		return log(logType::TRACE, std::forward<Args>(args) ...);
	}
	template<typename... Args>
	inline void debug(Args... args)
	{
		return log(logType::DEBUG, std::forward<Args>(args) ...);
	}
	template<typename... Args>
	inline void info(Args... args)
	{
		return log(logType::INFO, std::forward<Args>(args) ...);
	}
	template<typename... Args>
	inline void warn(Args... args)
	{
		return log(logType::WARN, std::forward<Args>(args) ...);
	}
	template<typename... Args>
	inline void crash(Args... args)
	{
		return log(logType::CRASH, std::forward<Args>(args) ...);
	}
};

