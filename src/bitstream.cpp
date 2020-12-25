#include "bitstream.h"

#include "spdlog/spdlog.h"

BitStream::BitStream(const ByteBuffer& buf, bool msbOrder)
    : buf(buf), msbOrder(msbOrder), eof(false), currentByte(buf[0]), pos(0), bitPos(0) {
    spdlog::trace("Create bit stream for buffer, size: {}, msb order: {}", buf.size(), msbOrder);
}

bool BitStream::readBit() {
    u8 readBit;
    if (msbOrder) {
        readBit = 7 - bitPos;
    } else {
        readBit = bitPos;
    }
    bool value = (currentByte & (1 << readBit)) != 0;
    bitPos++;
    if (bitPos == 8) {
        pos++;
        bitPos = 0;
        if (pos == buf.size()) {
            eof = true;
        } else {
            currentByte = buf[pos];
        }
    }
    return value;
}

u8 BitStream::readByte() {
    u8 byte = 0;
    for (int i = 0; i < 8; i++) {
        byte |= (readBit() << (7 - i));
    }
    return byte;
}

u32 BitStream::readInt(u32 bits) {
    if (bits > 32) {
        bail("bits must be <= 32");
    }
    u32 value = 0;
    for (u32 i = 0; i < bits; i++) {
        value |= (readBit() << (bits - 1 - i));
    }
    return value;
}

u32 BitStream::bytePos() const {
    return pos;
}

u8 BitStream::bitOfBytePos() const {
    return bitPos;
}
