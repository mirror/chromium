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
  // Need at least 2 bytes for the options flags and one byte of test data.
  if (size < 3)
    return 0;

  static BlinkFuzzerTestSupport test_support = BlinkFuzzerTestSupport();
  FuzzedDataProvider fuzzed_data_provider(data, size);

  // Use the first 2 bytes of fuzz data to randomize the tokenizer options.
  HTMLParserOptions options;
  options.script_enabled = fuzzed_data_provider.ConsumeBool();
  options.plugins_enabled = fuzzed_data_provider.ConsumeBool();
  size -= 2;

  std::unique_ptr<HTMLTokenizer> tokenizer = HTMLTokenizer::Create(options);
  SegmentedString input;
  HTMLToken token;
  while (size > 0) {
    // The tokenizer deals with incremental strings as they are received.
    // Split the input into a bunch of small chunks to throw partial tokens
    // at the tokenizer and exercise the state machine and resumption.
    CString chunk = fuzzed_data_provider.ConsumeBytesInRange(1, 32);
    size -= chunk.length();
    if (chunk.length() == 0) {
      return 0;
    }
    SegmentedString segment(String(chunk.data(), chunk.length()));
    input.Append(segment);
    // If a token was generated from the input then the next call
    // needs to use a fresh token for output. If a token is not generated
    // then the same token instance needs to be reused in the next calls
    // until a token is generated (or we reach the end of the input).
    bool token_generated;
    do {
      token_generated = tokenizer->NextToken(input, token);
      if (token_generated) {
        token.Clear();
      }
    } while (token_generated);
  }
  return 0;
}
