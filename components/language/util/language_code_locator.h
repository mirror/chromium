// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <unordered_map>

namespace s2 {
class CellId;
}  // namespace s2

namespace language {

class LanguageCodeLocator {
 public:
  explicit LanguageCodeLocator();

  std::string FindLangCode(double lat, double lng);

 private:
  // TODO: consider s2 index.
  std::unordered_map<s2::CellId, string> s2cell_lang_;
};

}  // namespace language