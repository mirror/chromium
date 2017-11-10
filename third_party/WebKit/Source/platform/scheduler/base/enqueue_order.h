// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_ENQUEUE_ORDER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_ENQUEUE_ORDER_H_

#include <stdint.h>

#include "base/task/sequence_manager/enqueue_order.h"

namespace blink {
namespace scheduler {
namespace internal {

// Temporary symbol redirection during move of scheduler/base to
// //base/task/sequence_manager.
using EnqueueOrder = base::sequence_manager::internal::EnqueueOrder;
using EnqueueOrderValues = base::sequence_manager::internal::EnqueueOrderValues;
using EnqueueOrderGenerator =
    base::sequence_manager::internal::EnqueueOrderGenerator;

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_ENQUEUE_ORDER_H_
