#pragma once

#include "../commons.h"

extern "C" {
#include "libkirk/amctrl.h"
#include "libkirk/kirk_engine.h"
}

class PgdStreamFile {
  public:
    PgdStreamFile(Stream& input, i64 offset, ByteBuffer key, u32 pgdFlag);
    ~PgdStreamFile();
    void decryptPartially(u64 pgdOffset, ByteBuffer& output);
    void decryptFully(Progress& progress, Stream& output);

  private:
    Stream& input;
    i64 offset;
    PGD_DESC* pgd;
};
