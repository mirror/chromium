// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <unordered_map>
#include <vector>

#include "base/macros.h"

namespace language {

class LanguageCodeLocator {
 public:
  LanguageCodeLocator();
  ~LanguageCodeLocator();

  // Find the language code given a coordinate.
  // If the lat, lng pair is not found, will return an empty vector.
  std::vector<std::string> GetLanguageCode(double lat, double lng) const;

 private:
  // Map from s2 cellid to ';' delimited list of language codes.
  const std::unordered_map<uint64_t, std::string> district_lang_;

  DISALLOW_COPY_AND_ASSIGN(LanguageCodeLocator);
};

}  // namespace language
