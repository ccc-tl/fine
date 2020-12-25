#pragma once

#include "platform.h"
#include "streamio.h"

class Stream {
  public:
    Stream(const fs::path& path, bool readOnly = false);
    Stream(ByteBuffer& buf);
    Stream(u8* data, i64 size);

    i8 readByte();
    i16 readShort();
    i32 readInt();
    i64 readLong();
    float readFloat();
    double readDouble();

    i16 readShortB();
    i32 readIntB();
    i64 readLongB();

    std::string readString();
    std::string readString(i32 len);
    void readFully(ByteBuffer& buf);
    void readFully(u8* buf, i64 bufLen);

    void writeByte(i8 value);
    void writeShort(i16 value);
    void writeInt(i32 value);
    void writeLong(i64 value);
    void writeFloat(float value);
    void writeDouble(double value);

    void writeShortB(i16 value);
    void writeIntB(i32 value);
    void writeLongB(i64 value);

    void writeString(const std::string& string);
    void writeString(const std::string& string, i32 len);
    void writeFully(const ByteBuffer& buf);
    void writeFully(const u8* buf, i64 bufLen);

    void align(i64 alignment);

    bool good();
    i64 pos();
    void seek(i64 offset);
    void skip(i64 num);
    i64 length();

  private:
    std::unique_ptr<StreamIO> stream;
};
