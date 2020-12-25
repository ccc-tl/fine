#pragma once

#include "iso9660.h"
#include "platform.h"

ByteBuffer stashIsoPrimaryDescriptor(Stream& iso);
void restoreIsoPrimaryDescriptor(Stream& iso, ByteBuffer descriptor);

std::vector<IsoDirectoryRecord> getIsoRecords(const fs::path& isoPath);
IsoDirectoryRecordEntry seekToIsoFile(Stream& iso, const std::vector<IsoDirectoryRecord>& records,
                                      const std::string relPath);

void patchIsoFile(Stream& iso, const std::vector<IsoDirectoryRecord>& records, const std::string relPath,
                  const ByteBuffer& patch);
void relocateIsoFile(Stream& srcIso, Stream& destIso, const std::vector<IsoDirectoryRecord>& records,
                     const std::string relPath);
