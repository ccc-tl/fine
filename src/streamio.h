#pragma once

#include "platform.h"

class StreamIO {
  public:
    virtual ~StreamIO() {
    }
    virtual bool good() = 0;
    virtual void seek(i64 pos) = 0;
    virtual i64 pos() = 0;
    virtual i64 length() = 0;
    virtual void write(u8 byte) = 0;
    virtual u8 read() = 0;
    virtual void writeFully(const u8* buf, i64 len) = 0;
    virtual void readFully(u8* buf, i64 len) = 0;
};

class FileStreamIO : public StreamIO {
  public:
    FileStreamIO(const fs::path& path, bool readOnly = false);
    virtual ~FileStreamIO() {
    }
    virtual bool good();
    virtual void seek(i64 pos);
    virtual i64 pos();
    virtual i64 length();
    virtual void write(u8 byte);
    virtual u8 read();
    virtual void writeFully(const u8* buf, i64 len);
    virtual void readFully(u8* buf, i64 len);

  private:
    std::fstream stream;
};

class BufferStreamIO : public StreamIO {
  public:
    BufferStreamIO(u8* data, i64 len);
    virtual ~BufferStreamIO() {
    }
    virtual bool good();
    virtual void seek(i64 pos);
    virtual i64 pos();
    virtual i64 length();
    virtual void write(u8 byte);
    virtual u8 read();
    virtual void writeFully(const u8* buf, i64 len);
    virtual void readFully(u8* buf, i64 len);

  private:
    u8* data;
    i64 dataLen;
    i64 position;
};
