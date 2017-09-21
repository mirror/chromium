// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <unordered_map>

namespace language {

class LanguageCodeLocator {
 public:
  LanguageCodeLocator();

  // Find the language code given a coordinate.
  std::string GetLanguageCode(double lat, double lng) const;

 private:
  std::unordered_map<string, string> district_lang_;
};

}  // namespace language