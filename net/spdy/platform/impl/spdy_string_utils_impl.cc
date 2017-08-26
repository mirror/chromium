// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/platform/impl/spdy_string_utils_impl.h"

#include "base/strings/string_number_conversions.h"

namespace net {

bool SpdyHexDecodeToUIntImpl(SpdyStringPiece data, uint32_t* out) {
  DCHECK(!data.empty());
  DCHECK_LE(data.size(), 8u);
  // Pad with leading zeros.
  SpdyString data_padded =
      SpdyString(8 - data.size(), '0').append(data.begin(), data.end());
  return base::HexStringToUInt(data_padded, out);
}

SpdyString SpdyHexEncodeUIntAndTrimImpl(uint32_t data) {
  if (data == 0)
    return "0";
  char data_bytes[4];
  for (int i = 3; i >= 0; --i, data >>= 8) {
    data_bytes[i] = data & 0xFF;
  }
  SpdyString hex_string = SpdyHexEncodeImpl(data_bytes, 4);
  // Trim leading zeros.
  auto pos = hex_string.find_first_not_of('0');
  DCHECK_NE(pos, SpdyString::npos);
  return hex_string.substr(pos);
}

}  // namespace net