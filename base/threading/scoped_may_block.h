// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_SCOPED_MAY_BLOCK_H
#define BASE_THREADING_SCOPED_MAY_BLOCK_H

#include "base/base_export.h"

namespace base {

namespace internal {

// Interface to inform about threads that become blocked or unblocked.
class BASE_EXPORT BlockingObserver {
 public:
  virtual void ThreadBlocked() = 0;
  virtual void ThreadUnblocked() = 0;
};

void SetBlockingObserverForCurrentThread(BlockingObserver* blocking_observer);

}  // namespace internal

class BASE_EXPORT ScopedMayBlock {
 public:
  ScopedMayBlock();
  ~ScopedMayBlock();
};

}  // namespace base

#endif  // BASE_THREADING_SCOPED_MAY_BLOCK_H
