// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEMORY_SWAP_THRASHING_COMMON_H_
#define CHROME_BROWSER_MEMORY_SWAP_THRASHING_COMMON_H_

namespace memory {

enum class SwapThrashingLevel {
  SWAP_THRASHING_LEVEL_NONE,
  SWAP_THRASHING_LEVEL_SUSPECTED,
  SWAP_THRASHING_LEVEL_CONFIRMED,
};

}  // namespace memory

#endif  // CHROME_BROWSER_MEMORY_SWAP_THRASHING_COMMON_H_
