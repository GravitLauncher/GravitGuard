#pragma once
#include <utility>
class MagicFunctionTable
{
private:
	void** raw_functions;
	volatile void* strange_data[2];
	size_t xor_param;
	size_t size;
public:
	MagicFunctionTable(size_t size, size_t xor_param);
	template<typename R, typename... Args>
	inline R call(size_t pos, Args... args)
	{
		using type = R(*)(Args &&... args);
		size_t real_pos = pos ^ xor_param;
#ifdef GUARD_DEBUG_MODE
		if (real_pos > size) throw real_pos;
#endif
		return ((type)raw_functions[real_pos])(std::forward<Args>(args) ... );
	}
	template<typename R, typename... Args>
	inline R callAndCorrupt(size_t pos, Args... args)
	{
		using type = R(*)(Args &&... args);
		size_t real_pos = pos ^ xor_param;
#ifdef GUARD_DEBUG_MODE
		if (real_pos > size) throw real_pos;
#endif
		type func = reinterpret_cast<type>(raw_functions[real_pos]);
		raw_functions[real_pos] = nullptr;
		return func(std::forward<Args>(args) ... );
	}
	template<typename T>
	inline bool write(size_t pos, T func)
	{
		raw_functions[pos] = reinterpret_cast<void*>(func);
		return true;
	}
	~MagicFunctionTable();
};

