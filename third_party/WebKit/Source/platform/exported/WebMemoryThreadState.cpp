// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/WebMemoryThreadState.h"

#include "platform/heap/ThreadState.h"

namespace blink {

void WebMemoryThreadState::SetForceMemoryPressureThreshold(size_t threshold) {
  ThreadState::SetForceMemoryPressureThreshold(threshold);
}

size_t WebMemoryThreadState::GetForceMemoryPressureThreshold() {
  return ThreadState::GetForceMemoryPressureThreshold();
}

}  // namespace blink
