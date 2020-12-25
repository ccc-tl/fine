#include "isoutils.h"

#include "fine.h"
#include "spdlog/spdlog.h"

ByteBuffer stashIsoPrimaryDescriptor(Stream& iso) {
    spdlog::trace("Remove ISO primary volume descriptor");
    ByteBuffer blank(ISO_SECTOR_SIZE);
    ByteBuffer descriptor(ISO_SECTOR_SIZE);
    iso.seek(16 * ISO_SECTOR_SIZE);
    iso.readFully(descriptor);
    iso.seek(16 * ISO_SECTOR_SIZE);
    iso.writeFully(blank);
    return descriptor;
}

void restoreIsoPrimaryDescriptor(Stream& iso, ByteBuffer descriptor) {
    spdlog::trace("Restore ISO primary volume descriptor");
    iso.seek(16 * ISO_SECTOR_SIZE);
    iso.writeFully(descriptor);
}

std::vector<IsoDirectoryRecord> getIsoRecords(const fs::path& iso) {
    spdlog::trace("Read ISO records: '{}'", iso.u8string());
    Iso9660Reader reader(iso);
    return reader.getRecords();
}

IsoDirectoryRecordEntry seekToIsoFile(Stream& iso, const std::vector<IsoDirectoryRecord>& records,
                                      const std::string relPath) {
    spdlog::trace("Seek to ISO file: '{}'", relPath);
    for (const auto& record : records) {
        for (const auto& entry : record.getEntries()) {
            if (entry.relPath == relPath && entry.atributes == 0) {
                spdlog::trace("Found file at LBA: {}", entry.lba);
                iso.seek(entry.lba * ISO_SECTOR_SIZE);
                return entry;
            }
        }
    }
    spdlog::error("ISO file: '{}' was not found", relPath);
    bail("Seek to ISO file failed");
    __builtin_unreachable();
}

void patchIsoFile(Stream& iso, const std::vector<IsoDirectoryRecord>& records, const std::string relPath,
                  const ByteBuffer& patch) {
    spdlog::trace("Patch ISO file: '{}'", relPath);
    auto record = seekToIsoFile(iso, records, relPath);
    ByteBuffer source(record.length);
    iso.readFully(source);
    ByteBuffer patched = applyPatch(source, patch);
    if (patched.size() / ISO_SECTOR_SIZE > source.size() / ISO_SECTOR_SIZE) {
        spdlog::error("ISO file '{}' won't fit in original place after patching", relPath);
        bail("Failed to patch ISO file in-place");
    }
    iso.seek(record.lba * ISO_SECTOR_SIZE);
    iso.writeFully(patched);
    u32 sizeDiff = source.size() - patched.size();
    if (sizeDiff > 0) {
        ByteBuffer blank(sizeDiff);
        iso.writeFully(blank);
    }
    iso.seek(record.isoOffset + 2 + 8);
    iso.writeInt(patched.size());
    iso.writeIntB(patched.size());
}

void relocateIsoFile(Stream& srcIso, Stream& destIso, const std::vector<IsoDirectoryRecord>& records,
                     const std::string relPath) {
    spdlog::trace("Relocate ISO file: '{}'", relPath);
    auto srcRecord = seekToIsoFile(srcIso, records, relPath);
    ByteBuffer source(srcRecord.length);
    srcIso.readFully(source);
    destIso.seek(destIso.length());
    destIso.align(ISO_SECTOR_SIZE);
    i32 destLba = destIso.pos() / ISO_SECTOR_SIZE;
    destIso.writeFully(source);
    destIso.align(ISO_SECTOR_SIZE);
    destIso.seek(srcRecord.isoOffset + 2);
    destIso.writeInt(destLba);
    destIso.writeIntB(destLba);
    destIso.seek(srcRecord.isoOffset + 2 + 8);
    destIso.writeInt(source.size());
    destIso.writeIntB(source.size());
}
