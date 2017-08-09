// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchStore;
enum class PrefetchItemState;

// Reconciliation task responsible for finalizing entries for which their
// freshness date are past the limits specific to each pipeline bucket. Entries
// considered stale are moved to the "finished" state and have their error code
// column set to the PrefetchItemErrorCode value that identifies the bucket they
// were at.
// Note that this task should be ordered to run before others that perform
// reconciliation with external systems. This will guarantee that the entries
// finalized by it will promptly effect any external system processing them.
class StaleEntryFinalizerTask : public Task {
 public:
  using NowCallback = base::RepeatingCallback<base::Time()>;

  // Lists of "stable" states for each bucket at which entries should be
  // considered freshness checks.
  static const std::vector<PrefetchItemState> kBucket1StatesToExpire;
  static const std::vector<PrefetchItemState> kBucket2StatesToExpire;
  static const std::vector<PrefetchItemState> kBucket3StatesToExpire;

  // Bucket specific periods of time for which entries are considered fresh.
  // TODO(carlosk): Figure out if these values should be Finch controlled.
  static const base::TimeDelta kBucket1FreshPeriod;
  static const base::TimeDelta kBucket2FreshPeriod;
  static const base::TimeDelta kBucket3FreshPeriod;

  StaleEntryFinalizerTask(PrefetchStore* prefetch_store);
  ~StaleEntryFinalizerTask() override;

  void Run() override;

  // Allows tests to control the source of current time values used internally
  // for freshness checks.
  void SetNowCallbackForTesting(NowCallback now_callback) {
    now_callback_ = now_callback;
  }

  // Will be set to true upon after an error-free run.
  bool ran_successfully() { return ran_successfully_; }

 private:
  void OnExpirationConcluded(bool success);

  // Prefetch store to execute against. Not owned.
  PrefetchStore* prefetch_store_;

  // Defaults to base::Time::Now upon construction.
  NowCallback now_callback_;

  bool ran_successfully_ = false;

  base::WeakPtrFactory<StaleEntryFinalizerTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(StaleEntryFinalizerTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STALE_ENTRY_FINALIZER_TASK_H_
