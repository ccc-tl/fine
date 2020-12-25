#pragma once

#include "platform.h"

#include "stream.h"

class PatchFsEntry;

class PatchFsFile {
  public:
    PatchFsFile(const fs::path& path);
    i32 getFilesCount();
    i64 getFileLength(const std::string& name);
    ByteBuffer getFileContents(const std::string& name);

  private:
    Stream stream;
    std::pair<PatchFsFile*, PatchFsEntry*> getEntry(const std::string& name);
    ByteBuffer readContents(PatchFsEntry* entry);
    std::map<std::string, PatchFsEntry> index;
    std::vector<PatchFsFile> nestedFs;
};

class PatchFsEntry {
  public:
    PatchFsEntry(i64 nameOffset, i64 offset, i64 length) : nameOffset(nameOffset), offset(offset), length(length) {
    }
    const i64 nameOffset;
    const i64 offset;
    const i64 length;
};
