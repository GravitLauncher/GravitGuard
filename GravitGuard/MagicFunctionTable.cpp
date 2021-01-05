#include "pch.h"
#include "MagicFunctionTable.h"

MagicFunctionTable::MagicFunctionTable(size_t size, size_t xor_param) : //-V730
	size(size), xor_param(xor_param)
{
	raw_functions = new void* [size];
}

MagicFunctionTable::~MagicFunctionTable()
{
	delete[] raw_functions;
}
