// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_THREADING_SCOPED_BLOCKING_CALL_H
#define BASE_THREADING_SCOPED_BLOCKING_CALL_H

#include "base/base_export.h"
#include "base/logging.h"

namespace base {

// BlockingType indicates the likelihood that a blocking call will actually
// block.
enum class BlockingType {
  // The call might block (e.g. file I/O that might hit in memory cache).
  MAY_BLOCK,
  // The call will definitely block (e.g. cache already checked and now pinging
  // server synchronously).
  WILL_BLOCK
};

namespace internal {
class BlockingObserver;
}

// This class can be instantiated in a scope where a a blocking call (which
// isn't using local computing resources -- e.g. a synchronous network request)
// is made. Instantiation will hint the BlockingObserver for this thread about
// the scope of the blocking operation.
//
// In particular, when instantiated from a TaskScheduler parallel or sequenced
// task, this will allow the thread to be replaced in its pool (more or less
// aggressively depending on BlockingType).
class BASE_EXPORT ScopedBlockingCall {
 public:
  ScopedBlockingCall(BlockingType blocking_type);
  ~ScopedBlockingCall();

 private:
  const BlockingType blocking_type_;
  internal::BlockingObserver* const blocking_observer_;

  // Previous ScopedBlockingCall instantiated on this thread.
  ScopedBlockingCall* const previous_scoped_blocking_call_;

  // Whether the observer was notified when this ScopedBlockingCall was
  // instantiated.
  const bool notified_observer_;

  DISALLOW_COPY_AND_ASSIGN(ScopedBlockingCall);
};

namespace internal {

// Interface for an observer to be informed when a thread enters or exits
// the scope of ScopedBlockingCall objects.
class BASE_EXPORT BlockingObserver {
 public:
  virtual ~BlockingObserver() = default;

  // Invoked when:
  // - A MAY_BLOCK ScopedBlockingCall is instantiated on a thread where there is
  //   no active ScopedBlockingCall.
  // - A WILL_BLOCK ScopedBlockingCall is instantiated on a thread when there is
  //   no active WILL_BLOCK ScopedBlockingCall.
  virtual void BlockingScopeEntered(BlockingType blocking_type) = 0;

  // Invoked when the last ScopedBlockingCall on a thread is destroyed.
  virtual void BlockingScopeExited() = 0;
};

// Registers |blocking_observer| to be informed when the current thread enters
// of exits the scope of ScopedBlockingCall objects. It is invalid to call this
// on a thread where there is an active ScopedBlockingCall.
BASE_EXPORT void SetBlockingObserverForCurrentThread(
    BlockingObserver* blocking_observer);

BASE_EXPORT void ClearBlockingObserverForTesting();

}  // namespace internal

}  // namespace base

#endif  // BASE_THREADING_SCOPED_BLOCKING_CALL_H
