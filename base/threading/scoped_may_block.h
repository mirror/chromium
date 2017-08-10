// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_SCOPED_MAY_BLOCK_H
#define BASE_THREADING_SCOPED_MAY_BLOCK_H

#include "base/base_export.h"
#include "base/logging.h"

namespace base {

// This class can be instantiated before a blocking call is made. Instantiation
// will hint the BlockingObserver for this thread about the scope of the
// blocking operation.
//
// In particular, on TaskScheduler owned threads, this will allow the thread to
// be replaced in its pool.
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

 private:
#if DCHECK_IS_ON()
  bool in_blocked_scope_ = false;

  friend class base::ScopedWillBlock;
#endif
};

void SetBlockingObserverForCurrentThread(BlockingObserver* blocking_observer);

}  // namespace internal

}  // namespace base

#endif  // BASE_THREADING_SCOPED_MAY_BLOCK_H
