#include "fine.h"

#include "spdlog/spdlog.h"
extern "C" {
#define WINVER _WIN32_WINNT
#include "xdelta3.h"
}

#include "platform.h"
#include "stream.h"

fs::path getBuildDirectory(const fs::path& base) {
    const fs::path& buildDir = base / "build";
    if (fs::exists(buildDir)) {
        spdlog::warn("Old build directory will be deleted");
        fs::remove_all(buildDir);
    }
    fs::create_directories(buildDir);
    spdlog::info("Build directory: '{}'", buildDir.u8string());
    return buildDir;
}

ByteBuffer readFile(const fs::path& path) {
    spdlog::debug("Read file '{}' into memory", path.u8string());
    Stream file(path);
    if (!file.good()) {
        bail("Failed to open file for reading");
    }
    ByteBuffer buf(file.length());
    if (!file.good()) {
        bail("Failed to read file");
    }
    file.readFully(buf);
    return buf;
}

void writeFile(const fs::path& path, ByteBuffer buf) {
    if (fs::exists(path)) {
        fs::resize_file(path, 0);
    } else {
        std::ofstream touch(path);
    }
    spdlog::debug("Write file '{}' from memory", path.u8string());
    Stream file(path);
    if (!file.good()) {
        bail("Failed to open file for writing");
    }
    file.writeFully(buf);
    if (!file.good()) {
        bail("Failed to write file");
    }
}

void copyFile(const fs::path& source, const fs::path& destination, std::function<void(usize, usize)> progressCallback) {
    spdlog::debug("Copy file '{}' -> '{}'", source.u8string(), destination.u8string());
    char buf[8192];

    FILE* src;
    FILE* dest;
#ifdef _WIN32
    std::wstring srcPath = convertToWstring(source.u8string());
    std::wstring destPath = convertToWstring(destination.u8string());
    src = _wfopen(srcPath.c_str(), L"rb");
    dest = _wfopen(destPath.c_str(), L"wb");
#else
    std::string srcPath = source.u8string();
    std::string destPath = destination.u8string();
    src = fopen(srcPath.c_str(), "rb");
    dest = fopen(destPath.c_str(), "wb");
#endif

    if (!src) {
        bail("Copy failed, failed to open source file");
    }
    if (!dest) {
        bail("Copy failed, failed to open destination file");
    }

    fseek(src, 0, SEEK_END);
    usize totalSize = ftell(src);
    fseek(src, 0, SEEK_SET);
    usize readSize;
    usize progressSize = 0;
    while ((readSize = fread(buf, 1, BUFSIZ, src))) {
        fwrite(buf, 1, readSize, dest);
        progressSize += readSize;
        progressCallback(progressSize, totalSize);
    }
    if (progressSize != totalSize) {
        bail("Copy failed, invalid number of bytes were copied");
    }
    fclose(src);
    fclose(dest);
}

static int MAX_BUFFER_SIZE = 64 * 1024 * 1024;

void createPatch(const fs::path& source, const fs::path& target, const fs::path& patch) {
    spdlog::debug("Create patch '{}' -> '{}', save patch to '{}'", source.u8string(), target.u8string(), patch.u8string());
    ByteBuffer sourceBuf = readFile(source);
    ByteBuffer targetBuf = readFile(target);
    ByteBuffer patchBuf(MAX_BUFFER_SIZE);
    u32 usedPatchSize;
    i32 result = xd3_encode_memory(targetBuf.data(), targetBuf.size(), sourceBuf.data(), sourceBuf.size(), patchBuf.data(),
                                   &usedPatchSize, patchBuf.size(), XD3_ADLER32);
    spdlog::trace("Xdelta result: {}", result);
    if (result != 0) {
        spdlog::error("Xdelta returned non zero result: {}", result);
        bail("Xdelta failed");
    }
    patchBuf.resize(usedPatchSize);
    writeFile(patch, patchBuf);
}

void applyPatch(const fs::path& source, const fs::path& target, const ByteBuffer patch) {
    spdlog::debug("Apply patch on '{}' -> '{}'", source.u8string(), target.u8string());
    ByteBuffer targetBuf = applyPatch(source, patch);
    writeFile(target, targetBuf);
}

ByteBuffer applyPatch(const fs::path& source, const ByteBuffer& patch) {
    spdlog::debug("Apply patch on '{}'", source.u8string());
    ByteBuffer sourceBuf = readFile(source);
    return applyPatch(sourceBuf, patch);
}

ByteBuffer applyPatch(const ByteBuffer& source, const ByteBuffer& patch) {
    return applyPatch(source.data(), source.size(), patch.data(), patch.size());
}

ByteBuffer applyPatch(const u8* source, usize sourceLen, const u8* patch, usize patchLen) {
    ByteBuffer outputBuf(MAX_BUFFER_SIZE);
    u32 usedOutBufSize;
    i32 result =
        xd3_decode_memory(patch, patchLen, source, sourceLen, outputBuf.data(), &usedOutBufSize, outputBuf.size(), 0);
    if (result != 0) {
        spdlog::error("Xdelta returned non zero result: {}", result);
        bail("Xdelta failed");
    }
    outputBuf.resize(usedOutBufSize);
    return outputBuf;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}
