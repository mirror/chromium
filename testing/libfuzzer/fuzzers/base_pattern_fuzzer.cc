// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <string>

#include "base/strings/pattern.h"
#include "base/test/fuzzed_data_provider.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  base::FuzzedDataProvider fuzzed_data(data, size);

  // Peel off 64 bytes for the pattern. Truncate the pattern randomly. Do this
  // after consuming so the rest of the input remains consistent across fuzzer
  // runs.
  uint32_t pattern_length = fuzzed_data.ConsumeUint32InRange(0, 64);
  std::string pattern = fuzzed_data.ConsumeBytes(64).substr(0, pattern_length);

  // The rest of the fuzzed data is the string to search.
  std::string string_to_search = fuzzed_data.ConsumeRemainingBytes();
  base::MatchPattern(string_to_search, pattern);
  return 0;
}
