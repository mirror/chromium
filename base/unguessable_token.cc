// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/unguessable_token.h"

#include <vector>

#include "base/format_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace base {
namespace {
const char kSeperator[] = ",";
}

UnguessableToken::UnguessableToken(uint64_t high, uint64_t low)
    : high_(high), low_(low) {}

std::string UnguessableToken::GetStringForSerialization() const {
  return JoinString({Uint64ToString(GetHighForSerialization()),
                     Uint64ToString(GetLowForSerialization())},
                    kSeperator);
}

std::string UnguessableToken::ToString() const {
  return base::StringPrintf("(%08" PRIX64 "%08" PRIX64 ")", high_, low_);
}

// static
UnguessableToken UnguessableToken::Create() {
  UnguessableToken token;
  // Use base::RandBytes instead of crypto::RandBytes, because crypto calls the
  // base version directly, and to prevent the dependency from base/ to crypto/.
  base::RandBytes(&token, sizeof(token));
  return token;
}

// static
UnguessableToken UnguessableToken::Deserialize(uint64_t high, uint64_t low) {
  // Receiving a zeroed out UnguessableToken from another process means that it
  // was never initialized via Create(). Treat this case as a security issue.
  DCHECK(!(high == 0 && low == 0));
  return UnguessableToken(high, low);
}

// static
UnguessableToken UnguessableToken::Deserialize(const std::string& str) {
  std::vector<std::string> high_low =
      SplitString(str, kSeperator, KEEP_WHITESPACE, SPLIT_WANT_ALL);
  if (high_low.size() != 2) {
    return UnguessableToken();
  }

  uint64_t high;
  if (!StringToUint64(high_low[0], &high)) {
    return UnguessableToken();
  }

  uint64_t low;
  if (!StringToUint64(high_low[1], &low)) {
    return UnguessableToken();
  }

  return Deserialize(high, low);
}

std::ostream& operator<<(std::ostream& out, const UnguessableToken& token) {
  return out << token.ToString();
}

}  // namespace base
