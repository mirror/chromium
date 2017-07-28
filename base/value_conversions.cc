// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/value_conversions.h"

#include <stdint.h>

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "base/values.h"

namespace base {
// |Value| internally stores strings in UTF-8, so we have to convert from the
// system native code to UTF-8 and back.
std::unique_ptr<Value> CreateFilePathValue(const FilePath& in_value) {
  return base::MakeUnique<Value>(in_value.AsUTF8Unsafe());
}

bool GetValueAsFilePath(const Value& value, FilePath* file_path) {
  std::string str;
  if (!value.GetAsString(&str))
    return false;
  if (file_path)
    *file_path = FilePath::FromUTF8Unsafe(str);
  return true;
}

// |Value| does not support 64-bit integers, and doubles do not have enough
// precision, so we store the 64-bit time value as a string instead.
std::unique_ptr<Value> CreateTimeDeltaValue(const TimeDelta& time) {
  std::string string_value = base::Int64ToString(time.ToInternalValue());
  return base::MakeUnique<Value>(string_value);
}

bool GetValueAsTimeDelta(const Value& value, TimeDelta* time) {
  std::string str;
  int64_t int_value;
  if (!value.GetAsString(&str) || !base::StringToInt64(str, &int_value))
    return false;
  if (time)
    *time = TimeDelta::FromInternalValue(int_value);
  return true;
}

std::unique_ptr<Value> CreateUnguessableTokenValue(
    const UnguessableToken& token) {
  uint64_t high_low[2] = {
      token.GetHighForSerialization(), token.GetLowForSerialization(),
  };

  static_assert(sizeof(high_low) == 2 * sizeof(uint64_t),
                "array size not match");

  return MakeUnique<Value>(HexEncode(high_low, 2 * sizeof(uint64_t)));
}

bool GetValueAsUnguessableToken(const Value& value, UnguessableToken* token) {
  if (!value.is_string()) {
    return false;
  }

  std::vector<uint8_t> high_low_bytes;
  if (!HexStringToBytes(value.GetString(), &high_low_bytes)) {
    return false;
  }

  if (high_low_bytes.size() != 2 * sizeof(uint64_t)) {
    return false;
  }

  uint64_t* high_low = reinterpret_cast<uint64_t*>(high_low_bytes.data());
  *token = UnguessableToken::Deserialize(high_low[0], high_low[1]);
  return true;
}
}  // namespace base
