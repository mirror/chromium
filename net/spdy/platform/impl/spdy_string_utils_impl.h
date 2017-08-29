// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_PLATFORM_IMPL_SPDY_STRING_UTILS_IMPL_H_
#define NET_SPDY_PLATFORM_IMPL_SPDY_STRING_UTILS_IMPL_H_

#include <sstream>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_byteorder.h"
#include "net/base/hex_utils.h"
#include "net/spdy/platform/api/spdy_export.h"
#include "net/spdy/platform/api/spdy_string.h"
#include "net/spdy/platform/api/spdy_string_piece.h"

namespace net {

template <typename... Args>
inline SpdyString SpdyStrCatImpl(const Args&... args) {
  std::ostringstream oss;
  int dummy[] = {1, (oss << args, 0)...};
  static_cast<void>(dummy);
  return oss.str();
}

template <typename... Args>
inline void SpdyStrAppendImpl(SpdyString* output, Args... args) {
  output->append(SpdyStrCatImpl(args...));
}

template <typename... Args>
inline SpdyString SpdyStringPrintfImpl(const Args&... args) {
  return base::StringPrintf(std::forward<const Args&>(args)...);
}

template <typename... Args>
inline void SpdyStringAppendFImpl(const Args&... args) {
  base::StringAppendF(std::forward<const Args&>(args)...);
}

inline char SpdyHexDigitToIntImpl(char c) {
  return base::HexDigitToInt(c);
}

inline SpdyString SpdyHexDecodeImpl(SpdyStringPiece data) {
  return HexDecode(data);
}

inline bool SpdyHexDecodeToUIntImpl(SpdyStringPiece data, uint32_t* out) {
  if (data.empty() || data.size() > 8u)
    return false;
  // Pad with leading zeros.
  SpdyString data_padded =
      SpdyString(8 - data.size(), '0').append(data.begin(), data.end());
  return base::HexStringToUInt(data_padded, out);
}

inline SpdyString SpdyHexEncodeImpl(const char* bytes, size_t size) {
  return base::ToLowerASCII(base::HexEncode(bytes, size));
}

inline SpdyString SpdyHexEncodeUIntAndTrimImpl(uint32_t data) {
  if (data == 0)
    return "0";
  uint32_t data_network_order = base::HostToNet32(data);
  SpdyString hex_string = base::ToLowerASCII(base::HexEncode(
      reinterpret_cast<char*>(&data_network_order), sizeof(uint32_t)));
  // Trim leading zeros.
  auto pos = hex_string.find_first_not_of('0');
  DCHECK_NE(pos, SpdyString::npos);
  return hex_string.substr(pos);
}

inline SpdyString SpdyHexDumpImpl(SpdyStringPiece data) {
  return HexDump(data);
}

}  // namespace net

#endif  // NET_SPDY_PLATFORM_IMPL_SPDY_STRING_UTILS_IMPL_H_
