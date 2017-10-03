// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "net/http2/decoder/http2_frame_decoder.h"

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  net::Http2FrameDecoderNoOpListener listener;
  net::Http2FrameDecoder decoder(&listener);
  net::DecodeBuffer buff(reinterpret_cast<const char*>(data), size);
  decoder.DecodeFrame(&buff);
  return 0;
}
