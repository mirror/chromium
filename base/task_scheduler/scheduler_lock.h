// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SCHEDULER_LOCK_H
#define BASE_TASK_SCHEDULER_SCHEDULER_LOCK_H

#include <memory>

#include "base/base_export.h"
#include "base/macros.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/task_scheduler/scheduler_lock_impl.h"

namespace base {
namespace internal {

// SchedulerLock should be used anywhere a lock would be used in the scheduler.
// When DCHECK_IS_ON(), lock checking occurs. Otherwise, SchedulerLock is
// equivalent to base::Lock.
//
// The shape of SchedulerLock is as follows:
// SchedulerLock(const SchedulerLock* predecessor)
//     Constructor that specifies an allowed predecessor for that lock.
//     DCHECKs
//         On Construction if |predecessor| forms a predecessor lock cycle or
//             if |predecessor| is a leaf (see next constructor).
//         On Acquisition if the previous lock acquired on the thread is not
//             |predecessor|. Okay if there was no previous lock acquired.
//
// SchedulerLock(IsLeaf is_leaf)
//     Constructor for a leaf SchedulerLock.
//     DCHECKs
//         On Acquisition if the previous lock acquired on the thread is a leaf.
//
// void Acquire()
//     Acquires the lock.
//
// void Release()
//     Releases the lock.
//
// void AssertAcquired().
//     DCHECKs if the lock is not acquired.
//
// std::unique_ptr<ConditionVariable> CreateConditionVariable()
//     Creates a condition variable using this as a lock.

#if DCHECK_IS_ON()
class SchedulerLock : public SchedulerLockImpl {
 public:
  struct IsLeaf {};

  explicit SchedulerLock(const SchedulerLock* predecessor = nullptr)
      : SchedulerLockImpl(predecessor, false) {}
  explicit SchedulerLock(IsLeaf is_leaf) : SchedulerLockImpl(nullptr, true) {}
};
#else  // DCHECK_IS_ON()
class SchedulerLock : public Lock {
 public:
  struct IsLeaf {};

  explicit SchedulerLock(const SchedulerLock* predecessor = nullptr) {}
  explicit SchedulerLock(IsLeaf is_leaf) {}

  std::unique_ptr<ConditionVariable> CreateConditionVariable() {
    return std::unique_ptr<ConditionVariable>(new ConditionVariable(this));
  }
};
#endif  // DCHECK_IS_ON()

// Provides the same functionality as base::AutoLock for SchedulerLock.
class AutoSchedulerLock {
 public:
  explicit AutoSchedulerLock(SchedulerLock& lock) : lock_(lock) {
    lock_.Acquire();
  }

  ~AutoSchedulerLock() {
    lock_.AssertAcquired();
    lock_.Release();
  }

 private:
  SchedulerLock& lock_;

  DISALLOW_COPY_AND_ASSIGN(AutoSchedulerLock);
};

}  // namespace internal
}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SCHEDULER_LOCK_H
