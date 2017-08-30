// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebHTTPHeaderSet_h
#define WebHTTPHeaderSet_h

#include <set>
#include <string>
#include "base/strings/string_util.h"

namespace blink {

struct CompareIgnoreCase {
  bool operator()(const std::string& Left, const std::string& Right) const {
    return base::ToLowerASCII(Left) < base::ToLowerASCII(Right);
  }
};

using WebHTTPHeaderSet = std::set<std::string, CompareIgnoreCase>;

}  // namespace blink

#endif  // WebHTTPHeaderSet_h
