// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_SCOPED_WILL_BLOCK_H
#define BASE_THREADING_SCOPED_WILL_BLOCK_H

#include "base/base_export.h"
#include "base/logging.h"

namespace base {

// This class can be instantiated in a scope where a a blocking call (which
// isn't using local computing resources -- e.g. a synchronous network request)
// is made. Instantiation will hint the BlockingObserver for this thread about
// the scope of the blocking operation.
//
// In particular, when instantiated from a TaskScheduler parallel or sequenced
// task, this will allow the thread to be replaced in its pool.
class BASE_EXPORT ScopedWillBlock {
 public:
  ScopedWillBlock();
  ~ScopedWillBlock();
};

namespace internal {

// Interface for an observer to be informed when a thread enters or exits
// the scope of a ScopedWillBlock object.
class BASE_EXPORT BlockingObserver {
 public:
  virtual void BlockingScopeEntered() = 0;
  virtual void BlockingScopeExited() = 0;
};

void SetBlockingObserverForCurrentThread(BlockingObserver* blocking_observer);

}  // namespace internal

}  // namespace base

#endif  // BASE_THREADING_SCOPED_WILL_BLOCK_H
