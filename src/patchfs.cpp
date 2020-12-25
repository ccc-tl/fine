#include "patchfs.h"

#include "spdlog/spdlog.h"

PatchFsFile::PatchFsFile(const fs::path& path) : stream(path) {
    if (!stream.good()) return;
    std::string magic = stream.readString();
    if (magic != "PATCHFS") return;
    i32 fileCount = stream.readInt();
    i32 nestedCount = stream.readInt();
    std::vector<i64> nestedPtrs;
    stream.seek(0x10);
    for (int i = 0; i < nestedCount; i++) {
        nestedPtrs.push_back(stream.readLong());
    }
    std::vector<PatchFsEntry> entries;
    for (int i = 0; i < fileCount; i++) {
        i64 namePtr = stream.readLong();
        i64 contentPtr = stream.readLong();
        i64 length = stream.readLong();
        stream.readLong();
        PatchFsEntry entry(namePtr, contentPtr, length);
        entries.emplace_back(entry);
    }
    for (const PatchFsEntry& entry : entries) {
        stream.seek(entry.nameOffset);
        std::string name = stream.readString();
        index.insert(std::make_pair(name, entry));
    }
    for (const i64 nestedPtr : nestedPtrs) {
        stream.seek(nestedPtr);
        std::string name = stream.readString();
        fs::path nestedPath = path.parent_path() / name.c_str();
        spdlog::info("Open nested PatchFS: '{}'", nestedPath.u8string());
        nestedFs.emplace_back(nestedPath);
    }
}

i32 PatchFsFile::getFilesCount() {
    auto count = index.size();
    for (auto& nested : nestedFs) {
        count += nested.getFilesCount();
    }
    return count;
}

i64 PatchFsFile::getFileLength(const std::string& name) {
    auto entry = index.find(name);
    if (entry != index.end()) return entry->second.length;
    for (auto& nestedFile : nestedFs) {
        auto nestedEntry = nestedFile.getFileLength(name);
        if (nestedEntry != -1) return nestedEntry;
    }
    return -1;
}

ByteBuffer PatchFsFile::getFileContents(const std::string& name) {
    spdlog::trace("Get PatchFS entry for: '{}'", name);
    auto entryPair = getEntry(name);
    if (entryPair.first == nullptr || entryPair.second == nullptr) {
        spdlog::critical("Missing PatchFS entry for: '{}'", name);
        bail("Missing PatchFS entry");
    }
    return entryPair.first->readContents(entryPair.second);
}

std::pair<PatchFsFile*, PatchFsEntry*> PatchFsFile::getEntry(const std::string& name) {
    auto entry = index.find(name);
    if (entry != index.end()) return std::make_pair(this, &(entry->second));
    for (auto& nestedFile : nestedFs) {
        auto nestedEntry = nestedFile.getEntry(name);
        if (nestedEntry.first != nullptr && nestedEntry.second != nullptr) {
            return nestedEntry;
        }
    }
    return std::make_pair(nullptr, nullptr);
}

ByteBuffer PatchFsFile::readContents(PatchFsEntry* entry) {
    ByteBuffer data(entry->length);
    stream.seek(entry->offset);
    stream.readFully(data.data(), entry->length);
    return data;
}
