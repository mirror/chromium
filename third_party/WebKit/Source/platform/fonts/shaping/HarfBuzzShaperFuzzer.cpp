// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/Font.h"
#include "platform/fonts/FontCache.h"
#include "platform/fonts/shaping/HarfBuzzShaper.h"
#include "platform/testing/BlinkFuzzerTestSupport.h"

#include <stddef.h>
#include <stdint.h>
#include <unicode/ustring.h>

namespace blink {

int LLVMFuzzerInitialize(int*, char***) {
  static BlinkFuzzerTestSupport fuzzer_support = BlinkFuzzerTestSupport();
  return 0;
}

constexpr size_t kMaxInputLength = 80;

// Simple shaping fuzzer which only relies on available fonts provided by the
// webkit_unit_test environment.  TODO: Check whether this runs our custom
// fontconfig.
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  constexpr int32_t kDestinationCapacity = 2 * kMaxInputLength;
  int32_t converted_length = 0;
  UChar converted_input_buffer[kDestinationCapacity];
  UErrorCode error_code = U_ZERO_ERROR;

  // Discard trailing bytes.
  u_strFromUTF32(converted_input_buffer, kDestinationCapacity,
                 &converted_length, reinterpret_cast<const UChar32*>(data),
                 size / 4, &error_code);
  if (U_FAILURE(error_code))
    return 0;

  FontCachePurgePreventer font_cache_purge_preventer;
  FontDescription font_description;
  Font font(font_description);
  font_description.SetComputedSize(12.0f);
  // Only look for system fonts for now.
  font.Update(nullptr);

  HarfBuzzShaper shaper(converted_input_buffer, converted_length);
  RefPtr<ShapeResult> result = shaper.Shape(&font, TextDirection::kLtr);
  return 0;
}

}  // namespace blink

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  return blink::LLVMFuzzerInitialize(argc, argv);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  return blink::LLVMFuzzerTestOneInput(data, size);
}
