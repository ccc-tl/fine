#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <map>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

namespace fs = std::filesystem;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef size_t usize;
typedef std::vector<u8> ByteBuffer;

void platformInit();

#ifdef _WIN32
std::wstring convertToWstring(const std::string& str);
std::string convertFromWstring(const std::wstring& wstr);
#endif

void bail(const std::string& msg);
