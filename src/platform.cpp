#include "platform.h"

#include "spdlog/spdlog.h"
#include <locale>

#ifdef _WIN32
static void platformInitImpl() {
    SetConsoleOutputCP(CP_UTF8);
}

std::wstring convertToWstring(const std::string& str) {
    int numChars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
    std::wstring wstrTo;
    if (numChars) {
        wstrTo.resize(numChars);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstrTo[0], numChars);
    }
    return wstrTo;
}

std::string convertFromWstring(const std::wstring& wstr) {
    int numChars = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
    std::string strTo;
    if (numChars > 0) {
        strTo.resize(numChars);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), &strTo[0], numChars, NULL, NULL);
    }
    return strTo;
}
#else
static void platformInitImpl() {
    // nothing to do
}
#endif

void platformInit() {
    platformInitImpl();
    spdlog::info("Init: Platform init completed");
}

void bail(const std::string& msg) {
    spdlog::critical(msg);
    throw std::runtime_error("");
}
