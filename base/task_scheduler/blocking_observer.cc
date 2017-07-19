// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task_scheduler/blocking_observer.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace base {
namespace internal {

namespace {

LazyInstance<ThreadLocalPointer<BlockingObserver>>::Leaky
    tls_blocking_observer = LAZY_INSTANCE_INITIALIZER;

}  // namespace

BlockingObserver* GetBlockingObserver() {
  return tls_blocking_observer.Get().Get();
}

void SetBlockingObserver(BlockingObserver* blocking_observer) {
  tls_blocking_observer.Get().Set(blocking_observer);
}

}  // namespace internal
}  // namespace base
