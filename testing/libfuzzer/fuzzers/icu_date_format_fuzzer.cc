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
#include "third_party/icu/source/i18n/unicode/datefmt.h"

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

  std::unique_ptr<icu::SimpleDateFormat> date_format(
      DateFormat::InitializeDateTimeFormat(data, locale.c_str(), size));
  if (!date_format)
    return 0;

  icu::UnicodeString formatted;
  if (size < sizeof(UDate)) {
    return 0;
  }
  UDate date = reinterpret_cast<const UDate*>(data)[0];
  data += sizeof(UDate);
  size -= sizeof(UDate);
  date_format->format(date, formatted);

  return 0;
}
