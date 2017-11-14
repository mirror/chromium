// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_ENQUEUE_ORDER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_ENQUEUE_ORDER_H_

#include <stdint.h>

#include <type_traits>

#include "base/atomicops.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "build/build_config.h"

namespace blink {
namespace scheduler {
namespace internal {

// An atomic machine word is used to generate indices.
using EnqueueOrderGeneratorType = base::subtle::AtomicWord;

// Each index is stored as an unsigned representation of the generated word. The
// space of indices in a word is assumed to be infinite in practice, except on
// 32-bit platforms where a few tricks are used in operator<() and
// GenerateNext().
using EnqueueOrderValueType =
    typename std::make_unsigned<EnqueueOrderGeneratorType>::type;

// Special EnqueueOrder values.
// TODO(scheduler-dev): Remove explicit casts when c++17 comes.
enum class EnqueueOrderValues : EnqueueOrderValueType {
  // Invalid EnqueueOrder.
  NONE = 0,

  // Earliest possible EnqueueOrder, to be used for fence blocking  (ref.
  // InsertFencePosition::BEGINNING_OF_TIME).
  BLOCKING_FENCE = 1,
  // More special values can be added above, this entry must remain last.
  FIRST
};

#if defined(ARCH_CPU_32_BITS)
// operator<()'s overflow logic is based on the assumption that any enqueued
// task is at least processed before one billion (2^30) more tasks are enqueued.
// operator<() views the 2^32 space as 4 quadrants of 2^30 indices. The
// comparison then enforces that for two EnqueueOrders in different quadrants,
// the EnqueueOrder in the quadrant preceding the other is always considered
// smaller. Since any enqueued task is at least processed before its quadrant
// and the next have been fully used (assumption #1), the comparison operator is
// guaranteed to never see numbers more than a single quadrant apart and
// operator<() can hence be done without further state. Furthermore, it's
// unnecessary to bother with quadrants in the majority of comparisons except
// when a number from the 4th quadrant should be considered smaller than one in
// the first quadrant (after overflow).
static_assert(sizeof(EnqueueOrderValueType) == 4);
constexpr EnqueueOrderValueType kLastQuadrantMask =
    0b11 << (8 * sizeof(EnqueueOrderValueType) - 2);
#endif  // defined(ARCH_CPU_32_BITS)

// An EnqueueOrder is an unsigned word-sized int with custom comparison
// operators (see above).
struct EnqueueOrder {
  bool operator<(const EnqueueOrder& other) const {
#if defined(ARCH_CPU_32_BITS)
    if (value_ < static_cast<EnqueueOrder>(EnqueueOrderValues::FIRST) ||
        other_.value_ < static_cast<EnqueueOrder>(EnqueueOrderValues::FIRST)) {
      // Special values always come first.
      return value_ < other_.value_;
    }

    const bool is_in_last_quadrant =
        (value_ & kLastQuadrantMask == kLastQuadrantMask);
    const bool other_is_in_last_quadrant =
        (other_.value_ & kLastQuadrantMask == kLastQuadrantMask);
    if (is_in_last_quadrant) {
      return !other_is_in_last_quadrant || value_ < other_.value_;
    } else if (other_is_in_last_quadrant) {
      // Already know |!is_in_last_quadrant| per previous if.
      return false;
    }
    return value_ < other_.value_;
#else   // defined(ARCH_CPU_32_BITS)
    return value_ < other_.value_;
#endif  // defined(ARCH_CPU_32_BITS)
  }

  bool operator>(const EnqueueOrder& other) const { return other < *this; }

  // This value is initialized from an atomic number but it's kept const
  // thereafter and can hence be read without atomic loads.
  EnqueueOrderValueType value_;
};

// An integer used to provide ordering of tasks. The scheduler assumes these
// values will not overflow on 64-bit, special logic is used to support overflow
// without using a lock on 32-bit. This overflow logic is based on the
// assumption that every task is at least processed before one billion (2^30)
// more tasks are enqueued.
class EnqueueOrderGenerator {
 public:
  EnqueueOrderGenerator() = default;
  ~EnqueueOrderGenerator() = default;

  // Returns a monotonically increasing integer, starting from one. Can be
  // called from any thread.
  EnqueueOrder GenerateNext();

  static bool IsValidEnqueueOrder(EnqueueOrder enqueue_order) {
    return enqueue_order != static_cast<EnqueueOrder>(EnqueueOrderValues::NONE);
  }

 private:
  EnqueueOrderGeneratorType next_ =
      static_cast<EnqueueOrder>(EnqueueOrderValues::FIRST);

  DISALLOW_COPY_AND_ASSIGN(EnqueueOrderGenerator);
};

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_ENQUEUE_ORDER_H_
