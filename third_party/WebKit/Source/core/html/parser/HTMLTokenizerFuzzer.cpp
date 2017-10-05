// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "HTMLTokenizer.h"

#include <stddef.h>
#include <stdint.h>
#include "platform/testing/BlinkFuzzerTestSupport.h"
#include "platform/testing/FuzzedDataProvider.h"

using namespace blink;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  static BlinkFuzzerTestSupport test_support = BlinkFuzzerTestSupport();
  FuzzedDataProvider fuzzed_data_provider(data, size);
  HTMLParserOptions options;
  std::unique_ptr<HTMLTokenizer> tokenizer = HTMLTokenizer::Create(options);
  SegmentedString input;
  HTMLToken token;
  while (size > 0) {
    CString chunk = fuzzed_data_provider.ConsumeBytesInRange(1, 32);
    size -= chunk.length();
    if (chunk.length() == 0) {
      return 0;
    }
    SegmentedString segment(String(chunk.data(), chunk.length()));
    input.Append(segment);
    bool token_processed;
    do {
      token_processed = tokenizer->NextToken(input, token);
      if (token_processed) {
        token.Clear();
      }
    } while (token_processed);
  }
  return 0;
}
