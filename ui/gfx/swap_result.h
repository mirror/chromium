// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SWAP_RESULT_H_
#define UI_GFX_SWAP_RESULT_H_

namespace gfx {

enum class SwapResult {
  SWAP_ACK,
  SWAP_FAILED,
  SWAP_NAK_RECREATE_BUFFERS,
  SWAP_RESULT_LAST = SWAP_NAK_RECREATE_BUFFERS,
};

struct SwapResponse {
  uint64_t swap_id;
  SwapResult result;

  // Sampled by Chrome. Supported by all platforms.
  base::TimeTicks swap_start;
  base::TimeTicks swap_end;
};

}  // namespace gfx

#endif  // UI_GFX_SWAP_RESULT_H_
