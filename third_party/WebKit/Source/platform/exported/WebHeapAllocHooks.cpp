// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebHeapAllocHooks.h"

#include "platform/heap/Heap.h"

namespace blink {

void WebHeapAllocHooks::SetAllocationHook(AllocationHook alloc_hook) {
  HeapAllocHooks::SetAllocationHook(alloc_hook);
}

void WebHeapAllocHooks::SetFreeHook(FreeHook free_hook) {
  HeapAllocHooks::SetFreeHook(free_hook);
}

}  // namespace blink
