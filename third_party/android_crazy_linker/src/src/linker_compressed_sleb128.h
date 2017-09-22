/*
 * Copyright (C) 2017 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _LINKER_COMPRESSED_SLEB128_H
#define _LINKER_COMPRESSED_SLEB128_H

#include <stdint.h>

class compressed_sleb128_decoder {
 public:
  compressed_sleb128_decoder(const uint8_t* buffer, size_t count);
  ~compressed_sleb128_decoder();

  size_t pop_front();
  bool initialize();
 private:
  bool uncompress_chunk();

  // Holds the active uncompressed chunk.
  char* buffer_ = nullptr;
  // Points to next chunk to uncompress.
  const char* lz4_src_;
  // Points to end of compressed data.
  const char* lz4_end_;

  // Cursor for sleb128 decoding (within buffer_).
  const uint8_t* current_ = nullptr;
  // Points to end of uncompressed chunk.
  const uint8_t* end_ = nullptr;
};

#endif // __LINKER_COMPRESSED_SLEB128_H
