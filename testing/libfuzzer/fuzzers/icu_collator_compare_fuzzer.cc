// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include "testing/libfuzzer/fuzzers/icu_fuzzer_common.h"
#include "third_party/icu/fuzzers/fuzzer_utils.h"
#include "third_party/icu/source/i18n/unicode/coll.h"

IcuEnvironment* env = new IcuEnvironment();

using namespace std;
using namespace icu_fuzzer;

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (!size)
    return 0;

  string locale;
  bool ret = ExtractString(data, size, locale);
  if (!ret)
    return 0;

  std::unique_ptr<icu::Collator> collator(
      Collator::InitializeCollator(data, locale.c_str(), size));
  if (!collator)
    return 0;

  icu::UnicodeString string_val1;
  icu::UnicodeString string_val2;
  if (!ExtractStringSetting(data, size, "string_val1", &string_val1) ||
      !ExtractStringSetting(data, size, "string_val2", &string_val2)) {
    return 0;
  }
  collator->compare(string_val1, string_val2);

  return 0;
}
