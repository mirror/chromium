// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/numerics/safe_conversions.h"
#include "media/filters/ivf_parser.h"
#include "media/filters/vp9_parser.h"

struct Environment {
  Environment() {
    // Disable noisy logging as per "libFuzzer in Chrome" documentation:
    // testing/libfuzzer/getting_started.md#Disable-noisy-error-message-logging.
    logging::SetMinLogLevel(logging::LOG_FATAL);
  }
};

Environment* env = new Environment();

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  uint8_t fuzz_type;
  if (size < sizeof(fuzz_type))
    return 0;
  fuzz_type = data[0];
  data += sizeof(fuzz_type);
  size -= sizeof(fuzz_type);

  if (fuzz_type > 2)
    return 0;

  const uint8_t* ivf_payload = nullptr;
  media::IvfParser ivf_parser;
  media::IvfFileHeader ivf_file_header;
  media::IvfFrameHeader ivf_frame_header;

  if (!ivf_parser.Initialize(data, size, &ivf_file_header))
    return 0;

  media::Vp9Parser vp9_parser(fuzz_type == 1);
  // Parse until the end of stream/unsupported stream/error in stream is found.
  while (ivf_parser.ParseNextFrame(&ivf_frame_header, &ivf_payload)) {
    media::Vp9FrameHeader vp9_frame_header;
    vp9_parser.SetStream(ivf_payload, ivf_frame_header.frame_size);
    // TODO(kcwu): further fuzzing the case of Vp9Parser::kAwaitingRefresh.
    while (vp9_parser.ParseNextFrame(&vp9_frame_header) ==
           media::Vp9Parser::kOk) {
      // Repeat until all frames processed.
    }
  }

  return 0;
}
