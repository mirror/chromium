// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SEQUENCE_MANAGER_ENQUEUE_ORDER_
#define BASE_TASK_SEQUENCE_MANAGER_ENQUEUE_ORDER_

#include <stdint.h>

#include "base/base_export.h"
#include "base/synchronization/lock.h"

namespace base {
namespace sequence_manager {
namespace internal {

using EnqueueOrder = uint64_t;

// TODO(scheduler-dev): Remove explicit casts when c++17 comes.
enum class EnqueueOrderValues : EnqueueOrder {
  // Invalid EnqueueOrder.
  NONE = 0,

  // Earliest possible EnqueueOrder, to be used for fence blocking (ref.
  // InsertFencePosition::BEGINNING_OF_TIME).
  BLOCKING_FENCE = 1,
  FIRST = 2,
};

// A 64bit integer used to provide ordering of tasks. NOTE The sequence manager
// assumes these values will not overflow.
// TODO(scheduler-dev): Remove BASE_EXPORT after move to
// base/task/sequence_manager is complete.
class BASE_EXPORT EnqueueOrderGenerator {
 public:
  EnqueueOrderGenerator();
  ~EnqueueOrderGenerator();

  // Returns a monotonically increasing integer, starting from one. Can be
  // called from any thread.
  EnqueueOrder GenerateNext();

  static bool IsValidEnqueueOrder(EnqueueOrder enqueue_order) {
    return enqueue_order != static_cast<EnqueueOrder>(EnqueueOrderValues::NONE);
  }

 private:
  base::Lock lock_;
  EnqueueOrder next_ = static_cast<EnqueueOrder>(EnqueueOrderValues::FIRST);
};

}  // namespace internal
}  // namespace sequence_manager
}  // namespace base

#endif  // BASE_TASK_SEQUENCE_MANAGER_ENQUEUE_ORDER_
