#pragma once
#include <string>
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);
std::string base64_decode(std::string const& encoded_string);
int char2int(char input);
void hex2bin(const std::string& src, std::string& target);