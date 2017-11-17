// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SWAP_RESULT_H_
#define UI_GFX_SWAP_RESULT_H_

#include "base/time/time.h"

namespace gfx {

enum class SwapResult {
  SWAP_ACK,
  SWAP_FAILED,
  SWAP_NAK_RECREATE_BUFFERS,
  SWAP_RESULT_LAST = SWAP_NAK_RECREATE_BUFFERS,
};

struct SwapResponse {
  SwapResponse() = default;
  SwapResponse(base::TimeTicks start,
               SwapResult result = SwapResult::SWAP_FAILED)
      : result(result), swap_start(start), swap_end(start) {}

  SwapResponse& Finalize(base::TimeTicks end, gfx::SwapResult res) {
    result = res;
    swap_end = end;
    return *this;
  }

  uint64_t swap_id = 0;
  SwapResult result = SwapResult::SWAP_FAILED;

  // Sampled by Chrome. Supported by all platforms.
  base::TimeTicks swap_start;
  base::TimeTicks swap_end;
};

}  // namespace gfx

#endif  // UI_GFX_SWAP_RESULT_H_
