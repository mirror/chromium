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

#include "linker_compressed_sleb128.h"

#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "third_party/lz4/lib/lz4.h"

#include "crazy_linker_debug.h"


// Must match the constant used by relocation_packer.
static constexpr size_t kCompressionChunkSize = 64 * 1024;


compressed_sleb128_decoder::compressed_sleb128_decoder(const uint8_t* buffer, size_t count)
  // Skip over decoded length (header has already been skipped).
  : lz4_src_(reinterpret_cast<const char*>(buffer) + 4),
    lz4_end_(reinterpret_cast<const char*>(buffer) + count) {}

compressed_sleb128_decoder::~compressed_sleb128_decoder() {
  if (buffer_) {
    munmap(buffer_, kCompressionChunkSize);
  }
}

bool compressed_sleb128_decoder::initialize() {
  buffer_ = reinterpret_cast<char*>(mmap(nullptr, kCompressionChunkSize, PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, 0, 0));
  if (buffer_ == MAP_FAILED) {
    LOG("mmap failed");
    return false;
  }

  return uncompress_chunk();
}

bool compressed_sleb128_decoder::uncompress_chunk() {
  uint32_t chunk_size;
  memcpy(&chunk_size, lz4_src_, sizeof(uint32_t));
  if (lz4_src_ + chunk_size >= lz4_end_) {
    LOG("Out-of-bounds during lz4 decompress");
    return false;
  }
  lz4_src_ += sizeof(uint32_t);
  int bytes_decompressed = LZ4_decompress_safe(lz4_src_, buffer_, chunk_size, kCompressionChunkSize);
  if (bytes_decompressed <= 0) {
    LOG("Decompress failed.");
    return false;
  }
  lz4_src_ += chunk_size;
  current_ = reinterpret_cast<uint8_t*>(buffer_);
  end_ = current_ + bytes_decompressed;

  return true;
}

size_t compressed_sleb128_decoder::pop_front() {
  size_t value = 0;
  static const size_t size = CHAR_BIT * sizeof(value);

  size_t shift = 0;
  uint8_t byte;

  do {
    if (current_ >= end_) {
      if (!uncompress_chunk()) {
        LOG("compressed_sleb128_decoder ran out of bounds");
        assert(false);
        return 0;
      }
    }

    byte = *current_++;
    value |= (static_cast<size_t>(byte & 127) << shift);
    shift += 7;
  } while (byte & 128);

  if (shift < size && (byte & 64)) {
    value |= -(static_cast<size_t>(1) << shift);
  }

  return value;
}
