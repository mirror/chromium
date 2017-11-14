// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <algorithm>

#include "content/browser/webauth/cbor/cbor_reader.h"  // nogncheck

namespace content {

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::vector<uint8_t> input(data, data + size);
  content::CBORReader::Read(input);
  return 0;
}

}  // namespace content
