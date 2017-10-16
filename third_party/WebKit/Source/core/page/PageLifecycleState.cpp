// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/page/PageLifecycleState.h"
#include "platform/wtf/Assertions.h"

namespace blink {

AtomicString PageLifecycleStateString(PageLifecycleState state) {
  switch (state) {
    case kPageLifecycleStateUnknown:
      return "unknown";
    case kPageLifecycleStateDiscarded:
      return "discarded";
    case kPageLifecycleStateStopped:
      return "stopped";
    case kPageLifecycleStateCached:
    case kPageLifecycleStateTerminated:
    case kPageLifecycleStateHidden:
    case kPageLifecycleStatePassive:
    case kPageLifecycleStateActive:
    case kPageLifecycleStateLoading:
      return "unused";
  }

  NOTREACHED();
  return AtomicString();
}

}  // namespace blink
