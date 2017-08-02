// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>
#include <vector>

#include "net/spdy/platform/impl/spdy_string_utils_impl.h"

#include "base/strings/string_number_conversions.h"

namespace net {

SpdyString SpdyHexDecodeImpl(SpdyStringPiece data) {
  std::vector<uint8_t> output;
  std::string result;
  if (base::HexStringToBytes(data.as_string(), &output))
    result.assign(reinterpret_cast<const char*>(&output[0]), output.size());
  return result;
}

}  // namespace net
