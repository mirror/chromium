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
  size_t remaining = output_length;
  unsigned char* offset = reinterpret_cast<unsigned char*>(output);
  size_t actual;
  size_t read_len;
  do {
    // The syscall has a maximum number of bytes that can be read at once.
    read_len =
        remaining > MX_CPRNG_DRAW_MAX_LEN ? MX_CPRNG_DRAW_MAX_LEN : remaining;

    mx_status_t status = mx_cprng_draw(offset, read_len, &actual);
    CHECK(status == NO_ERROR);

    remaining -= actual;
    offset += actual;
  } while (remaining > 0);
}

}  // namespace base
