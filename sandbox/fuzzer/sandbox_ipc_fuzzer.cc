// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "sandbox/win/src/crosscall_server.cc"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  sandbox::CrossCallParamsEx* ccp = nullptr;
  uint32_t output_size = 0;
  // put your fuzzing code here and use data+size as input.
  ccp = sandbox::CrossCallParamsEx::CreateFromBuffer(const_cast<uint8_t*>(data),
                                                     size, &output_size);
  return 0;
}
