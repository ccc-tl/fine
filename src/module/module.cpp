#include "../commons.h"

void moduleBootup(__attribute__((unused)) PatchFsFile& patchFs, __attribute__((unused)) const fs::path& input,
                  __attribute__((unused)) const fs::path& output, __attribute__((unused)) const std::vector<std::string>& args) {
    spdlog::debug("Running module code");
}
