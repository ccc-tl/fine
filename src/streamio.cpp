#include "streamio.h"

static std::ios_base::openmode fstreamOpenFlags(bool readOnly) {
    if (readOnly) {
        return std::ios::binary | std::ios::in;
    } else {
        return std::ios::binary | std::ios::in | std::ios::out | std::ios::ate;
    }
}

FileStreamIO::FileStreamIO(const fs::path& path, bool readOnly) : stream(std::fstream(path, fstreamOpenFlags(readOnly))) {
    seek(0);
}

bool FileStreamIO::good() {
    return stream.good();
}

void FileStreamIO::seek(i64 pos) {
    stream.seekg(pos, std::ios::beg);
}

i64 FileStreamIO::pos() {
    return stream.tellg();
}

i64 FileStreamIO::length() {
    i64 prevPos = pos();
    stream.seekg(0, std::ios::end);
    i64 length = pos();
    seek(prevPos);
    return length;
}

void FileStreamIO::write(u8 byte) {
    stream.write(reinterpret_cast<char*>(&byte), 1);
}

u8 FileStreamIO::read() {
    i8 byte;
    stream.read(reinterpret_cast<char*>(&byte), 1);
    return byte;
}

void FileStreamIO::writeFully(const u8* buf, i64 bufLen) {
    stream.write(reinterpret_cast<const char*>(buf), bufLen);
}

void FileStreamIO::readFully(u8* buf, i64 bufLen) {
    stream.read(reinterpret_cast<char*>(buf), bufLen);
}

BufferStreamIO::BufferStreamIO(u8* data, i64 len) : data(data), dataLen(len), position(0) {
}

bool BufferStreamIO::good() {
    return true;
}

void BufferStreamIO::seek(i64 pos) {
    position = pos;
}

i64 BufferStreamIO::pos() {
    return position;
}

i64 BufferStreamIO::length() {
    return dataLen;
}

void BufferStreamIO::write(u8 byte) {
    if (position >= dataLen) {
        bail("Buffer overflow in BufferStreamIO.write");
    }
    data[position++] = byte;
}

u8 BufferStreamIO::read() {
    if (position >= dataLen) {
        bail("Buffer EOF in BufferStreamIO.read");
    }
    return data[position++];
}

void BufferStreamIO::writeFully(const u8* buf, i64 len) {
    if (position + len > dataLen) {
        bail("Buffer overflow in BufferStreamIO.writeFully");
    }
    memcpy(data + position, buf, len);
    position += len;
}

void BufferStreamIO::readFully(u8* buf, i64 len) {
    if (position + len > dataLen) {
        bail("Buffer EOF in BufferStreamIO.readFully");
    }
    memcpy(buf, data + position, len);
    position += len;
}
