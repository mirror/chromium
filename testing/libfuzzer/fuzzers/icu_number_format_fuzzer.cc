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
#include "testing/libfuzzer/fuzzers/intl-objects.h"
#include "third_party/icu/fuzzers/fuzzer_utils.h"
#include "third_party/icu/source/i18n/unicode/decimfmt.h"

IcuEnvironment* env = new IcuEnvironment();

using namespace std;
using namespace fuzzer;

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (!size)
    return 0;

  string locale;
  bool ret = ExtractString(reinterpret_cast<const char*&>(data), size, locale);
  if (!ret)
    return 0;

  std::unique_ptr<icu::DecimalFormat> number_format(
      NumberFormat::InitializeNumberFormat(data, locale.c_str(), size));
  if (!number_format)
    return 0;

  icu::UnicodeString formatted;
  if (size < sizeof(double)) {
    return 0;
  }
  double number = ((double*)data)[0];
  data += sizeof(double);
  size -= sizeof(double);
  number_format->format(number, formatted);

  return 0;
}
