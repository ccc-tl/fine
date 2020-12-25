#pragma once

#include "platform.h"
#include "stream.h"

const u32 ISO_SECTOR_SIZE = 2048;

class IsoPrimaryVolumeDescriptor {
  public:
    IsoPrimaryVolumeDescriptor(Stream& input);
    inline u32 getPathTableSize();
    inline u32 getPathTableLba();

  private:
    i64 volumeSpaceSizePos;
    u32 pathTableSize;
    u32 pathTableLba;
};

class IsoTerminatorVolumeDescriptor {
  public:
    IsoTerminatorVolumeDescriptor(Stream& input);
};

class IsoDirectoryRecordEntry {
  public:
    IsoDirectoryRecordEntry(i64 isoOffset, std::string& basePath, std::string& name, u32 lba, u32 length, u8 atributes);
    i64 isoOffset;
    std::string relPath;
    std::string name;
    u32 lba;
    u32 length;
    u8 atributes;
};

class IsoDirectoryRecord {
  public:
    IsoDirectoryRecord(Stream& input, std::string relPath);
    const std::vector<IsoDirectoryRecordEntry>& getEntries() const;

  private:
    std::vector<IsoDirectoryRecordEntry> entries;
};

class IsoPathTableEntry {
  public:
    IsoPathTableEntry(std::string& name, u32 lba, u16 parentNumber);
    std::string name;
    u32 lba;
    u16 parentNumber;
};

class IsoPathTable {
  public:
    IsoPathTable(Stream& input, u64 size);
    const std::vector<IsoPathTableEntry>& getEntries();
    std::string getRelPath(const IsoPathTableEntry& entry);

  private:
    std::vector<IsoPathTableEntry> entries;
};

class Iso9660Reader {
  public:
    Iso9660Reader(const fs::path& iso);
    const std::vector<IsoDirectoryRecord>& getRecords();

  private:
    Stream stream;
    std::unique_ptr<IsoPrimaryVolumeDescriptor> primaryDescriptor;
    std::unique_ptr<IsoTerminatorVolumeDescriptor> terminatorDescriptor;
    std::unique_ptr<IsoPathTable> pathTable;
    std::vector<IsoDirectoryRecord> records;
};
