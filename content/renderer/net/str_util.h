// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_NET_STR_UTIL_H_
#define CONTENT_RENDERER_NET_STR_UTIL_H_

namespace content {

struct IgnoreCaseHash {
  size_t operator()(const std::string& value) const {
    std::string lcase = value;
    std::transform(lcase.begin(), lcase.end(), lcase.begin(), ::tolower);

    return std::hash<std::string>{}(lcase);
  }
};

struct IgnoreCaseEqual {
  bool operator()(const std::string& left, const std::string& right) const {
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(),
                      [](char a, char b) { return tolower(a) == tolower(b); });
  }
};

typedef std::unordered_set<std::string, IgnoreCaseHash, IgnoreCaseEqual>
    IgnoreCaseStringSet;

}  // namespace content

#endif  // CONTENT_RENDERER_NET_STR_UTIL_H_
