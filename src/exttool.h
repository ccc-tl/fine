#ifdef FINE_ENABLE_EXTERNAL_TOOLS

#pragma once

#include "platform.h"

class ExtTool {
  public:
    ExtTool(const std::string& exe, const std::string& verifyArg = "", u32 verifyReturnValue = 0);
    bool works() const;
    u32 execute(const std::vector<std::string>& args, bool silent = false) const;

  private:
    const std::string exe;
    const std::string verifyArg;
    const u32 verifyReturnValue;
};

#endif
