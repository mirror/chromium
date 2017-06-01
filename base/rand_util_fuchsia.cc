// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/rand_util.h"

#include <magenta/syscalls.h>

#include "base/logging.h"

namespace base {

uint64_t RandUint64() {
  uint64_t number;
  RandBytes(&number, sizeof(number));
  return number;
}

void RandBytes(void* output, size_t output_length) {
  size_t bytes_read;
  CHECK(mx_cprng_draw(output, output_length, &bytes_read) == NO_ERROR &&
        bytes_read == output_length);
}

}  // namespace base
