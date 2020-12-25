#ifdef FINE_ENABLE_EXTERNAL_TOOLS

#include "exttool.h"

#include "spdlog/spdlog.h"

#ifdef _WIN32
#include <Windows.h>
#else
#error External tools are not implemented for this platform
#endif

ExtTool::ExtTool(const std::string& exe, const std::string& verifyArg, u32 verifyReturnValue)
    : exe(exe), verifyArg(verifyArg), verifyReturnValue(verifyReturnValue) {
}

bool ExtTool::works() const {
    spdlog::trace("Check if external tool works using argument '{}' expecting return code {}", verifyArg, verifyReturnValue);
    return execute(std::vector<std::string>{verifyArg}, true) == verifyReturnValue;
}

#ifdef _WIN32
u32 ExtTool::execute(const std::vector<std::string>& args, bool silent) const {
    std::stringstream execSs;
    execSs << exe << " ";
    for (auto ii = args.begin(); ii != args.end(); ii++) {
        execSs << "\"";
        execSs << (*ii);
        execSs << "\"";
        if (ii + 1 != args.end()) {
            execSs << " ";
        }
    }
    std::string execLine = execSs.str();
    std::wstring execLineW = convertToWstring(execLine);
    wchar_t* execLineWPtr = _wcsdup(execLineW.c_str());
    spdlog::debug("Execute external tool '{}'", execLine);

    DWORD flags = 0;
    if (silent) {
        flags = CREATE_NO_WINDOW;
    }

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    BOOL result = CreateProcessW(NULL, execLineWPtr, NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi);
    spdlog::trace("CreateProcess returned {}", result);
    if (!result) {
        free(execLineWPtr);
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);

    u32 exitCode;
    GetExitCodeProcess(pi.hProcess, (LPDWORD)&exitCode);
    spdlog::debug("Process exited with code {}", exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    free(execLineWPtr);
    return exitCode;
}
#endif

#endif
