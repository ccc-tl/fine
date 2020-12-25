#include "fine.h"
#include "patchfs.h"
#include "platform.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

extern void moduleBootup(PatchFsFile& patchFs, const fs::path& input, const fs::path& output,
                         const std::vector<std::string>& args);

static void setupLogger() {
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("patcher-logs.txt", true);
    std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
    auto logger = std::make_shared<spdlog::logger>("", begin(sinks), end(sinks));
    logger->set_level(spdlog::level::debug);
    spdlog::set_default_logger(logger);
}

static void createPatch(const std::vector<std::string>& args) {
    auto sourcePath = fs::u8path(args[1]);
    auto targetPath = fs::u8path(args[2]);
    auto patchPath = fs::u8path(args[3]);
    if (!fs::exists(sourcePath)) {
        bail("Source does not exist");
    }
    if (!fs::exists(targetPath)) {
        bail("Target does not exist");
    }
    createPatch(sourcePath, targetPath, patchPath);
}

static void applyPatchFs(const std::vector<std::string>& args) {
    auto patchFsPath = fs::u8path(args[0]);
    auto inputPath = fs::u8path(args[1]);
    auto outputPath = fs::u8path(args[2]);
    if (std::find(args.begin(), args.end(), "--trace") != args.end()) {
        spdlog::default_logger()->set_level(spdlog::level::trace);
    }

    if (!fs::exists(patchFsPath)) {
        bail("PatchFS does not exist");
    }
    if (!fs::exists(inputPath)) {
        bail("Input does not exist");
    }
    // output not checked, it's job of the module because it depends on what module needs.

    spdlog::info("Open PatchFS: '{}'", args[0]);
    PatchFsFile patchFs(patchFsPath);
    if (patchFs.getFilesCount() == 0) {
        bail("PatchFS is empty or failed to open");
    } else {
        spdlog::debug("Opened PatchFS containing {} files", patchFs.getFilesCount());
    }
    auto extraArgs = std::vector(args.begin() + 3, args.end());
    moduleBootup(patchFs, inputPath, outputPath, extraArgs);
    spdlog::info("Patching completed");
}

static void fineBootup(const std::vector<std::string>& args) {
    try {
        spdlog::info("Init: setup logger");
        setupLogger();
        spdlog::info("Init: completed");
        spdlog::debug("Arguments: {}", args.size());
        for (const auto& arg : args) {
            spdlog::debug("Argument: '{}'", arg);
        }
        if (args.size() < 3) {
            bail("Invalid number of arguments. Please specify arguments: [patchFs] [input] [output] <--trace>");
        }
        if (args[0] == "-encode" && args.size() == 4) {
            createPatch(args);
        } else {
            applyPatchFs(args);
        }
    } catch (const std::runtime_error& e) {
        spdlog::critical("A critical error has occurred, the process will exit");
        auto msg = e.what();
        if (strlen(msg) > 0) {
            spdlog::critical("Here is some additional details to help you fix the issue:\n{}", msg);
        }
        exit(1);
    }
}

#if _WIN32
static void setupWorkingDirectory(const wchar_t* arg0) {
    spdlog::info("Init: setup working directory");
    auto dir = fs::weakly_canonical(fs::path(arg0)).parent_path();
    spdlog::info("Init: changing working directory to {}", convertFromWstring(dir));
    _wchdir(dir.c_str());
}

int wmain(int argc, const wchar_t* argv[]) {
    platformInit();
    if (argc <= 0) {
        bail("Argv 0 not provided");
    }
    setupWorkingDirectory(argv[0]);
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(convertFromWstring(argv[i]));
    }
    fineBootup(args);
    return 0;
}
#else
static void setupWorkingDirectory(const char* arg0) {
    spdlog::info("Init: setup working directory");
    auto dir = fs::weakly_canonical(fs::path(arg0)).parent_path();
    spdlog::info("Init: changing working directory to {}", dir);
    chdir(dir.c_str());
}

int main(int argc, const char* argv[]) {
    platformInit();
    if (argc <= 0) {
        bail("Argv 0 not provided");
    }
    setupWorkingDirectory(argv[0]);
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    fineBootup(args);
    return 0;
}
#endif
