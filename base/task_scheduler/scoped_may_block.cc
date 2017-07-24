// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/scoped_may_block.h"

#include "base/task_scheduler/blocking_observer.h"

namespace base {

ScopedMayBlock::ScopedMayBlock() {
  if (internal::GetBlockingObserver())
    internal::GetBlockingObserver()->TaskBlocked();
}

ScopedMayBlock::~ScopedMayBlock() {
  if (internal::GetBlockingObserver())
    internal::GetBlockingObserver()->TaskUnblocked();
}

}  // namespace base
