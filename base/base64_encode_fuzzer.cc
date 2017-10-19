// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/base64.h"
#include "base/logging.h"

// Encode some random data, and then decode it.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::string encode_output;
  std::string decode_output;
  std::string data_str(reinterpret_cast<const char*>(data), size);
  base::Base64Encode(data_str, &encode_output);
  CHECK(base::Base64Decode(encode_output, &decode_output));
  CHECK_EQ(data_str, decode_output);
  return 0;
}
