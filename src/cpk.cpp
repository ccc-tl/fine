#include "cpk.h"

#include "spdlog/spdlog.h"

#include "bitstream.h"
#include "stream.h"

class SizeSeq {
  public:
    i32 next() {
        if (idx > 2) {
            return 8;
        }
        i32 retVal;
        switch (idx) {
        case 0:
            retVal = 2;
            break;
        case 1:
            retVal = 3;
            break;
        case 2:
            retVal = 5;
            break;
        }
        idx++;
        return retVal;
    }

  private:
    u8 idx = 0;
};

ByteBuffer decompressLayla(ByteBuffer& bytes) {
    spdlog::trace("Decompress LAYLA file, size: {}", bytes.size());
    Stream input(bytes);
    std::string magic = input.readString(8);
    if (magic != "CRILAYLA") {
        spdlog::error("Invalid LAYLA magic value: '{}'", magic);
        bail("Attempted to decompress invalid LAYLA file");
    }
    u32 sizeOrig = input.readInt();
    u32 sizeComp = input.readInt();
    ByteBuffer compressed(sizeComp);
    ByteBuffer prefix(0x100);
    input.readFully(compressed);
    input.readFully(prefix);
    std::reverse(compressed.begin(), compressed.end());
    BitStream compressedBits(compressed);

    ByteBuffer decompressed(sizeOrig);
    Stream decompressedStream(decompressed);

    while (decompressedStream.pos() < sizeOrig) {
        SizeSeq sizes;
        if (compressedBits.readBit()) {
            i32 repetitions = 3;
            const i32 lookBehind = compressedBits.readInt(13) + 3;
            while (true) {
                i32 size = sizes.next();
                const i32 marker = compressedBits.readInt(size);
                repetitions += marker;
                if (marker != ((1 << size) - 1)) {
                    break;
                }
            }
            for (i32 i = 0; i < repetitions; i++) {
                u8 byte = decompressed[decompressedStream.pos() - lookBehind];
                decompressedStream.writeByte(byte);
            }
        } else {
            u8 byte = compressedBits.readByte();
            decompressedStream.writeByte(byte);
        }
    }
    std::reverse(decompressed.begin(), decompressed.end());

    ByteBuffer combined(sizeOrig + 0x100);
    Stream combinedStream(combined);
    combinedStream.writeFully(prefix);
    combinedStream.writeFully(decompressed);
    return combined;
}
