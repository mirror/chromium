// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/base/enqueue_order.h"

namespace blink {
namespace scheduler {
namespace internal {

EnqueueOrder EnqueueOrderGenerator::GenerateNext() {
  EnqueueOrder generated = reinterpret_cast<EnqueueOrderValueType>(
      base::subtle::NoBarrier_AtomicIncrement(&next_, 1));
#if defined(ARCH_CPU_32_BITS)
  // Make sure overflow skips special EnqueueOrder values on 32-bit.
  while (generated < EnqueueOrderValues::FIRST)
    generated = GenerateNext();
#endif  // defined(ARCH_CPU_32_BITS)
  return generated;
}

}  // namespace internal
}  // namespace scheduler
}  // namespace blink
