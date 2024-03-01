#pragma once
#include <format>
#include <chrono>
class Logger
{
public:
	enum class Level {
		TRACE, DEBUG, INFO, CRITICAL
	};
	template<class... T>
	void log(Level level, const std::format_string<T...> _Fmt,T... _Args);
	template<class... T>
	void log(Level level, const std::wformat_string<T...> _Fmt, T... _Args);
private:
	void log(std::string_view str);
	void log(std::wstring_view str);
	inline std::string_view get_level_str(Level lvl) {
		switch (lvl) {
		case Level::TRACE:
			return "TRACE";
		case Level::DEBUG:
			return "DEBUG";
		case Level::INFO:
			return "INFO";
		case Level::CRITICAL:
			return "CRITICAL";
		default:
			return "UNKNOWN";
		}
	}
	inline std::wstring_view get_level_wstr(Level lvl) {
		switch (lvl) {
		case Level::TRACE:
			return L"TRACE";
		case Level::DEBUG:
			return L"DEBUG";
		case Level::INFO:
			return L"INFO";
		case Level::CRITICAL:
			return L"CRITICAL";
		default:
			return L"UNKNOWN";
		}
	}
};

template<class ...T>
inline void Logger::log(Level level, const std::format_string<T...> _Fmt, T..._Args)
{
	auto const time = std::chrono::system_clock::now();
	auto const content = std::format(_Fmt, std::forward<T>(_Args)...);
	log(std::format("[{:%Y-%m-%d %X}][{}] {}", time, get_level_str(level), content));
}

template<class ...T>
inline void Logger::log(Level level, const std::wformat_string<T...> _Fmt, T..._Args)
{
	auto const time = std::chrono::system_clock::now();
	auto const content = std::format(_Fmt, std::forward<T>(_Args)...);
	log(std::format(L"[{:%Y-%m-%d %X}][{}] {}", time, get_level_wstr(level), content));
}

extern Logger logger;