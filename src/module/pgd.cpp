#include "pgd.h"

static const u8 pgdHeaderSize = 0x90;

PgdStreamFile::PgdStreamFile(Stream& input, i64 offset, ByteBuffer key, u32 pgdFlag) : input(input), offset(offset) {
    kirk_init();
    if (key.size() != 16) {
        bail("PGD key must be exactly 16 bytes long");
    }
    input.seek(offset);
    ByteBuffer pgdHeader(pgdHeaderSize);
    input.readFully(pgdHeader);
    pgd = pgd_open(pgdHeader.data(), pgdFlag, key.data());
    if (!pgd) {
        bail("PGD open failed");
    }
}
PgdStreamFile::~PgdStreamFile() {
    if (pgd) {
        pgd->block_buf = nullptr;
        pgd_close(pgd);
        pgd = nullptr;
    }
}

void PgdStreamFile::decryptPartially(u64 pgdOffset, ByteBuffer& outputBuf) {
    spdlog::trace("Decrypt PGD partially at {}, size {}", pgdOffset, outputBuf.size());
    const u32 blockSize = pgd->block_size;
    const u32 blockNr = pgd->block_nr;
    i64 startingBlock = pgdOffset / blockSize;
    i64 neededBlocks = outputBuf.size() / blockSize;
    if (startingBlock + neededBlocks > blockNr) {
        bail("PGD block out of range");
    }

    ByteBuffer blockBuf(blockSize);
    pgd->block_buf = blockBuf.data();
    Stream output(outputBuf);
    // decrypt initial block
    input.seek(offset + pgdHeaderSize + startingBlock * blockSize);
    input.readFully(blockBuf);
    pgd_decrypt_block(pgd, startingBlock);
    output.writeFully(blockBuf.data() + (pgdOffset - startingBlock * blockSize),
                      std::min((usize)blockSize, outputBuf.size()));

    // decrypt the rest
    for (int i = 0; i < neededBlocks; i++) {
        input.readFully(blockBuf);
        pgd_decrypt_block(pgd, startingBlock + 1 + i);

        output.writeFully(blockBuf.data(), std::min((u64)blockSize, outputBuf.size() - output.pos()));
        if (output.pos() >= (i64)outputBuf.size()) {
            break;
        }
    }

    pgd->block_buf = NULL;
}

void PgdStreamFile::decryptFully(Progress& progress, Stream& output) {
    spdlog::trace("Decrypt PGD");
    input.seek(offset + pgdHeaderSize);
    const u32 blockSize = pgd->block_size;
    const u32 blockNr = pgd->block_nr;
    ByteBuffer blockBuf(blockSize);
    pgd->block_buf = blockBuf.data();
    for (u32 i = 0; i < blockNr; i++) {
        input.readFully(blockBuf);
        pgd_decrypt_block(pgd, i);
        output.writeFully(blockBuf);
        progress.updatePart(i, blockNr - 1);
    }
    pgd->block_buf = NULL;
}
