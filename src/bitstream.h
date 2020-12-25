#pragma once

#include "platform.h"

class BitStream {
  public:
    BitStream(const ByteBuffer& buf, bool msbOrder = true);
    bool readBit();
    u8 readByte();
    u32 readInt(u32 bits = 32);
    u32 bytePos() const;
    u8 bitOfBytePos() const;

  private:
    const ByteBuffer buf;
    const bool msbOrder;
    bool eof;
    u8 currentByte;
    u32 pos;
    u8 bitPos;
};
