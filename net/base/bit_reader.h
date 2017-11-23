// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_BIT_READER_H_
#define NET_BASE_BIT_READER_H_

#include <stdint.h>
#include <stdlib.h>

#include "net/base/net_export.h"

namespace net {

// BitReader is a class that allows a bytestring to be read bit-by-bit.
class NET_EXPORT BitReader {
 public:
  BitReader(const uint8_t* bytes, size_t num_bits);

  // Next sets |*out| to the next bit from the input. It returns false if no
  // more bits are available or true otherwise.
  bool Next(bool* out);

  // Read sets the |num_bits| least-significant bits of |*out| to the value of
  // the next |num_bits| bits from the input. It returns false if there are
  // insufficient bits in the input or true otherwise.
  bool Read(unsigned num_bits, uint32_t* out);

  // Unary sets |*out| to the result of decoding a unary value from the input.
  // It returns false if there were insufficient bits in the input and true
  // otherwise.
  bool Unary(size_t* out);

  // Seek sets the current offest in the input to bit number |offset|. It
  // returns true if |offset| is within the range of the input and false
  // otherwise.
  bool Seek(size_t offset);

 private:
  const uint8_t* const bytes_;
  const size_t num_bits_;
  const size_t num_bytes_;
  // current_byte_index_ contains the current byte offset in |bytes_|.
  size_t current_byte_index_;
  // current_byte_ contains the current byte of the input.
  uint8_t current_byte_;
  // num_bits_used_ contains the number of bits of |current_byte_| that have
  // been read.
  unsigned num_bits_used_;
};

}  // namespace net

#endif  // NET_BASE_BIT_READER_H_
