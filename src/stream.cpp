#include "stream.h"

Stream::Stream(const fs::path& path, bool readOnly) : stream(std::make_unique<FileStreamIO>(path, readOnly)) {
}

Stream::Stream(ByteBuffer& buf) : stream(std::make_unique<BufferStreamIO>(buf.data(), buf.size())) {
}

Stream::Stream(u8* data, i64 size) : stream(std::make_unique<BufferStreamIO>(data, size)) {
}

i8 Stream::readByte() {
    return stream->read();
}

i16 Stream::readShort() {
    u16 b1 = readByte();
    u16 b2 = readByte();
    return (b2 & 0xFF) << 8 | (b1 & 0xFF);
}

i32 Stream::readInt() {
    u32 b1 = readByte();
    u32 b2 = readByte();
    u32 b3 = readByte();
    u32 b4 = readByte();
    return (b4 & 0xFF) << 24 | (b3 & 0xFF) << 16 | (b2 & 0xFF) << 8 | (b1 & 0xFF);
}

i64 Stream::readLong() {
    u64 b1 = readByte();
    u64 b2 = readByte();
    u64 b3 = readByte();
    u64 b4 = readByte();
    u64 b5 = readByte();
    u64 b6 = readByte();
    u64 b7 = readByte();
    u64 b8 = readByte();
    return (b8 & 0xFF) << 56 | (b7 & 0xFF) << 48 | (b6 & 0xFF) << 40 | (b5 & 0xFF) << 32 | (b4 & 0xFF) << 24 |
           (b3 & 0xFF) << 16 | (b2 & 0xFF) << 8 | (b1 & 0xFF);
}

float Stream::readFloat() {
    i32 data = readInt();
    return reinterpret_cast<float&>(data);
}

double Stream::readDouble() {
    i64 data = readLong();
    return reinterpret_cast<double&>(data);
}

i16 Stream::readShortB() {
    u16 b2 = readByte();
    u16 b1 = readByte();
    return (b2 & 0xFF) << 8 | (b1 & 0xFF);
}

i32 Stream::readIntB() {
    u32 b4 = readByte();
    u32 b3 = readByte();
    u32 b2 = readByte();
    u32 b1 = readByte();
    return (b4 & 0xFF) << 24 | (b3 & 0xFF) << 16 | (b2 & 0xFF) << 8 | (b1 & 0xFF);
}

i64 Stream::readLongB() {
    u64 b8 = readByte();
    u64 b7 = readByte();
    u64 b6 = readByte();
    u64 b5 = readByte();
    u64 b4 = readByte();
    u64 b3 = readByte();
    u64 b2 = readByte();
    u64 b1 = readByte();
    return (b8 & 0xFF) << 56 | (b7 & 0xFF) << 48 | (b6 & 0xFF) << 40 | (b5 & 0xFF) << 32 | (b4 & 0xFF) << 24 |
           (b3 & 0xFF) << 16 | (b2 & 0xFF) << 8 | (b1 & 0xFF);
}

std::string Stream::readString() {
    std::stringstream ss;
    while (true) {
        i8 byte = readByte();
        if (byte == 0) return ss.str();
        ss << (char)byte;
    }
}

std::string Stream::readString(i32 len) {
    ByteBuffer buf(len);
    readFully(buf);
    return std::string(buf.begin(), buf.end());
}

void Stream::readFully(ByteBuffer& buf) {
    readFully(buf.data(), buf.size());
}

void Stream::readFully(u8* buf, i64 bufLen) {
    stream->readFully(buf, bufLen);
}

void Stream::writeByte(i8 value) {
    stream->write(value);
}

void Stream::writeShort(i16 value) {
    writeByte(value & 0xFF);
    writeByte((value >> 8) & 0xFF);
}

void Stream::writeInt(i32 value) {
    writeByte(value & 0xFF);
    writeByte((value >> 8) & 0xFF);
    writeByte((value >> 16) & 0xFF);
    writeByte((value >> 24) & 0xFF);
}

void Stream::writeLong(i64 value) {
    writeByte(value & 0xFF);
    writeByte((value >> 8) & 0xFF);
    writeByte((value >> 16) & 0xFF);
    writeByte((value >> 24) & 0xFF);
    writeByte((value >> 32) & 0xFF);
    writeByte((value >> 40) & 0xFF);
    writeByte((value >> 48) & 0xFF);
    writeByte((value >> 56) & 0xFF);
}

void Stream::writeFloat(float value) {
    writeInt(reinterpret_cast<i32&>(value));
}

void Stream::writeDouble(double value) {
    writeLong(reinterpret_cast<i64&>(value));
}

void Stream::writeShortB(i16 value) {
    writeByte((value >> 8) & 0xFF);
    writeByte(value & 0xFF);
}

void Stream::writeIntB(i32 value) {
    writeByte((value >> 24) & 0xFF);
    writeByte((value >> 16) & 0xFF);
    writeByte((value >> 8) & 0xFF);
    writeByte(value & 0xFF);
}

void Stream::writeLongB(i64 value) {
    writeByte((value >> 56) & 0xFF);
    writeByte((value >> 48) & 0xFF);
    writeByte((value >> 40) & 0xFF);
    writeByte((value >> 32) & 0xFF);
    writeByte((value >> 24) & 0xFF);
    writeByte((value >> 16) & 0xFF);
    writeByte((value >> 8) & 0xFF);
    writeByte(value & 0xFF);
}

void Stream::writeString(const std::string& string) {
    writeString(string, string.length());
}

void Stream::writeString(const std::string& string, i32 len) {
    writeFully(reinterpret_cast<const u8*>(string.c_str()), len);
}

void Stream::writeFully(const ByteBuffer& buf) {
    writeFully(buf.data(), buf.size());
}

void Stream::writeFully(const u8* buf, i64 bufLen) {
    stream->writeFully(buf, bufLen);
}

void Stream::align(i64 alignment) {
    if (pos() % alignment == 0) return;
    i64 targetCount = (pos() / alignment + 1) * alignment;
    ByteBuffer blank(targetCount - pos());
    writeFully(blank);
}

bool Stream::good() {
    return stream->good();
}

i64 Stream::pos() {
    return stream->pos();
}

void Stream::seek(i64 offset) {
    stream->seek(offset);
}

void Stream::skip(i64 num) {
    seek(pos() + num);
}

i64 Stream::length() {
    return stream->length();
}
