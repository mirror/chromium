// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task/sequence_manager/enqueue_order.h"

namespace base {
namespace sequence_manager {
namespace internal {

EnqueueOrderGenerator::EnqueueOrderGenerator() = default;

EnqueueOrderGenerator::~EnqueueOrderGenerator() = default;

EnqueueOrder EnqueueOrderGenerator::GenerateNext() {
  base::AutoLock lock(lock_);
  return next_++;
}

}  // namespace internal
}  // namespace sequence_manager
}  // namespace base
