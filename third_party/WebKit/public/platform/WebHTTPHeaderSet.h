// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebHTTPHeaderSet_h
#define WebHTTPHeaderSet_h

#include <string>
#include <unordered_set>

namespace blink {

namespace {

// Check-webkit-style complains when we use |tolower| but wtf/ASCIICType.h isn't
// allowed in platform either.
inline char ToASCIILower(char c) {
  return c | ((c >= 'A' && c <= 'Z') << 5);
}

}  // namespace

struct HashIgnoreCase {
  size_t operator()(const std::string& Keyval) const {
    size_t h = 0;
    std::for_each(Keyval.begin(), Keyval.end(),
                  [&](char c) { h += ToASCIILower(c); });
    return h;
  }
};

struct EqualIgnoreCase {
  bool operator()(const std::string& Left, const std::string& Right) const {
    return Left.size() == Right.size() &&
           std::equal(Left.begin(), Left.end(), Right.begin(),
                      [](char a, char b) {
                        return ToASCIILower(a) == ToASCIILower(b);
                      });
  }
};

using WebHTTPHeaderSet =
    std::unordered_set<std::string, HashIgnoreCase, EqualIgnoreCase>;

}  // namespace blink

#endif  // WebHTTPHeaderSet_h
