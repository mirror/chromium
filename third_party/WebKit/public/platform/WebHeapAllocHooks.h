// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebHeapAllocHooks_h
#define WebHeapAllocHooks_h

#include <cstdint>

#include "WebCommon.h"

namespace blink {

class WebHeapAllocHooks {
 public:
  using AllocationHook = void (*)(uint8_t*, size_t, const char*);
  using FreeHook = void (*)(uint8_t*);

  BLINK_PLATFORM_EXPORT static void SetAllocationHook(AllocationHook);
  BLINK_PLATFORM_EXPORT static void SetFreeHook(FreeHook);
};

}  // namespace blink

#endif
