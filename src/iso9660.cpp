#include "iso9660.h"

#include "spdlog/spdlog.h"
#include "stream.h"

IsoPrimaryVolumeDescriptor::IsoPrimaryVolumeDescriptor(Stream& input) {
    if (input.readString(5) != "CD001") {
        bail("Invalid primary volume identifier");
    }
    u8 version = input.readByte();
    spdlog::trace("ISO version: {}", version);
    input.skip(1);

    // offset 8
    std::string systemIdentifier = input.readString(32);
    spdlog::trace("ISO system identifier: {}", systemIdentifier);
    std::string volumeIdentifier = input.readString(32);
    spdlog::trace("ISO volume identifier: {}", volumeIdentifier);
    input.skip(8);

    // offset 80 - volume root descriptor
    volumeSpaceSizePos = input.pos();
    u32 volumeSpaceSize = input.readInt();
    input.readIntB();
    spdlog::trace("ISO volume space size: {}", volumeSpaceSize);
    input.skip(32);

    u16 setSize = input.readShort();
    input.readShortB();
    u16 seqNumber = input.readShort();
    input.readShortB();
    if (setSize != 1 || seqNumber != 1) {
        bail("ISO is not the first disk or volume has more than one disk");
    }

    u16 logicalBlockSize = input.readShort();
    input.readShortB();
    spdlog::trace("ISO logical block size: {}", logicalBlockSize);
    if (logicalBlockSize != 2048) {
        bail("ISO logical block size is not 2048");
    }

    pathTableSize = input.readInt();
    input.readIntB();
    spdlog::trace("ISO path table size: {}", pathTableSize);

    pathTableLba = input.readInt();
    u32 optionalPathTableLba = input.readInt();
    u32 pathTableLbaB = input.readIntB();
    u32 optionalPathTableLbaB = input.readIntB();
    spdlog::trace("ISO path table LBA: LE: {}, BE: {}", pathTableLba, pathTableLbaB);
    spdlog::trace("ISO optional path table LBA: LE: {}, BE: {}", optionalPathTableLba, optionalPathTableLbaB);

    // offset 156 - root directory record
    if (input.readByte() != 34) {
        bail("Expected root record to be exactly 34 bytes long");
    }
    if (input.readByte() != 0) {
        bail("Expected root record to not have extended attribute record");
    }
    u32 lba = input.readInt();
    input.readIntB();
    u32 dataLen = input.readInt();
    input.readIntB();
    spdlog::trace("ISO root lba: {}, data length: {}", lba, dataLen);
    input.skip(7); // date and time, don't care
    if (input.readByte() != 2) {
        bail("Expected root recrod to be a standard directory");
    }
    if (input.readByte() != 0) {
        bail("Expected file unit size to be 0");
    }
    if (input.readByte() != 0) {
        bail("Expected interleave gap size to be 0");
    }
    u16 volumeSequenceNumber = input.readShort();
    input.readShortB();
    if (volumeSequenceNumber != 1) {
        bail("Volume sequence number is not 1");
    }
    if (input.readByte() != 1) {
        bail("Length of root file identifier is not 1");
    }
    input.skip(1);

    // offset 190 - more id fields, publisher etc. don't care
    input.skip(128);
    input.skip(128);
    input.skip(128);
    input.skip(128);
    input.skip(37);
    input.skip(37);
    input.skip(37);

    std::string volumeCreationDate = input.readString(17);
    spdlog::trace("ISO volume creation date: {}", volumeCreationDate);
    input.skip(17);
    input.skip(17);
    input.skip(17);
    input.readByte();
    // application defined sector follows
}

inline u32 IsoPrimaryVolumeDescriptor::getPathTableSize() {
    return pathTableSize;
}

inline u32 IsoPrimaryVolumeDescriptor::getPathTableLba() {
    return pathTableLba;
}

IsoTerminatorVolumeDescriptor::IsoTerminatorVolumeDescriptor(Stream& input) {
    if (input.readString(5) != "CD001" || input.readByte() != 1) {
        bail("Invalid volume descriptor terminator magic values");
    }
}

IsoDirectoryRecordEntry::IsoDirectoryRecordEntry(i64 isoOffset, std::string& basePath, std::string& name, u32 lba,
                                                 u32 length, u8 atributes)
    : isoOffset(isoOffset), name(name), lba(lba), length(length), atributes(atributes) {
    std::stringstream relPathSs;
    relPathSs << basePath << name;
    relPath = relPathSs.str();
}

IsoDirectoryRecord::IsoDirectoryRecord(Stream& input, std::string basePath) {
    spdlog::trace("Directory record at {}, base path {}", input.pos(), basePath);
    while (true) {
        i64 startPos = input.pos();
        u8 recordLength = input.readByte();
        if (recordLength == 0) {
            break;
        }

        if (input.readByte() != 0) {
            bail("Expected record to not have extended attribute record");
        }
        u32 lba = input.readInt();
        input.readIntB();
        u32 length = input.readInt();
        input.readIntB();

        input.skip(7); // date and time, don't care
        u8 atributes = input.readByte();

        if (input.readByte() != 0) {
            bail("Expected file unit size to be 0");
        }
        if (input.readByte() != 0) {
            bail("Expected interleave gap size to be 0");
        }
        u16 volumeSequenceNumber = input.readShort();
        input.readShortB();
        if (volumeSequenceNumber != 1) {
            bail("Volume sequence number is not 1");
        }
        u8 nameLength = input.readByte();
        std::string name = input.readString(nameLength);

        if (nameLength % 2 == 1) {
            input.skip(1); // padding
        }
        input.skip(recordLength - (input.pos() - startPos));
        entries.emplace_back(startPos, basePath, name, lba, length, atributes);
        spdlog::trace("Entry at {}, name: '{}', LBA: {}, data length: {}, atributes: {}", startPos, name, lba, length,
                      atributes);
    }
}

const std::vector<IsoDirectoryRecordEntry>& IsoDirectoryRecord::getEntries() const {
    return entries;
}

IsoPathTableEntry::IsoPathTableEntry(std::string& name, u32 lba, u16 parentNumber)
    : name(name), lba(lba), parentNumber(parentNumber) {
}

IsoPathTable::IsoPathTable(Stream& input, u64 size) {
    i64 tableEnd = input.pos() + size;
    spdlog::trace("Path table at {}, size {}", input.pos(), size);
    while (input.pos() < tableEnd) {
        u8 nameLength = input.readByte();
        u8 extAtribCount = input.readByte();
        if (extAtribCount != 0) {
            bail("Path table extended atributes count is not 0. This isn't implemented.");
        }
        u32 lba = input.readInt();
        u16 parentNum = input.readShort();
        std::string name = input.readString(nameLength);
        if (nameLength % 2 == 1) {
            input.skip(1); // padding
        }
        spdlog::trace("Path table entry: '{}', LBA: {}, parent: {}", name, lba, parentNum);
        entries.emplace_back(name, lba, parentNum);
    }
}

const std::vector<IsoPathTableEntry>& IsoPathTable::getEntries() {
    return entries;
}

std::string IsoPathTable::getRelPath(const IsoPathTableEntry& entry) {
    auto& entries = getEntries();
    std::vector<std::string> parts;
    parts.emplace_back(entry.name);
    const IsoPathTableEntry* currentEntry = &entry;
    while (currentEntry->parentNumber != 1) {
        currentEntry = &entries[currentEntry->parentNumber - 1];
        parts.emplace_back(currentEntry->name);
    }
    std::reverse(parts.begin(), parts.end());
    std::stringstream ss;
    for (auto& part : parts) {
        ss << part << "/";
    }
    return ss.str();
}

Iso9660Reader::Iso9660Reader(const fs::path& iso) : stream(iso, true) {
    spdlog::info("Reading ISO: '{}'", iso.u8string());
    i64 length = stream.length();
    if (!stream.good()) {
        bail("Failed to open ISO");
    }
    if (stream.length() <= 18 * ISO_SECTOR_SIZE) {
        bail("ISO file is too small, expected at least 18 sectors");
    }
    stream.seek(16 * ISO_SECTOR_SIZE);
    while (stream.pos() < length) {
        i64 sectorStart = stream.pos();
        u8 volumeDescriptorType = stream.readByte();
        spdlog::trace("Volume descriptor of type {} at {}", volumeDescriptorType, sectorStart);
        if (volumeDescriptorType == 1) {
            primaryDescriptor = std::make_unique<IsoPrimaryVolumeDescriptor>(stream);
        } else if (volumeDescriptorType == 255) {
            terminatorDescriptor = std::make_unique<IsoTerminatorVolumeDescriptor>(stream);
            break;
        } else {
            if (primaryDescriptor == nullptr) {
                bail("ISO is damaged (primary descriptor missing)");
            }
            spdlog::warn("Skipped volume descriptor type: {}", volumeDescriptorType);
        }
        stream.seek(sectorStart + ISO_SECTOR_SIZE);
    }
    stream.seek(primaryDescriptor->getPathTableLba() * ISO_SECTOR_SIZE);
    pathTable = std::make_unique<IsoPathTable>(stream, primaryDescriptor->getPathTableSize());
    for (auto& entry : pathTable->getEntries()) {
        std::string relPath = pathTable->getRelPath(entry);
        stream.seek(entry.lba * ISO_SECTOR_SIZE);
        records.emplace_back(stream, relPath);
    }
}

const std::vector<IsoDirectoryRecord>& Iso9660Reader::getRecords() {
    return records;
}
