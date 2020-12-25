#include "cmp.h"

class CmpDict {
  public:
    CmpDict() : data(4096), pointer(0xFEE) {
    }
    void write(u8 byte) {
        data[pointer] = byte;
        pointer++;
        if (pointer == data.size()) {
            pointer = 0;
        }
    }
    u8 get(u32 offset) {
        return data[offset];
    }

  private:
    ByteBuffer data;
    u32 pointer;
};

ByteBuffer decompressCmp(ByteBuffer& buf) {
    spdlog::trace("Decompressing CMP file");
    Stream input(buf);
    std::string magic = input.readString(4);
    if (magic != "IECP") {
        spdlog::error("CMP magic is invalid: '{}'", magic);
        bail("Attempted to decompress invalid CMP file");
    }
    i64 inputLen = input.length();
    i32 decompressedSize = input.readInt();
    ByteBuffer data(decompressedSize);
    Stream output(data);
    CmpDict dict;
    while (input.pos() < inputLen) {
        u8 control = input.readByte();
        for (u8 i = 0; i < 8; i++) {
            if (input.pos() == inputLen) {
                break;
            }
            u8 testBit = 1 << i;
            if (control & testBit) {
                u8 byte = input.readByte();
                output.writeByte(byte);
                dict.write(byte);
            } else {
                u32 b1 = input.readByte() & 0xFF;
                u32 b2 = input.readByte() & 0xFF;
                u32 len = (b2 & 0xF) + 2;
                u32 offset = ((b2 & 0xF0) << 4) | b1;
                for (u32 localOffset = 0; localOffset <= len; localOffset++) {
                    u32 dictOffset = (offset + localOffset) & 0xFFF;
                    u8 byte = dict.get(dictOffset);
                    output.writeByte(byte);
                    dict.write(byte);
                }
            }
        }
    }
    return data;
}
