#pragma once

#include "platform.h"

fs::path getBuildDirectory(const fs::path& base);

ByteBuffer readFile(const fs::path& path);
void writeFile(const fs::path& path, ByteBuffer buf);
void copyFile(const fs::path& source, const fs::path& destination, std::function<void(usize, usize)> progressCallback);

void createPatch(const fs::path& target, const fs::path& source, const fs::path& patch);
void applyPatch(const fs::path& source, const fs::path& target, const ByteBuffer patch);
ByteBuffer applyPatch(const fs::path& source, const ByteBuffer& patch);
ByteBuffer applyPatch(const ByteBuffer& source, const ByteBuffer& patch);
ByteBuffer applyPatch(const u8* source, usize sourceLen, const u8* patch, usize patchLen);

bool endsWith(const std::string& str, const std::string& suffix);
